---
name: execplan
description: >
  Create and execute structured ExecPlans — self-contained design documents that guide coding agents
  through complex implementations. Use this skill whenever the user's task involves multi-file changes,
  significant refactors, new features with multiple components, architectural changes, or any work that
  would benefit from a structured plan-then-execute approach. Also trigger when the user explicitly asks
  for an "execplan", "execution plan", or "structured plan". Even if the user doesn't mention planning,
  use this skill when the task is clearly too complex for ad-hoc implementation — e.g., tasks touching
  3+ files, requiring research before coding, involving migrations, or needing coordinated changes across
  multiple packages or services. When in doubt about whether a task needs an ExecPlan, it probably does.
---

# ExecPlan: Structured Execution Plans for Complex Tasks

An ExecPlan is a self-contained, living design document that guides you through implementing a complex feature or system change. It translates user needs into observable outcomes with step-by-step guidance.

The core idea: write a plan so complete that a novice with only the plan and the working tree could implement the feature end-to-end. Then follow that plan, keeping it updated as you go.

## When to Create an ExecPlan

Create an ExecPlan when:
- The task touches 3 or more files
- You need to research the codebase before knowing what to change
- The task involves migrations, refactors, or architectural changes
- Multiple components must be coordinated
- The user explicitly requests one
- You find yourself uncertain about the implementation approach

Do NOT create an ExecPlan for trivial fixes (typos, single-function bug fixes, config tweaks).

## The ExecPlan Workflow

### Phase 1: Research

Before writing a single line of the plan, understand the landscape:

1. Read the relevant source files. Use language server tools (go_file_context, go_search, go_symbol_references, etc.) to understand cross-file dependencies.
2. Identify the key types, functions, and modules involved.
3. Understand existing patterns — how does the codebase solve similar problems?
4. If the task has significant unknowns, prototype first. Create a small spike to validate feasibility before committing to a design.

### Phase 2: Author the ExecPlan

Save the plan to `.agents/plans/<descriptive-name>.md` in the repository root.

Write the plan following the template below. Every section matters — do not skip any.

### Phase 3: Implement

Follow the plan without stopping to ask "what next?" — just proceed to the next milestone. At every stopping point:
- Update the Progress section with timestamps
- Record any surprises in Surprises & Discoveries
- Log design decisions in the Decision Log
- Commit frequently with evidence of progress

Resolve ambiguities autonomously. If you change course, document why.

### Phase 4: Validate

Run the validation steps described in the plan. Capture evidence (test output, HTTP responses, CLI transcripts). Update the Outcomes & Retrospective section.

## Non-Negotiable Requirements

These are hard rules. Every ExecPlan must satisfy all of them:

1. **Fully self-contained.** The plan contains all knowledge needed for a novice to succeed. No references to "as defined previously" or external docs. If knowledge is required, embed it in the plan in your own words.

2. **Living document.** Update the plan as progress is made, discoveries occur, and decisions are finalized. Each revision must remain self-contained.

3. **Novice-accessible.** Define every term of art in plain language or do not use it. If you introduce jargon ("daemon", "middleware", "RPC gateway"), define it immediately and name the files where it manifests.

4. **Observable outcomes.** State what the user can do after implementation, the commands to run, and the outputs they should see. Acceptance is behavioral ("navigating to /health returns HTTP 200") not structural ("added a HealthCheck struct").

5. **Idempotent steps.** Steps can be run multiple times without damage or drift. Include retry/rollback paths for risky operations.

## Writing Guidelines

**Prose over lists.** Prefer sentences over checklists, tables, and enumerations. Checklists are mandatory only in Progress. Everything else should read as narrative.

**Anchor with observables.** For internal changes, explain how impact can still be demonstrated (tests that fail before and pass after, scenarios that exercise the new behavior).

**Be explicit about context.** Name files with full repo-relative paths. Name functions and modules precisely. Show working directories for commands. State environment assumptions.

**Capture evidence.** Include terminal output, short diffs, or logs as indented blocks. Keep them concise and focused on proving success.

**Milestones are stories.** Each milestone gets a brief paragraph: scope, what will exist at the end, commands to run, expected acceptance. Goal, work, result, proof.

## ExecPlan Template

When writing an ExecPlan to a standalone `.md` file, omit the outer triple backtick fence.

```
# <Short, action-oriented title>

This ExecPlan is a living document. The sections Progress, Surprises & Discoveries,
Decision Log, and Outcomes & Retrospective must be kept up to date as work proceeds.

## Purpose / Big Picture

Explain in a few sentences what someone gains after this change and how they can see
it working. State the user-visible behavior you will enable.

## Progress

- [x] (YYYY-MM-DD HH:MMZ) Example completed step.
- [ ] Example incomplete step.
- [ ] Example partially completed step (completed: X; remaining: Y).

## Context and Orientation

Describe the current state relevant to this task as if the reader knows nothing.
Name the key files and modules by full path. Define any non-obvious term.

## Plan of Work

Describe, in prose, the sequence of edits and additions. For each edit, name the
file and location (function, module) and what to insert or change. Keep it concrete
and minimal.

## Concrete Steps

State the exact commands to run and where to run them (working directory). Show
expected output so the reader can compare. Update as work proceeds.

## Validation and Acceptance

Describe how to exercise the system and what to observe. Phrase acceptance as
behavior with specific inputs and outputs. State test commands and how to
interpret results.

## Idempotence and Recovery

If steps can be repeated safely, say so. If a step is risky, provide a safe
retry or rollback path.

## Surprises & Discoveries

Document unexpected behaviors, bugs, optimizations, or insights. Provide evidence.

- Observation: ...
  Evidence: ...

## Decision Log

Record every decision made during implementation.

- Decision: ...
  Rationale: ...
  Date: ...

## Outcomes & Retrospective

Summarize outcomes, gaps, and lessons learned at major milestones or at completion.
Compare the result against the original purpose.

## Interfaces and Dependencies

Name the libraries, modules, and services to use and why. Specify the types,
interfaces, and function signatures that must exist at the end of the milestone.

## Artifacts and Notes

Include the most important transcripts, diffs, or snippets as indented examples.
Keep them concise and focused on what proves success.
```

## Prototyping and Parallel Implementations

It is encouraged to include explicit prototyping milestones when they de-risk a larger change. Examples: adding a low-level operator to validate feasibility, or exploring two composition orders while measuring performance.

Keep prototypes additive and testable. Label the scope as "prototyping." Describe how to run and observe results. State the criteria for promoting or discarding the prototype.

Parallel implementations (keeping an adapter alongside an older path during migration) are fine when they reduce risk or enable tests to keep passing. Describe how to validate both paths and how to retire one safely.

## Plan Maintenance

When revising a plan:
- Ensure changes are reflected across ALL sections, including the living document sections
- Write a note at the bottom describing the change and the reason why
- If building upon a prior ExecPlan that is checked into the repo, incorporate it by reference
- If the prior plan is not checked in, include all relevant context from it

The bar: a single, stateless agent — or a human novice — can read the ExecPlan from top to bottom and produce a working, observable result. Self-contained, self-sufficient, novice-guiding, outcome-focused.
