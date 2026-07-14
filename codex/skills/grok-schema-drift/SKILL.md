---
name: grok-schema-drift
description: Verify a Grok binary against the versioned schema baseline with hermetic, read-only drift checks. Use for Grok schema drift checks, binary upgrades or replacement builds, next-version schema investigations, baseline verification, and interpretation of local or CI drift reports.
---

# Grok Schema Drift

Verify the selected Grok binary without updating schemas, fixtures, catalogs, or
baseline metadata. Treat the schema repository CLI as the only implementation
of validation and drift logic; this skill only supplies the safe operating
sequence and a thin launcher.

## Required inputs

Resolve these paths explicitly before running anything:

- the schema repository containing `cmd/grok_schema_drift.py` and
  `grok.schema-drift-baselines.json`;
- the Grok binary to inspect;
- the documentation root used by the selected baseline;
- a fresh evidence root outside immutable baseline artifacts.

Read the baseline manifest before selecting a baseline. Never infer the active
baseline from the binary's filename or symlink name.

## Safe workflow

Run the phases in this order and stop at the first failure:

1. Snapshot both the schema and skill worktrees, including content digests for
   every pre-existing dirty or untracked path.
2. Run the schema CLI's repository validation for the selected baseline.
3. Resolve the target and compute its SHA-256 before executing it.
4. Run the deny-network and real-config sandbox self-test.
5. Obtain the canonical version only through the hermetic runner.
6. Run two ordered closure passes in fresh disposable roots.
7. Interpret the process exit code and validate the resulting report.
8. Recompute both worktree snapshots and require every protected byte to match.

Use `scripts/run-grok-schema-drift.py` for the live `check` phase. Pass every
path explicitly; the launcher does not search user configuration or hard-code a
checkout:

```sh
python3 scripts/run-grok-schema-drift.py \
  --schema-repo /path/to/schema \
  --binary /path/to/grok \
  --baseline 0.2.101 \
  --docs-root /path/to/grok/docs/user-guide \
  --evidence-root /path/to/fresh/evidence
```

The launcher replaces itself with the schema CLI, so signals and process exit
codes are preserved. It never implements schema validation, binary discovery,
sandboxing, or report generation.

## Stop and handoff rules

- Never update the active baseline automatically.
- Exit `20` ends the automatic phase after a validated drift report. It does
  not authorize edits to schemas, fixtures, catalogs, or baseline metadata.
- Fail closed when identity, sandbox, dependency, determinism, report, cleanup,
  or worktree-preservation checks fail.
- Never install, upgrade, remove, or replace Grok, `rustfilt`, Ghidra, or any
  dependency.
- Never read real Grok settings, credentials, keychains, agent configuration,
  or unrelated project roots.

After a human reviews an exit-`20` report, use the installed
`rust-reverse-engineering` skill for the bounded follow-up. Run only its
dependency checker and artifact collector with `--no-ghidra`, verify the
existing `rustfilt` path, and never invoke `install-dep.sh`. A deeper Ghidra
pass requires a separate reviewed plan and an already-installed tool.

Read [references/operator-contract.md](references/operator-contract.md) before
running or interpreting a check. It defines exact exit meanings, evidence-tree
requirements, CI/local boundaries, and the drift-review checklist.
