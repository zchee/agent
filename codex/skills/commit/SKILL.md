---
name: commit
description: Split the current git worktree into one or more meaningful commits with clean staging boundaries and commit hygiene. Use when the user asks to commit current changes, wants related changes grouped into multiple commits, or needs help preparing commit messages without disturbing unrelated local modifications.
---

# Commit

Use this skill to turn the current worktree into reviewable git commits.

## Workflow

1. Inspect `git status --short`, the relevant diffs, and repository instructions before staging anything.
2. Partition the worktree by concern.
   - Keep unrelated user changes unstaged and untouched.
   - Prefer the smallest coherent commit that still preserves behavior.
3. Stage only the files or hunks that belong together.
4. Review `git diff --cached --stat` and `git diff --cached` before each commit.
5. Write a meaningful commit message from the staged diff instead of using generic phrasing.
6. Commit with any repository-required flags or trailers.
7. Re-check `git status --short` after each commit and repeat until the requested work is committed.

## Rules

- Never revert, discard, or absorb unrelated local changes just to make the worktree look clean.
- If staged and unstaged hunks coexist in one file, verify that the split still leaves both states coherent.
- Run the narrowest useful validation before committing when a fast, directly relevant check exists.
- If the user did not provide commit-message text, derive it from the staged content and the repository's message conventions.
- If a required precondition is missing, say so instead of forcing a partial or misleading commit.

## Output

- Report the commits created, in order.
- Note any remaining unstaged or uncommitted changes.
- Call out skipped validation or blockers explicitly.
