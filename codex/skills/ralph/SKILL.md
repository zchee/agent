---
name: ralph
description: Run Ralph-style bounded autonomous refinement for coding and repository tasks. Use when the user explicitly asks for Ralph, a Ralph loop, repeated self-correction, or wants Codex to keep iterating on one fixed objective until validation passes, a `--completion-promise` token is emitted, the user cancels, or a `--max-iterations` limit is reached.
---

# Ralph

Use this skill to translate Ralph into a Codex-native inner loop. Preserve Ralph's core discipline:

- keep one fixed objective,
- prefer current workspace state over prior chat reasoning,
- validate after every change,
- stop only on a real exit boundary.

## Upstream Mapping

The source Ralph implementation relies on cross-turn machinery that Codex does not have in a normal skill:

- `setup.sh` writes `.codex/ralph/state.json` with the original prompt, iteration count, and optional completion promise.
- An `AfterAgent` hook replays the original prompt, increments the iteration counter, and clears conversation context between turns.
- The hook stops on a completion promise, an iteration cap, or a prompt mismatch that indicates the user moved on.
- `/ralph:cancel` removes the state file and stops the loop.

In Codex, emulate the behavior inside the current turn. Do not recreate extension files or hooks unless the user explicitly asks for harness-level tooling.

## What Carries Over

- Ralph is repeated improvement on the same task, not one large speculative pass.
- Each iteration should start from the repository's current state.
- `--max-iterations <N>` is a hard safety cap. Default to `5` if the user omits it.
- `--completion-promise <TEXT>` means emit exactly `<promise>TEXT</promise>` once, and only when the task is actually complete.
- If the user changes to a different task, the Ralph loop no longer owns the turn.

## What Does Not Carry Over

- Do not create `.codex/ralph/state.json`.
- Do not implement `/ralph:loop`, `/ralph:cancel`, `/ralph:help`, hook files, or extension manifests unless the user explicitly asks for harness-level extension work.
- There is no automatic re-entry across turns in Codex. Run the iterations yourself inside the current task.
- There is no hook enforcing continuation. The loop discipline must come from your own planning, validation, and reflection.

## Input Contract

- Parse only these Ralph-style control flags from the user request:
  - `--max-iterations <N>`
  - `--completion-promise <TEXT>`
- Treat the remaining text as the fixed objective.
- Do not silently rename, reorder, or invent controls.
- If the user includes unknown flags, call that out and treat them as literal task text unless their meaning is explicit from context.
- Freeze the objective at the start of the loop. Do not keep redefining "done" midstream.

## Loop Setup

1. Extract the fixed objective and loop controls.
2. Define completion before editing.
3. If the user did not define "done", infer conservative, testable completion criteria and state them in a short progress update.
4. Identify the narrowest validation command that can prove progress.
5. Record the iteration budget in the active plan so the loop stays explicit.

## Iteration Protocol

For each iteration from `1` to `max_iterations`:

1. Rebuild context from the source of truth.
   - Re-read the relevant files, diagnostics, tests, and repository state.
   - Distrust stale chat context. Ralph works because file state persists while conversational memory is disposable; simulate that by checking the workspace again each pass.
2. Choose one high-leverage step.
   - Make the smallest change that materially improves the task.
   - Avoid unrelated cleanup, opportunistic refactors, or broad rewrites unless validation forces them.
3. Implement the change.
4. Validate immediately.
   - Run the narrowest command that proves or disproves the latest step.
   - Escalate to broader validation only when the focused check passes or is insufficient.
5. Reflect before the next pass.
   - If validation passed and the full completion criteria are satisfied, stop.
   - If validation failed, diagnose the exact failure and use it to plan the next iteration.
   - If the remaining work cannot fit in the loop budget, stop and report that directly.

## Stop Conditions

Stop the loop when any of these becomes true:

- The completion criteria are satisfied.
- The exact promise token has been earned and can be emitted.
- `max_iterations` is exhausted.
- The user explicitly cancels, aborts, or switches to a different task.
- A blocker requires user input, permissions, or a decision that cannot be inferred safely.

## Cancel Semantics

The original Ralph flow has `/ralph:cancel`. In Codex, interpret cancellation behaviorally:

- If the user says `cancel`, `stop`, or redirects the task, stop iterating immediately.
- Report the current state, the last validation result, and the next highest-leverage step.
- Do not leave fake Ralph state files behind.

## Ghost Protection

The original Ralph flow prevents an old loop from hijacking a new prompt. Mirror that rule:

- If the user introduces a new objective, abandon the old Ralph loop framing instead of dragging prior completion criteria forward.
- Never emit a stale completion promise for a superseded objective.

## Prompting Guidance

Ralph works best when the prompt includes:

- a concrete deliverable,
- objective validation,
- a bounded iteration cap,
- an explicit promise token when the user needs strict completion signaling.

Use or accept prompts like these:

- `$ralph fix the failing parser tests and keep iterating until the targeted package passes --max-iterations 6 --completion-promise TESTS_GREEN`
- `$ralph implement the missing pagination handling, validate it, and stop only when the API client tests pass --max-iterations 8`
- `$ralph inspect this refactor, patch the highest-leverage bug each pass, and stop when the repository is stable`

## Quality Bar

- Prefer deterministic evidence: targeted tests, diagnostics, reproducible commands, concrete diffs.
- Prefer several short corrective passes over one speculative rewrite.
- Do not claim success because the approach seems right.
- If `--completion-promise` is set, emit the exact token once in the final response and nowhere else.
- If the loop stops incomplete, report the best current state, the last failed check, and the next step.
