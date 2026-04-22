from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType
from typing import Any


SCRIPT_PATH = Path(__file__).resolve().parents[1] / "scripts" / "history-compact.py"


def load_history_compact_module() -> ModuleType:
    spec = importlib.util.spec_from_file_location("history_compact", SCRIPT_PATH)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


history_compact = load_history_compact_module()


def empty_plan() -> Any:
    return history_compact.CompactPlan(
        keep_line_numbers=frozenset(),
        stats=history_compact.CompactStats(
            total_records=0,
            dedup_candidates=0,
            unique_texts=0,
            removed_records=0,
            kept_records=0,
            non_text_records=0,
            text_without_valid_timestamp=0,
        ),
    )


def test_resolve_path_argument_expands_env_and_user_home(
    monkeypatch: Any, tmp_path: Path
) -> None:
    codex_home = tmp_path / "custom-codex-home"
    monkeypatch.setenv("CODEX_HOME", str(codex_home))

    resolved = history_compact.resolve_path_argument("$CODEX_HOME/history.jsonl")

    assert resolved == codex_home / "history.jsonl"
    assert history_compact.resolve_path_argument("~/history.jsonl") == (
        Path.home() / "history.jsonl"
    )


def test_default_history_path_uses_codex_home_when_set(
    monkeypatch: Any, tmp_path: Path
) -> None:
    codex_home = tmp_path / "project-codex-home"
    monkeypatch.setenv("CODEX_HOME", str(codex_home))

    assert history_compact.default_history_path() == codex_home / "history.jsonl"
    assert history_compact.parse_args([]).input_path == str(
        codex_home / "history.jsonl"
    )


def test_default_history_path_falls_back_when_codex_home_blank(
    monkeypatch: Any,
) -> None:
    monkeypatch.setenv("CODEX_HOME", "   ")

    assert history_compact.default_history_path() == (
        Path("~/.codex/history.jsonl").expanduser()
    )
    assert history_compact.parse_args([]).input_path == str(
        Path("~/.codex/history.jsonl").expanduser()
    )


def test_main_expands_env_vars_for_input_and_output_paths(
    monkeypatch: Any, tmp_path: Path
) -> None:
    codex_home = tmp_path / "explicit-codex-home"
    monkeypatch.setenv("CODEX_HOME", str(codex_home))
    captured: dict[str, Any] = {}

    def fake_compact_history(
        input_path: Path,
        output_path: Path | None = None,
        *,
        write_stdout: bool = False,
        dry_run: bool = False,
    ) -> Any:
        captured["input_path"] = input_path
        captured["output_path"] = output_path
        captured["write_stdout"] = write_stdout
        captured["dry_run"] = dry_run
        return empty_plan()

    monkeypatch.setattr(history_compact, "compact_history", fake_compact_history)

    exit_code = history_compact.main(
        [
            "$CODEX_HOME/history.jsonl",
            "--output",
            "$CODEX_HOME/compacted/history.jsonl",
            "--dry-run",
        ]
    )

    assert exit_code == 0
    assert captured == {
        "input_path": codex_home / "history.jsonl",
        "output_path": codex_home / "compacted" / "history.jsonl",
        "write_stdout": False,
        "dry_run": True,
    }
