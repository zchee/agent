#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.14"
# dependencies = [
#   "orjson",
# ]
# ///
"""Compact Codex history JSONL by sorting and filtering text entries."""

from __future__ import annotations

import argparse
import math
import os
import sys
import tempfile
from collections.abc import Iterator, Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO, Final

import orjson

DEFAULT_CODEX_HOME: Final[str] = "~/.codex"
REMOVED_TEXT_MARKERS: Final[tuple[str, ...]] = (
    "OMX_TMUX_INJECT",
    "OMX_TEAM_STATE_ROOT",
    "/.omx/",
    "omx question answered",
)


class HistoryCompactError(RuntimeError):
    """Raised when the history file cannot be compacted safely."""


@dataclass(frozen=True, slots=True)
class HistoryLine:
    line_number: int
    raw_line: bytes
    text: str | None
    timestamp: int | float | None


@dataclass(frozen=True, slots=True)
class CompactStats:
    total_records: int
    dedup_candidates: int
    unique_texts: int
    removed_records: int
    kept_records: int
    non_text_records: int
    text_without_valid_timestamp: int
    removed_text_records: int


@dataclass(frozen=True, slots=True)
class CompactPlan:
    keep_line_numbers: frozenset[int]
    remove_line_numbers: frozenset[int]
    is_sorted: bool
    stats: CompactStats


def resolve_path_argument(value: str) -> Path:
    expanded_value = os.path.expandvars(value)
    expanded_value = os.path.expanduser(expanded_value)
    return Path(expanded_value)


def default_history_path() -> Path:
    configured_codex_home = os.environ.get("CODEX_HOME", "").strip()
    codex_home = configured_codex_home or DEFAULT_CODEX_HOME
    return resolve_path_argument(codex_home) / "history.jsonl"


def should_remove_text(text: str) -> bool:
    return any(marker in text for marker in REMOVED_TEXT_MARKERS)


def _normalize_timestamp(value: object) -> int | float | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return value if math.isfinite(value) else None
    if isinstance(value, str):
        stripped = value.strip()
        if not stripped:
            return None
        try:
            if any(marker in stripped for marker in (".", "e", "E")):
                parsed = float(stripped)
                return parsed if math.isfinite(parsed) else None
            return int(stripped, 10)
        except ValueError:
            return None
    return None


def _iter_history_lines(path: Path) -> Iterator[HistoryLine]:
    with path.open("rb") as history_file:
        for line_number, raw_line in enumerate(history_file, start=1):
            try:
                record = orjson.loads(raw_line)
            except orjson.JSONDecodeError as exc:
                raise HistoryCompactError(
                    f"{path}:{line_number}: invalid JSON: {exc}"
                ) from exc

            if not isinstance(record, dict):
                record_type = type(record).__name__
                raise HistoryCompactError(
                    f"{path}:{line_number}: expected a JSON object, got {record_type}"
                )

            text = record.get("text")
            normalized_text = text if isinstance(text, str) else None
            timestamp = _normalize_timestamp(record.get("ts"))
            yield HistoryLine(
                line_number=line_number,
                raw_line=raw_line,
                text=normalized_text,
                timestamp=timestamp,
            )


def build_compaction_plan(path: Path) -> CompactPlan:
    latest_by_text: dict[str, tuple[int | float, int]] = {}
    retained_candidates: list[tuple[int | float | None, int, bool]] = []
    total_records = 0
    dedup_candidates = 0
    non_text_records = 0
    text_without_valid_timestamp = 0
    remove_line_numbers: set[int] = set()

    for history_line in _iter_history_lines(path):
        total_records += 1

        if history_line.text is None:
            non_text_records += 1
            retained_candidates.append(
                (history_line.timestamp, history_line.line_number, False)
            )
            continue
        if should_remove_text(history_line.text):
            remove_line_numbers.add(history_line.line_number)
            continue
        if history_line.timestamp is None:
            text_without_valid_timestamp += 1
            retained_candidates.append((None, history_line.line_number, False))
            continue

        dedup_candidates += 1
        retained_candidates.append(
            (history_line.timestamp, history_line.line_number, True)
        )
        previous = latest_by_text.get(history_line.text)
        candidate = (history_line.timestamp, history_line.line_number)
        if previous is None or candidate > previous:
            latest_by_text[history_line.text] = candidate

    keep_line_numbers = frozenset(
        line_number for _, line_number in latest_by_text.values()
    )
    retained_line_order = tuple(
        (timestamp, line_number)
        for timestamp, line_number, requires_keep_match in retained_candidates
        if not requires_keep_match or line_number in keep_line_numbers
    )
    sorted_retained_candidates = sorted(
        retained_line_order,
        key=lambda candidate: _history_sort_key(candidate[0]),
    )
    is_sorted = list(retained_line_order) == sorted_retained_candidates
    unique_texts = len(latest_by_text)
    removed_text_records = len(remove_line_numbers)
    removed_records = dedup_candidates - unique_texts + removed_text_records
    kept_records = total_records - removed_records

    return CompactPlan(
        keep_line_numbers=keep_line_numbers,
        remove_line_numbers=frozenset(remove_line_numbers),
        is_sorted=is_sorted,
        stats=CompactStats(
            total_records=total_records,
            dedup_candidates=dedup_candidates,
            unique_texts=unique_texts,
            removed_records=removed_records,
            kept_records=kept_records,
            non_text_records=non_text_records,
            text_without_valid_timestamp=text_without_valid_timestamp,
            removed_text_records=removed_text_records,
        ),
    )


def _history_sort_key(timestamp: int | float | None) -> tuple[bool, int | float]:
    if timestamp is None:
        return (True, 0)
    return (False, timestamp)


def write_retained_lines(
    input_path: Path, destination: BinaryIO, plan: CompactPlan
) -> None:
    retained_lines: list[HistoryLine] = []
    for history_line in _iter_history_lines(input_path):
        should_keep = (
            history_line.line_number not in plan.remove_line_numbers
            and (
                history_line.text is None
                or history_line.timestamp is None
                or history_line.line_number in plan.keep_line_numbers
            )
        )
        if should_keep:
            retained_lines.append(history_line)

    retained_lines.sort(
        key=lambda history_line: _history_sort_key(history_line.timestamp)
    )
    for history_line in retained_lines:
        destination.write(history_line.raw_line)


def compact_history(
    input_path: Path,
    output_path: Path | None = None,
    *,
    write_stdout: bool = False,
    dry_run: bool = False,
) -> CompactPlan:
    if output_path is not None and write_stdout:
        raise ValueError("--output and --stdout are mutually exclusive")

    plan = build_compaction_plan(input_path)
    if dry_run:
        return plan

    if write_stdout:
        write_retained_lines(input_path, sys.stdout.buffer, plan)
        return plan

    destination_path = output_path or input_path
    if output_path is None and plan.stats.removed_records == 0 and plan.is_sorted:
        return plan

    destination_path.parent.mkdir(parents=True, exist_ok=True)
    mode_source = destination_path if destination_path.exists() else input_path
    destination_mode = mode_source.stat().st_mode & 0o777
    temp_path: Path | None = None

    try:
        with tempfile.NamedTemporaryFile(
            mode="wb",
            delete=False,
            dir=destination_path.parent,
            prefix=f".{destination_path.name}.",
            suffix=".tmp",
        ) as temp_file:
            temp_path = Path(temp_file.name)
            write_retained_lines(input_path, temp_file, plan)
        temp_path.chmod(destination_mode)
        temp_path.replace(destination_path)
    except Exception:
        if temp_path is not None:
            temp_path.unlink(missing_ok=True)
        raise

    return plan


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Compact Codex history JSONL by sorting top-level ts timestamps "
            "ascending, removing older duplicate top-level text entries, "
            "and removing known unsafe OMX injection text."
        )
    )
    parser.add_argument(
        "input_path",
        nargs="?",
        default=str(default_history_path()),
        help="History JSONL file to compact.",
    )
    destination_group = parser.add_mutually_exclusive_group()
    destination_group.add_argument(
        "-o",
        "--output",
        help="Write compacted JSONL to this path instead of replacing the input.",
    )
    destination_group.add_argument(
        "--stdout",
        action="store_true",
        help="Write compacted JSONL to stdout.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Analyze the file and print the summary without writing changes.",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    input_path = resolve_path_argument(args.input_path)
    output_path = resolve_path_argument(args.output) if args.output else None

    try:
        plan = compact_history(
            input_path=input_path,
            output_path=output_path,
            write_stdout=args.stdout,
            dry_run=args.dry_run,
        )
    except (FileNotFoundError, HistoryCompactError, OSError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    output_label = "stdout" if args.stdout else str(output_path or input_path)
    mode_label = "dry-run" if args.dry_run else "compacted"
    stats = plan.stats
    print(
        f"{mode_label}: input={input_path} output={output_label} "
        f"total={stats.total_records} kept={stats.kept_records} "
        f"removed={stats.removed_records} dedup_candidates={stats.dedup_candidates} "
        f"unique_texts={stats.unique_texts} non_text={stats.non_text_records} "
        "text_without_valid_ts="
        f"{stats.text_without_valid_timestamp} "
        f"removed_text={stats.removed_text_records}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
