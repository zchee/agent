---
name: progress-report
description: Reports progress of running plans (tasks)
arguments:
- mode
argument-hint: 
- mode=all|phase|custom
---

Input arguments:

- mode (optional): report type mode. When provided, switches the report scope.
    - all: All parities
    - phase: Current phase of the running task

[PROGRESS REPORT MODE]

## Capabilities

Reports the progress of running plans in a table format.

<!-- You are the lead manager overseeing the workers. -->

First, chck the plans you delegated to the workers. These are markdown files or user-prompted plans.
<!-- This includes markdown files, or user-prompted. -->

Next, check the progress of those plans. The git commit log and past `SendMessages` received from workers are the sources of this information. Accuracy of the report is important, but do not ask workers `SendMessage` questions. This is to avoid interrupting their work.
<!-- Next, check the progress of those plans. Sources include the git commit log and `SendMessage` messages received from workers. Accuracy of the report is important, but do not ask workers `SendMessage` questions. This is to avoid interrupting their work. -->

### Mode Operation:

- The default mode is `phase`. Uses if no argument is specified
- If `$mode` is `all`, it reports the parity of all plans
- If `$mode` is `phase`, it reports only the phase of the currently ongoing parity
- Otherwise, it reports according to the context provided by the user prompt

## Output format:

Present Parity table / Phase table / Wave table in Markdown format. Use ✅ / 🔶 / ⏸ / 🔜 and final commit hash. The output language is determined from the previous context.

<example>

```markdown
⏺ P25 All parity progress — W7 late stage (no change)

feature/nexus @ 5479224

| Phase | Status | Commit hash |
|-----------|--------------|--------------|
| P25a W1 (fable-5 / implementer) | ✅ Completed/Exited (2 commits completed) | 54792247707c, 41b9c80f7d73 |
| W2 (gpt-5.8-sol / artitect) | ✅ Completed | 6fd496268e3b |
| W3 (grok-4.6 / executor) | 🔶 Building: sessions --live(health().session_id resolved | |
| W4: (executor) | 🔶 Under Implementation: Raw socket unlink/lock health | |
| W5: serve/client/uds | 🔶 Late stage (current location) | |
| W6: verify (verifier) | 🔜 — 09y (14 comments) | |
| W7: under consideration (planner) | ⏸ | |

W7 wave breakdown

| Lane | Status | Commit hash |
|-----------|--------------|--------------|
| L1 (fable-5) | ✅ Completed/Exited — 4 commits (listener/R5 discipline/flock liveness/race/D2 Discrimination Test (All reverted) | bf633e0a899b |
| L2 (fable-5) | ✅ Landed — Integrated verify pending, frozen | 32465221a2f6 |
| L3 (fable-5) | ✅ Delivered/frozen — sessions --live + AC-9 ×4 + F1~F5 + (held) marker + intratree falsifier, 6/6 | e1f2a08c885d |
| verify-l3 (opus-5) | 🔶 Passed dc47119 (0 findings) → Waiting for micro-delta of 37f7fbc (difference = marker + 1 test) | |
| L4 | 🔜 After pick pair — per-session bind (W7 final lane, tui∌axum gate issue) | |

AC parity (28 items)

| AC | Status |
|-----------|--------------|
| P25a 16 items + W5b 7 items | ✅ |
| AC-22 / 15 / 26 | ✅ Landed |
| AC-9 | ✅ Implemented/verify PASS (waiting pick) |
| AC-13 live | 🔶 Waiting for user approval (independent item) |
| AC-21 Remaining | 🔜 W8 |

---

- Current Stop: None
- Next: L4 Delivery → verify → pick → wave integration verify + boundary review → handoff → lt2 close → W8 Startup
- Independent Pending: AC-13 live user approval
```

</example>

---

<!-- TODO(zchee): Make the [mode] in argument-hint more explicit -->
