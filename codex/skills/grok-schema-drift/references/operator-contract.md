# Grok schema drift operator contract

## Contents

- [Modes](#modes)
- [Exit codes](#exit-codes)
- [Evidence contract](#evidence-contract)
- [Drift review](#drift-review)
- [Rust reverse-engineering handoff](#rust-reverse-engineering-handoff)

## Modes

`validate-repository` is a read-only, clean-checkout-compatible gate. It checks
the baseline manifest, schemas, fixtures, coverage ledger, index, and Grok-only
catalog slice without executing Grok.

`check` adds target identity, sandbox proof, hermetic discovery, and two closure
passes. Run it locally or on the isolated macOS live runner. Never run a live
check when the repository gate or sandbox self-test has failed.

CI repository validation must use read-only permissions and no target binary.
CI live drift may use only a pre-provisioned binary on the labeled isolated
runner. Neither mode may update, commit, push, or open a pull request.

## Exit codes

| Code | Meaning | Required action |
| ---: | --- | --- |
| `0` | Selected binary matches the immutable baseline after two deterministic zero-diff passes. | Validate terminal status and worktree preservation. |
| `20` | Version or SHA drift was detected and a validated versioned report was written. | Stop automatic work and request human review. |
| `30` | Input, baseline, repository, or target identity is invalid. | Correct the input; do not execute the target. |
| `31` | A required pre-provisioned dependency is unavailable. | Record the missing dependency; do not install it. |
| `32` | Isolation, execution, determinism, report validation, cleanup, or protected-worktree preservation failed. | Quarantine the fresh run and investigate without weakening gates. |

Treat an undocumented code as a failure. Use `status.json`, written last, as
the terminal-state record rather than inferring success from partial files.

## Evidence contract

Evidence is scoped as:

```text
<evidence-root>/
`-- grok-<observed-version>/
    `-- drift/
        `-- <full-sha256>/
            `-- <run-id>/
                |-- identity.json
                |-- sandbox-self-test.json
                |-- pass-1/
                |-- pass-2/
                |-- drift-report.json
                |-- drift-report.md
                |-- evidence-manifest.json
                `-- status.json
```

Before accepting a result, require identity data for the launcher, resolved
target, SHA-256, canonical version/revision, platform/architecture, selected
baseline, sandbox-profile hash, and evidence directory. Require both closure
pass results, cleanup proof, zero prohibited-side-effect counters, a validated
manifest, and a terminal status. Stored output must be redacted and use
disposable-relative paths.

Never write fresh evidence into the immutable 0.2.101 G006 artifact tree.

## Drift review

For exit `20`, verify all of the following before handoff:

1. Classification distinguishes a new semantic version from the same version
   with a different SHA.
2. `automaticUpdatePerformed` is `false`.
3. Added, removed, changed, and unavailable sections are explicit.
4. JSON validates against the repository report schema and Markdown agrees.
5. The active baseline and every frozen corpus hash remain unchanged.
6. Both protected worktree snapshots match their pre-run content digests.
7. The next action calls for reviewed reverse engineering, not adoption.

## Rust reverse-engineering handoff

After explicit review, load the installed `rust-reverse-engineering` skill.
Confirm the existing `/opt/local/rust/cargo/bin/rustfilt`; run its dependency
checker and artifact collector with `--no-ghidra`. Do not run
`install-dep.sh`, package managers, download commands, or tool replacement.

Use the version/SHA-scoped drift bundle as input. A Ghidra investigation is a
separate reviewed phase and may use only an already-installed tool. It never
changes the active baseline automatically.
