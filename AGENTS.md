# Repository Guidelines

## Project Structure & Module Organization
This repository stores configuration, prompts, and skills for multiple coding
agents rather than a single deployable application. Keep changes scoped to the
agent surface you are modifying: `agent/instructions/` holds shared guidance,
`codex/` contains Codex config, skills, scripts, and tests, `claude/` contains
Claude-specific commands, agents, plugins, and settings, `gemini/` holds Gemini
settings and command definitions, and `docs/` contains shared documentation such
as `docs/PROMPT.md`. Place new tests beside the feature they cover, for example
under `codex/tests/` for `codex/scripts/`.

## Tooling

- When running `./codex/skills/.system/skill-creator/scripts/quick_validate.py`,
  use `uv run --with PyYAML` so the script has the `yaml` dependency available.

## Build, Test, and Development Commands
There is no root build system; use the toolchain that matches the surface you
touch. Verified commands in this repo include:

- `uv run --script codex/scripts/history-compact.py --help` to inspect the
  current Codex utility CLI.
- `uv run --with pytest --with orjson python -m pytest codex/tests/test_history_compact.py -q`
  to run the tracked Python tests.
- `rg --threads=8 "pattern" codex claude agent docs` for repository-wide
  discovery; this is the expected ripgrep form in this repo.

## Coding Style & Naming Conventions
Follow the existing file conventions of each subtree instead of inventing new
structure. Markdown, TOML, JSON, and source files use LF endings
(`.gitattributes`). Python code in `codex/scripts/` uses four-space indentation,
type hints, and `from __future__ import annotations`; keep script filenames
kebab-case, such as `history-compact.py`. Use descriptive lowercase directory
names for agent surfaces and keep generated artifacts out of tracked paths.

## Testing Guidelines
Use `pytest` for Python coverage and keep tests close to the code under test.
Name files `test_*.py` and prefer behavior-focused test names such as
`test_compact_history_rewrites_in_place`. For CLI-oriented scripts, cover both
argument parsing and file rewrite behavior, not just happy-path output.

## Commit & Pull Request Guidelines
Recent history uses short, scoped, imperative subjects such as
`codex: update config.toml` and `docs: rename to PROMPT.md`. Follow the same
pattern: prefix with the affected area (`codex`, `claude`, `docs`, `agent`,
`gemini`) and keep each commit to one logical change. No PR template or GitHub
Actions workflow is tracked here, so include a brief description, touched paths,
and the exact validation commands you ran in every PR.

## Configuration & Hygiene
Do not commit local caches or editor output. `.gitignore` already excludes
`**/__pycache__/`, `claude/plugins/cache/`, `claude/plugins/marketplaces/`, and
`codex/skills/dist`; extend ignore rules only for project-generated files, not
personal editor state.
