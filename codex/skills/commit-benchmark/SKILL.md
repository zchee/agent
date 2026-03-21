---
name: commit-benchmark
description: Split work into meaningful commits and attach comparable benchmark evidence for performance-sensitive changes, especially Go code with `Benchmark...` tests. Use when the user asks for benchmark-backed commits, the diff affects benchmarked code, or relevant commit messages should include raw performance comparison output.
---

# Commit Benchmark

Use this skill to create clean commits without losing the performance evidence that justifies them.

## Workflow

1. Inspect `git status --short` and the relevant diffs to find which changes affect performance-sensitive code or existing benchmark targets.
2. Determine the concrete benchmark names and commands that match the repository's language and harness.
3. Capture a comparable baseline before the final optimized state.
   - Use the same benchmark names, parameters, and environment for every comparison.
   - For Go, prefer `go test -run='^$' ... -bench ... -benchmem`.
   - For Rust, prefer the repository's `cargo bench` and `criterion` flow.
4. Run the current benchmark after the change and compare baseline vs current with `benchstat` or the repository-standard comparator.
5. If results regress, re-evaluate the approach before committing.
6. Split commits by concern and keep benchmark evidence attached to the commit or commits that need it.
7. Include the raw comparison output in the relevant commit message when the workflow or repository expects it.
8. Re-check the worktree after each commit.

## Evidence Rules

- Do not compare different benchmark names or mismatched parameters.
- Prefer repeated samples over one noisy run so the comparison is worth trusting.
- Preserve the raw benchmark outputs until the commit message and final summary are complete.
- If no reliable benchmark exists, say that plainly and fall back to normal commit discipline instead of fabricating evidence.

## Output

- Report the baseline and current commands used.
- Summarize the comparison result and whether it justified the commit.
- List the commits created and any remaining worktree state.
