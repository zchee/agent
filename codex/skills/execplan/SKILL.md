---
name: execplan
description: Create and maintain repository-local ExecPlan documents for complex features and long-horizon tasks. Trigger only when the user explicitly invokes /execplan, asks to create or revise an ExecPlan, asks for a PLANS.md-compliant execution plan, or requests a design-to-implementation plan that must include milestones, progress tracking, validation commands, decision logs, and recovery guidance.
---

# ExecPlan

Use this skill to produce or update a living ExecPlan document that a stateless contributor can execute end-to-end.

## Reference Loading

Load the canonical guidance before drafting or revising a plan:

- references/codex_exec_plans.md

Use the source URLs listed there when you need full upstream context.

## Workflow

1. Confirm the target plan file path from repository conventions.
2. Inspect repository instructions (AGENTS.md, .agents/plans/<descriptive-name>.md, and related docs).
3. Capture user-visible outcome first, then derive milestones and validation.
4. Write or update the plan as a self-contained document that assumes no prior context.
5. Keep the living sections current while implementation progresses.
6. Record design changes and rationale each time the plan changes.
7. Re-check consistency across all sections before handing off.

## Required Document Sections

Include and maintain these sections in every ExecPlan:

- Purpose / Big Picture
- Progress
- Surprises & Discoveries
- Decision Log
- Outcomes & Retrospective
- Context and Orientation
- Plan of Work
- Concrete Steps
- Validation and Acceptance
- Idempotence and Recovery
- Artifacts and Notes
- Interfaces and Dependencies

## Authoring Rules

- Keep the plan fully self-contained for a novice.
- Define any non-obvious term immediately in plain language.
- Name repository-relative file paths and exact commands.
- Phrase acceptance criteria as observable behavior.
- Prefer prose in narrative sections; use checkboxes only in Progress.
- Make steps idempotent and include retry or rollback guidance for risky operations.
- Capture concise evidence snippets that demonstrate success.
- Update all affected sections when decisions change.

## Progress Discipline

Use timestamped checklist entries and keep them accurate at every stop:

- [x] (YYYY-MM-DD HH:MMZ) Completed step.
- [ ] Pending step.
- [ ] Partial step (completed: X; remaining: Y).

Never leave stale progress state in the plan.

## Output Template

Use this skeleton as the starting envelope, then fill details with repo-specific context:

    # <Short, action-oriented description>

    This ExecPlan is a living document. Keep Progress, Surprises & Discoveries, Decision Log, and Outcomes & Retrospective current as work proceeds.

    ## Purpose / Big Picture

    ## Progress

    - [ ] (YYYY-MM-DD HH:MMZ) Initial step.

    ## Surprises & Discoveries

    - Observation:
      Evidence:

    ## Decision Log

    - Decision:
      Rationale:
      Date/Author:

    ## Outcomes & Retrospective

    ## Context and Orientation

    ## Plan of Work

    ## Concrete Steps

    ## Validation and Acceptance

    ## Idempotence and Recovery

    ## Artifacts and Notes

    ## Interfaces and Dependencies

## Revision Notes

When revising an existing ExecPlan, append a short note at the bottom that states:

- What changed
- Why it changed
- Which sections were updated
