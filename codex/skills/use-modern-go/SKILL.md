---
name: use-modern-go
description: Apply version-aware modern Go idioms and standard-library features that are supported by the target module's Go version. Use when writing, reviewing, refactoring, or modernizing Go code and the correct choice depends on whether the repository targets Go 1.18 through Go 1.26 features.
---

# Use Modern Go

Use this skill to modernize Go code without overshooting the Go version declared by the affected module.

## Workflow

1. Determine the target Go version before editing.
   - Read the nearest `go.mod` that governs the files you are changing.
   - If a change spans multiple modules, use the version of each owning module or constrain shared code to the lowest compatible version.
   - If no relevant `go.mod` exists, ask the user which Go version to target before introducing version-specific features.
2. State the chosen version once in a short progress update.
   - Example: `This module targets Go 1.24, so I will use modern patterns up to Go 1.24 and avoid newer APIs.`
   - Do not ask for confirmation when the version is already clear.
3. Load [references/version-matrix.md](references/version-matrix.md) when you need exact feature cutoffs or replacement patterns.
4. Apply features up to and including the target version. Never introduce syntax, APIs, or behavioral assumptions from a newer version.
5. Prefer targeted modernization.
   - Update touched code and nearby helpers when it improves clarity, correctness, or maintainability.
   - Avoid broad style-only rewrites unless the user explicitly asks for a sweep.
6. Preserve behavior.
   - Do not swap in a modern helper if it changes error identity, nil handling, ordering, allocation profile, or exported API semantics.
7. Prefer standard-library upgrades before third-party helpers when the standard library now covers the same use case.

## Decision Rules

- Prefer `any` over `interface{}` in new or edited code.
- Prefer `slices`, `maps`, and `cmp` helpers over manual loops when they are clearer and supported by the target version.
- Prefer `errors.Is`, `errors.Join`, cause-aware `context` APIs, and newer `sync` helpers when they fit the code path.
- Prefer version-appropriate test and benchmark APIs such as `t.Context()` and `b.Loop()` when the target version supports them.
- Prefer newer `net/http` ServeMux patterns only for modules targeting Go 1.22 or newer.
- When several modern options are valid, choose the one that best matches surrounding code and minimizes surprise for future readers.

## Version Selection Notes

```bash
grep -rh "^go " --include="go.mod" . 2>/dev/null | cut -d' ' -f2 | sort | uniq -c | sort -nr | head -1 | xargs | cut -d' ' -f2 | grep . || echo unknown

```

- Treat the `go` directive as the compatibility floor.
- Treat `toolchain` as a local tooling hint, not automatic permission to use language features newer than the `go` directive.
- If repository docs or CI enforce an older minimum version than the local toolchain, honor the older minimum.
- If the user explicitly asks to raise or lower the target version, say so in the summary and apply the requested constraint.

## Upstream Mapping

The upstream JetBrains Claude skill used inline shell execution to inject a detected Go version into the prompt. Codex skills do not have that mechanism, so perform the version lookup yourself as part of the workflow above and use the reference file for the feature matrix.

## Good Triggers

Use this skill for requests like:

- `modernize this Go package without requiring Go 1.25`
- `review whether this refactor uses APIs newer than our module allows`
- `replace legacy loops with standard-library helpers that fit Go 1.23`
- `write new Go code that matches current best practices for this repository's Go version`
