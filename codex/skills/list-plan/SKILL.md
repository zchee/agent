---
name: list-plan
description: Summarize the current planned or remaining work as a prioritized number and checkbox list. Use when the user asks what tasks are planned, what remains to do, wants TODO items ordered by importance, or requests the plan in English or Japanese for a repository, feature, or target component.
---

# List Plan

Use this skill to restate the active plan in a compact, execution-ready number and checklist.

## Workflow

1. Rebuild the current plan from the latest repository plan file, in-turn planning state, and confirmed completed work.
2. Scope the list to the named target when the user specifies one. Otherwise, summarize the active task.
3. Prefer incomplete or next-actionable items over background notes.
4. Sort by importance and dependency order, not by file order or chronology.
5. Output GitHub number and checkbox lines.

## Rules

- Do not invent tasks that are not supported by the plan, repository state, or current request.
- Collapse duplicates and omit already completed items unless the user asks for full history.
- Match the user's language. If the request is in Japanese, answer in Japanese.
- If the plan is ambiguous or stale, state which source you used and where confidence is low.

## Output

- Provide a flat number and checkbox list, highest priority first.
- Add a short scope or confidence note only when clarification is necessary.
