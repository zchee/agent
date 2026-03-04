# Codex ExecPlan References

Primary sources:

- Human-readable article:
  https://developers.openai.com/cookbook/articles/codex_exec_plans/
- Raw markdown source:
  https://raw.githubusercontent.com/openai/openai-cookbook/main/articles/codex_exec_plans.md

Use these sources when you need authoritative wording or full context.

## Core Requirements (Condensed)

- Treat an ExecPlan as a living, self-contained implementation specification.
- Assume the executor has only the current repository and this single plan file.
- Explain user-visible purpose before implementation details.
- Include explicit repository paths, commands, and expected outcomes.
- Keep Progress, Surprises & Discoveries, Decision Log, and Outcomes & Retrospective continuously updated.
- Capture key decisions with rationale and timestamp/author context.
- Prefer observable validation over internal implementation assertions.
- Write retry/recovery guidance for risky or partially-failing steps.
- Keep milestones independently verifiable and outcome-focused.
- When revising, update all affected sections and append a change note.

## Recommended Section Envelope

Use these sections in this order unless repository conventions require extensions:

1. Purpose / Big Picture
2. Progress
3. Surprises & Discoveries
4. Decision Log
5. Outcomes & Retrospective
6. Context and Orientation
7. Plan of Work
8. Concrete Steps
9. Validation and Acceptance
10. Idempotence and Recovery
11. Artifacts and Notes
12. Interfaces and Dependencies

## Progress Entry Format

Use timestamped checkboxes with UTC suffix:

- [x] (YYYY-MM-DD HH:MMZ) Completed item.
- [ ] Pending item.
- [ ] Partial item (completed: X; remaining: Y).

Update this section at every stopping point.

## Validation Expectations

Define commands and expected evidence clearly:

- Command with explicit working directory.
- Output or behavior that proves success.
- Negative or rollback expectations when relevant.
- Test naming or scenario details that distinguish before-vs-after behavior.

## Revision Checklist

Before finalizing any ExecPlan revision, verify:

- Purpose still matches current implementation intent.
- Progress reflects actual state without stale entries.
- Decision log includes new or reversed decisions.
- Validation remains executable in the current repo state.
- Recovery instructions still match latest steps.
- Bottom-of-file revision note explains what changed and why.
