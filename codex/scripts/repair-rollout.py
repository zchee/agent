#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14"
# dependencies = [
#   "rich>=14.0.0",
# ]
# ///
"""Repair and prune stale Codex thread rollout paths in the state database."""

from __future__ import annotations

import argparse
import shutil
import sqlite3
import subprocess
import sys
from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from enum import StrEnum
from pathlib import Path
from urllib.parse import quote

from rich.console import Console
from rich.table import Table


DEFAULT_STATE_DB = Path.home() / ".config/codex/sqlite/state_5.sqlite"
TABLE_THREADS = "threads"
TABLE_THREAD_DYNAMIC_TOOLS = "thread_dynamic_tools"
TABLE_THREAD_SPAWN_EDGES = "thread_spawn_edges"
TABLE_AGENT_JOB_ITEMS = "agent_job_items"


class RepairMode(StrEnum):
    """Supported remediation modes."""

    ALL = "all"
    REPAIR = "repair"
    PRUNE = "prune"


@dataclass(frozen=True, slots=True)
class ThreadRow:
    """Thread state row relevant to rollout inventory validation.

    Args:
        thread_id: Primary key from the Codex threads table.
        rollout_path: Filesystem path recorded in the state database.
        archived: Whether the thread is archived in Codex state.
    """

    thread_id: str
    rollout_path: Path
    archived: bool


@dataclass(frozen=True, slots=True)
class RepairTarget:
    """A stale JSONL path that can be repointed to an existing Zstandard file.

    Args:
        thread_id: Primary key from the Codex threads table.
        old_path: Missing path currently stored in the database.
        new_path: Existing compressed rollout path to store.
    """

    thread_id: str
    old_path: Path
    new_path: Path


@dataclass(frozen=True, slots=True)
class PruneTarget:
    """A stale thread row whose rollout path cannot be recovered locally.

    Args:
        thread_id: Primary key from the Codex threads table.
        rollout_path: Missing path currently stored in the database.
        archived: Whether the thread is archived in Codex state.
    """

    thread_id: str
    rollout_path: Path
    archived: bool


@dataclass(frozen=True, slots=True)
class InventoryPlan:
    """Computed repair and prune targets for one database snapshot.

    Args:
        repair_targets: Threads that can be repointed to existing .jsonl.zst files.
        prune_targets: Threads whose rollout paths are missing and unrecoverable.
        existing_threads: Threads whose recorded rollout path already exists.
    """

    repair_targets: tuple[RepairTarget, ...]
    prune_targets: tuple[PruneTarget, ...]
    existing_threads: int


@dataclass(frozen=True, slots=True)
class Arguments:
    """Validated command-line arguments.

    Args:
        db_path: Codex state database path.
        mode: Which remediation subset to run.
        apply_changes: Whether to modify the database.
        skip_open_check: Whether to skip lsof-based database-open checks.
        sample_limit: Maximum sample rows shown for repair and prune groups.
    """

    db_path: Path
    mode: RepairMode
    apply_changes: bool
    skip_open_check: bool
    sample_limit: int


@dataclass(frozen=True, slots=True)
class WriteResult:
    """Database write summary.

    Args:
        repaired_threads: Number of threads whose rollout path was updated.
        pruned_threads: Number of threads deleted from the threads table.
        deleted_dynamic_tools: Rows deleted from thread_dynamic_tools.
        deleted_spawn_edges: Rows deleted from thread_spawn_edges.
        cleared_agent_items: Rows updated in agent_job_items.
    """

    repaired_threads: int = 0
    pruned_threads: int = 0
    deleted_dynamic_tools: int = 0
    deleted_spawn_edges: int = 0
    cleared_agent_items: int = 0


def main(argv: Sequence[str] | None = None) -> int:
    """Run the Codex rollout inventory repair tool.

    Args:
        argv: Optional argument vector for tests or embedded invocation.

    Returns:
        Process exit code.
    """

    console = Console()
    args = parse_args(argv)

    try:
        db_path = resolve_db_path(args.db_path)
        if args.apply_changes and not args.skip_open_check:
            ensure_database_not_open_elsewhere(db_path)

        with connect_database(db_path, read_only=not args.apply_changes) as connection:
            plan = build_inventory_plan(connection)
            render_plan(console, db_path, args, plan)

            if not args.apply_changes:
                render_dry_run_notice(console)
                return 0

            result = apply_plan(connection, plan, args.mode)
            verify_integrity(connection)
            render_write_result(console, result)
    except (OSError, RuntimeError, sqlite3.Error, subprocess.SubprocessError) as error:
        console.print(f"[red]error:[/red] {error}", highlight=False)
        return 1

    return 0


def parse_args(argv: Sequence[str] | None) -> Arguments:
    """Parse and validate command-line arguments.

    Args:
        argv: Optional argument vector. Uses sys.argv when omitted.

    Returns:
        Validated arguments.

    Raises:
        SystemExit: If argparse detects invalid command-line input.
    """

    parser = argparse.ArgumentParser(
        description=(
            "Repair Codex thread rollout_path values that point at missing "
            ".jsonl files when the corresponding .jsonl.zst exists, and "
            "optionally prune unrecoverable stale thread rows."
        )
    )
    parser.add_argument(
        "--db",
        default=str(DEFAULT_STATE_DB),
        help=f"Codex state sqlite DB path (default: {DEFAULT_STATE_DB})",
    )
    parser.add_argument(
        "--mode",
        choices=tuple(mode.value for mode in RepairMode),
        default=RepairMode.ALL.value,
        help="Remediation mode: all, repair, or prune (default: all)",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        dest="apply_changes",
        help="Modify the database. Without this flag the script is read-only.",
    )
    parser.add_argument(
        "--skip-open-check",
        action="store_true",
        help="Skip write-mode lsof check for other processes holding the DB open.",
    )
    parser.add_argument(
        "--sample-limit",
        type=positive_int,
        default=10,
        help="Number of sample targets to print per group (default: 10)",
    )
    namespace = parser.parse_args(argv)

    return Arguments(
        db_path=Path(namespace.db).expanduser(),
        mode=RepairMode(namespace.mode),
        apply_changes=bool(namespace.apply_changes),
        skip_open_check=bool(namespace.skip_open_check),
        sample_limit=int(namespace.sample_limit),
    )


def positive_int(value: str) -> int:
    """Parse a positive integer for argparse.

    Args:
        value: User-provided argument value.

    Returns:
        Parsed integer.

    Raises:
        argparse.ArgumentTypeError: If value is not a positive integer.
    """

    try:
        parsed = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error

    if parsed < 1:
        raise argparse.ArgumentTypeError("must be greater than zero")
    return parsed


def resolve_db_path(db_path: Path) -> Path:
    """Resolve and validate the SQLite database path.

    Args:
        db_path: User-provided database path.

    Returns:
        Resolved database path.

    Raises:
        FileNotFoundError: If the database does not exist.
    """

    resolved = db_path.expanduser().resolve()
    if not resolved.is_file():
        raise FileNotFoundError(f"database not found: {resolved}")
    return resolved


def ensure_database_not_open_elsewhere(db_path: Path) -> None:
    """Abort when another process appears to hold the database open.

    Args:
        db_path: Resolved database path.

    Raises:
        RuntimeError: If lsof reports open handles for the database.
    """

    if not command_exists("lsof"):
        return

    completed = subprocess.run(
        ["lsof", "-nP", "--", str(db_path)],
        check=False,
        capture_output=True,
        text=True,
    )
    if completed.returncode == 1 and not completed.stdout:
        return
    if completed.returncode not in {0, 1}:
        raise RuntimeError(
            "lsof failed while checking database handles: "
            f"{completed.stderr.strip() or completed.stdout.strip()}"
        )

    open_handles = strip_lsof_header(completed.stdout)
    if open_handles:
        raise RuntimeError(
            "database appears to be open by another process; stop Codex or "
            "rerun with --skip-open-check if you intentionally accept this "
            f"risk:\n{open_handles}"
        )


def command_exists(command: str) -> bool:
    """Return whether a command exists on PATH.

    Args:
        command: Executable name.

    Returns:
        True when the command can be found.
    """

    return shutil.which(command) is not None


def strip_lsof_header(output: str) -> str:
    """Remove the lsof header line from process output.

    Args:
        output: Raw lsof stdout.

    Returns:
        Non-header output lines joined by newlines.
    """

    lines = [line for line in output.splitlines() if line.strip()]
    if not lines:
        return ""
    if lines[0].startswith("COMMAND"):
        lines = lines[1:]
    return "\n".join(lines)


def connect_database(db_path: Path, *, read_only: bool) -> sqlite3.Connection:
    """Open the Codex state database.

    Args:
        db_path: Resolved database path.
        read_only: Whether to open SQLite with mode=ro.

    Returns:
        SQLite connection with row access by column name.
    """

    if read_only:
        quoted_path = quote(str(db_path), safe="/")
        connection = sqlite3.connect(f"file:{quoted_path}?mode=ro", uri=True)
    else:
        connection = sqlite3.connect(str(db_path))
    connection.row_factory = sqlite3.Row
    connection.execute("PRAGMA foreign_keys = ON")
    return connection


def build_inventory_plan(connection: sqlite3.Connection) -> InventoryPlan:
    """Classify threads by rollout path state.

    Args:
        connection: Open SQLite connection.

    Returns:
        Inventory plan with repairable and pruneable targets.
    """

    rows = load_threads(connection)
    repair_targets: list[RepairTarget] = []
    prune_targets: list[PruneTarget] = []
    existing_threads = 0

    for row in rows:
        if row.rollout_path.exists():
            existing_threads += 1
            continue

        compressed_path = compressed_rollout_path(row.rollout_path)
        if is_plain_jsonl(row.rollout_path) and compressed_path.exists():
            repair_targets.append(
                RepairTarget(
                    thread_id=row.thread_id,
                    old_path=row.rollout_path,
                    new_path=compressed_path,
                )
            )
            continue

        prune_targets.append(
            PruneTarget(
                thread_id=row.thread_id,
                rollout_path=row.rollout_path,
                archived=row.archived,
            )
        )

    return InventoryPlan(
        repair_targets=tuple(repair_targets),
        prune_targets=tuple(prune_targets),
        existing_threads=existing_threads,
    )


def load_threads(connection: sqlite3.Connection) -> tuple[ThreadRow, ...]:
    """Load thread rollout inventory from the state database.

    Args:
        connection: Open SQLite connection.

    Returns:
        Thread rows relevant to rollout repair.
    """

    cursor = connection.execute(
        f"""
        SELECT id, rollout_path, archived
        FROM {TABLE_THREADS}
        ORDER BY id
        """
    )
    return tuple(
        ThreadRow(
            thread_id=str(row["id"]),
            rollout_path=Path(str(row["rollout_path"])).expanduser(),
            archived=bool(row["archived"]),
        )
        for row in cursor
    )


def compressed_rollout_path(path: Path) -> Path:
    """Return the compressed rollout counterpart path.

    Args:
        path: Original rollout path.

    Returns:
        Path with a .zst suffix appended.
    """

    return Path(f"{path}.zst")


def is_plain_jsonl(path: Path) -> bool:
    """Return whether path refers to an uncompressed JSONL rollout.

    Args:
        path: Rollout path.

    Returns:
        True for .jsonl paths that are not already .jsonl.zst paths.
    """

    suffixes = path.suffixes
    return suffixes[-1:] == [".jsonl"]


def render_plan(
    console: Console,
    db_path: Path,
    args: Arguments,
    plan: InventoryPlan,
) -> None:
    """Print the computed inventory plan.

    Args:
        console: Rich console.
        db_path: Resolved database path.
        args: Validated command-line arguments.
        plan: Computed inventory plan.
    """

    summary = Table(title="Codex rollout inventory plan")
    summary.add_column("item")
    summary.add_column("count", justify="right")
    summary.add_row("database", str(db_path))
    summary.add_row("mode", args.mode.value)
    summary.add_row("existing rollout paths", str(plan.existing_threads))
    summary.add_row(
        "repairable .jsonl -> .jsonl.zst rows", str(len(plan.repair_targets))
    )
    summary.add_row("unrecoverable missing rows", str(len(plan.prune_targets)))
    summary.add_row("write mode", "apply" if args.apply_changes else "dry-run")
    console.print(summary)

    if args.mode in {RepairMode.ALL, RepairMode.REPAIR}:
        render_repair_samples(console, plan.repair_targets, args.sample_limit)
    if args.mode in {RepairMode.ALL, RepairMode.PRUNE}:
        render_prune_samples(console, plan.prune_targets, args.sample_limit)


def render_repair_samples(
    console: Console,
    targets: Sequence[RepairTarget],
    limit: int,
) -> None:
    """Print sample repair targets.

    Args:
        console: Rich console.
        targets: Repair targets.
        limit: Maximum number of samples.
    """

    if not targets:
        return

    table = Table(title="sample repair targets")
    table.add_column("thread_id")
    table.add_column("old_path")
    table.add_column("new_path")
    for target in targets[:limit]:
        table.add_row(target.thread_id, str(target.old_path), str(target.new_path))
    console.print(table)


def render_prune_samples(
    console: Console,
    targets: Sequence[PruneTarget],
    limit: int,
) -> None:
    """Print sample prune targets.

    Args:
        console: Rich console.
        targets: Prune targets.
        limit: Maximum number of samples.
    """

    if not targets:
        return

    table = Table(title="sample prune targets")
    table.add_column("thread_id")
    table.add_column("archived")
    table.add_column("rollout_path")
    for target in targets[:limit]:
        table.add_row(target.thread_id, str(target.archived), str(target.rollout_path))
    console.print(table)


def render_dry_run_notice(console: Console) -> None:
    """Print dry-run guidance.

    Args:
        console: Rich console.
    """

    console.print(
        "[yellow]dry-run only[/yellow]: rerun with --apply to modify the database.",
        highlight=False,
    )


def apply_plan(
    connection: sqlite3.Connection,
    plan: InventoryPlan,
    mode: RepairMode,
) -> WriteResult:
    """Apply selected repair and prune operations in one transaction.

    Args:
        connection: Open SQLite connection.
        plan: Computed inventory plan.
        mode: Selected remediation mode.

    Returns:
        Database write summary.
    """

    with connection:
        repaired_threads = (
            apply_repairs(connection, plan.repair_targets)
            if mode in {RepairMode.ALL, RepairMode.REPAIR}
            else 0
        )
        prune_result = (
            apply_prunes(connection, plan.prune_targets)
            if mode in {RepairMode.ALL, RepairMode.PRUNE}
            else WriteResult()
        )

    return WriteResult(
        repaired_threads=repaired_threads,
        pruned_threads=prune_result.pruned_threads,
        deleted_dynamic_tools=prune_result.deleted_dynamic_tools,
        deleted_spawn_edges=prune_result.deleted_spawn_edges,
        cleared_agent_items=prune_result.cleared_agent_items,
    )


def apply_repairs(
    connection: sqlite3.Connection,
    targets: Sequence[RepairTarget],
) -> int:
    """Update rollout_path values for repairable targets.

    Args:
        connection: Open SQLite connection.
        targets: Repair targets.

    Returns:
        Number of updated thread rows.
    """

    if not targets:
        return 0

    cursor = connection.executemany(
        f"""
        UPDATE {TABLE_THREADS}
        SET rollout_path = ?
        WHERE id = ? AND rollout_path = ?
        """,
        (
            (str(target.new_path), target.thread_id, str(target.old_path))
            for target in targets
        ),
    )
    return cursor.rowcount if cursor.rowcount >= 0 else len(targets)


def apply_prunes(
    connection: sqlite3.Connection,
    targets: Sequence[PruneTarget],
) -> WriteResult:
    """Delete unrecoverable stale thread rows and dependent references.

    Args:
        connection: Open SQLite connection.
        targets: Prune targets.

    Returns:
        Database write summary for prune operations.
    """

    if not targets:
        return WriteResult()

    target_ids = tuple(target.thread_id for target in targets)
    deleted_spawn_edges = execute_with_id_chunks(
        connection,
        f"""
        DELETE FROM {TABLE_THREAD_SPAWN_EDGES}
        WHERE parent_thread_id IN ({{placeholders}})
           OR child_thread_id IN ({{placeholders}})
        """,
        target_ids,
        repeat=2,
    )
    cleared_agent_items = execute_with_id_chunks(
        connection,
        f"""
        UPDATE {TABLE_AGENT_JOB_ITEMS}
        SET assigned_thread_id = NULL
        WHERE assigned_thread_id IN ({{placeholders}})
        """,
        target_ids,
    )
    deleted_dynamic_tools = execute_with_id_chunks(
        connection,
        f"""
        DELETE FROM {TABLE_THREAD_DYNAMIC_TOOLS}
        WHERE thread_id IN ({{placeholders}})
        """,
        target_ids,
    )
    pruned_threads = execute_with_id_chunks(
        connection,
        f"""
        DELETE FROM {TABLE_THREADS}
        WHERE id IN ({{placeholders}})
        """,
        target_ids,
    )

    return WriteResult(
        pruned_threads=pruned_threads,
        deleted_dynamic_tools=deleted_dynamic_tools,
        deleted_spawn_edges=deleted_spawn_edges,
        cleared_agent_items=cleared_agent_items,
    )


def execute_with_id_chunks(
    connection: sqlite3.Connection,
    sql_template: str,
    ids: Sequence[str],
    *,
    repeat: int = 1,
) -> int:
    """Execute a statement with safely chunked ID parameters.

    Args:
        connection: Open SQLite connection.
        sql_template: SQL containing a {placeholders} marker.
        ids: IDs to bind.
        repeat: Number of times to repeat each chunk's placeholders in params.

    Returns:
        Total changed rows reported by SQLite.
    """

    changed_rows = 0
    for chunk in chunked(ids, sqlite_chunk_size(repeat)):
        placeholders = ",".join("?" for _ in chunk)
        params = tuple(value for _ in range(repeat) for value in chunk)
        cursor = connection.execute(
            sql_template.format(placeholders=placeholders),
            params,
        )
        if cursor.rowcount > 0:
            changed_rows += cursor.rowcount
    return changed_rows


def sqlite_chunk_size(repeat: int) -> int:
    """Return a conservative parameter chunk size for SQLite statements.

    Args:
        repeat: Number of times IDs are repeated in one statement.

    Returns:
        Maximum IDs to include in one statement chunk.
    """

    sqlite_default_parameter_limit = 999
    return max(1, sqlite_default_parameter_limit // repeat)


def chunked(values: Sequence[str], size: int) -> Iterable[tuple[str, ...]]:
    """Yield fixed-size tuples from a sequence.

    Args:
        values: Values to chunk.
        size: Maximum chunk size.

    Yields:
        Tuples containing up to size values.
    """

    for index in range(0, len(values), size):
        yield tuple(values[index : index + size])


def verify_integrity(connection: sqlite3.Connection) -> None:
    """Verify the database after writes.

    Args:
        connection: Open SQLite connection.

    Raises:
        RuntimeError: If SQLite integrity checks fail.
    """

    integrity = connection.execute("PRAGMA integrity_check").fetchone()
    foreign_keys = connection.execute("PRAGMA foreign_key_check").fetchall()
    if integrity is None or str(integrity[0]) != "ok":
        raise RuntimeError(f"integrity_check failed: {integrity}")
    if foreign_keys:
        raise RuntimeError(f"foreign_key_check failed: {foreign_keys!r}")


def render_write_result(console: Console, result: WriteResult) -> None:
    """Print a write summary.

    Args:
        console: Rich console.
        result: Database write summary.
    """

    table = Table(title="applied changes")
    table.add_column("item")
    table.add_column("count", justify="right")
    table.add_row("repaired threads", str(result.repaired_threads))
    table.add_row("pruned threads", str(result.pruned_threads))
    table.add_row("deleted dynamic tool rows", str(result.deleted_dynamic_tools))
    table.add_row("deleted spawn edge rows", str(result.deleted_spawn_edges))
    table.add_row("cleared agent job item rows", str(result.cleared_agent_items))
    console.print(table)


if __name__ == "__main__":
    sys.exit(main())
