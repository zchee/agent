from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
SKILL_ROOT = REPOSITORY_ROOT / "codex" / "skills" / "grok-schema-drift"
LAUNCHER = SKILL_ROOT / "scripts" / "run-grok-schema-drift.py"


class GrokSchemaDriftLauncherTest(unittest.TestCase):
    def create_schema_repository(self, root: Path, exit_code: int = 0) -> Path:
        repository = root / "schema"
        engine = repository / "cmd" / "grok_schema_drift.py"
        engine.parent.mkdir(parents=True)
        engine.write_text(
            textwrap.dedent(
                f"""\
                from __future__ import annotations

                import json
                from pathlib import Path
                import sys

                output = Path(sys.argv[sys.argv.index("--json-output") + 1])
                output.write_text(json.dumps(sys.argv[1:]), encoding="utf-8")
                raise SystemExit({exit_code})
                """
            ),
            encoding="utf-8",
        )
        (repository / "grok.schema-drift-baselines.json").write_text(
            "{}\n", encoding="utf-8"
        )
        return repository

    def launcher_command(self, repository: Path, output: Path) -> list[str]:
        return [
            sys.executable,
            str(LAUNCHER),
            "--schema-repo",
            str(repository),
            "--binary",
            "relative/fake-grok",
            "--baseline",
            "0.2.101",
            "--docs-root",
            "relative/docs",
            "--evidence-root",
            "relative/evidence",
            "--json-output",
            str(output),
        ]

    def test_forwards_explicit_arguments_from_foreign_working_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            repository = self.create_schema_repository(root)
            output = root / "forwarded.json"
            foreign_cwd = root / "foreign"
            foreign_cwd.mkdir()

            result = subprocess.run(
                self.launcher_command(repository, output),
                cwd=foreign_cwd,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(
                json.loads(output.read_text(encoding="utf-8")),
                [
                    "check",
                    "--binary",
                    "relative/fake-grok",
                    "--baseline",
                    "0.2.101",
                    "--docs-root",
                    "relative/docs",
                    "--evidence-root",
                    "relative/evidence",
                    "--json-output",
                    str(output),
                ],
            )

    def test_preserves_schema_engine_exit_code(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            repository = self.create_schema_repository(root, exit_code=20)
            output = root / "forwarded.json"

            result = subprocess.run(
                self.launcher_command(repository, output),
                cwd=root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertEqual(result.returncode, 20, result.stderr)
            self.assertTrue(output.is_file())

    def test_rejects_repository_missing_required_marker(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            repository = root / "schema"
            (repository / "cmd").mkdir(parents=True)
            (repository / "cmd" / "grok_schema_drift.py").write_text(
                "raise AssertionError('must not execute')\n", encoding="utf-8"
            )
            output = root / "not-created.json"

            result = subprocess.run(
                self.launcher_command(repository, output),
                cwd=root,
                check=False,
                capture_output=True,
                text=True,
            )

            self.assertEqual(result.returncode, 2)
            self.assertIn("grok.schema-drift-baselines.json", result.stderr)
            self.assertFalse(output.exists())

    def test_launcher_has_no_process_spawning_or_installer_path(self) -> None:
        source = LAUNCHER.read_text(encoding="utf-8")

        self.assertIn("os.execv", source)
        self.assertNotIn("subprocess", source)
        self.assertNotIn("install-dep.sh", source)
        self.assertNotIn("/Users/", source)


if __name__ == "__main__":
    unittest.main()
