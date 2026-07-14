#!/usr/bin/env python3
"""Execute the schema repository's Grok drift engine without wrapping it."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
from typing import NoReturn, Sequence


ENGINE_PATH = Path("cmd/grok_schema_drift.py")
BASELINE_PATH = Path("grok.schema-drift-baselines.json")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Run the schema repository's hermetic Grok schema-drift check and "
            "preserve its process exit code."
        )
    )
    parser.add_argument(
        "--schema-repo",
        required=True,
        type=Path,
        help="schema repository containing the drift engine and baseline manifest",
    )
    parser.add_argument("--binary", required=True, type=Path, help="Grok binary")
    parser.add_argument("--baseline", required=True, help="baseline version")
    parser.add_argument(
        "--docs-root", required=True, type=Path, help="Grok documentation root"
    )
    parser.add_argument(
        "--evidence-root", required=True, type=Path, help="fresh evidence root"
    )
    parser.add_argument(
        "--json-output",
        type=Path,
        help="optional path for the schema engine's JSON result",
    )
    return parser


def resolve_engine(parser: argparse.ArgumentParser, repository: Path) -> Path:
    try:
        repository = repository.expanduser().resolve(strict=True)
    except OSError as error:
        parser.error(f"schema repository is unavailable: {error}")

    if not repository.is_dir():
        parser.error(f"schema repository is not a directory: {repository}")

    missing = [
        marker
        for marker in (ENGINE_PATH, BASELINE_PATH)
        if not (repository / marker).is_file()
    ]
    if missing:
        parser.error(
            "schema repository is missing required marker(s): "
            + ", ".join(str(marker) for marker in missing)
        )
    return repository / ENGINE_PATH


def exec_engine(arguments: Sequence[str] | None = None) -> NoReturn:
    parser = build_parser()
    options = parser.parse_args(arguments)
    engine = resolve_engine(parser, options.schema_repo)

    command = [
        sys.executable,
        str(engine),
        "check",
        "--binary",
        str(options.binary),
        "--baseline",
        options.baseline,
        "--docs-root",
        str(options.docs_root),
        "--evidence-root",
        str(options.evidence_root),
    ]
    if options.json_output is not None:
        command.extend(("--json-output", str(options.json_output)))

    os.execv(sys.executable, command)


if __name__ == "__main__":
    exec_engine()
