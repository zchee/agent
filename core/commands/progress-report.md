---
name: progress-report
description: Reports progress of running plans (tasks)
arguments:
  - mode
argument-hint: 
  - mode=all|phase|custom
---

## Input arguments:

- mode (optional): report type mode. When provided, switches the report scope.
    - all: All parities
    - phase: Current phase of the running task

### Mode Operation

- The default mode is `phase`. Uses if no argument is specified
- If `$mode` is `all`, it reports the parity of all plans
- If `$mode` is `phase`, it reports only the phase of the currently ongoing parity
- Otherwise, it reports according to the context provided by the user prompt

## Capabilities

Reports the progress of running plans in a table format.

First, chck the plans you delegated to the workers. These are markdown files or user-prompted plans.

Next, check the progress of those plans. You can get this information from the worker's transcript, the git status, the git commit log, and past `SendMessages` received from the worker. Report accuracy is important, but don't ask employees about "SendMessages." This avoids interruptions in your work.

## Output format:

Present Parity table / Phase table / Wave table in Markdown format. Use ✅ / 🔶 / ⏸ / 🔜 and final commit hash. The output language is determined from the previous context.

<example>

```markdown
feature/nexus @ 5479224

**Lane**


| Wave / Lane (model) | Description | Status | Commit |
|---|---|---|---|
| w1 L1a (ganja-tool / fable-5) | New `registry.rs`: the Record type, the name grammar, the sanitizer, the shared case-fold predicate, the collision scan, the liveness probe. | ✅ Complete/Exited (358/358) | `53bbfe3` |
| w1 L1b (protocol+config / fable-5) | `session_mentions: Vec<String>` on `SendPrompt`/`Steer`; the `teamless_send: "unasked"\|"ask"` config key + schema in the same commit + the S2 cross-references | ✅ Complete/Exited (45/45 + 125/125) | `53bbfe3` |
| w1 L1c (ganja-permission / fable-5) | `gate_with_default(unmatched: Option<Decision>)`, API-only: None = today's static ladder; Some consulted only at the unmatched arm. | ✅ Complete/Exited (71/71) | `53bbfe3` |
| w1 integration (w1-integrate / sonnet-5) | Mechanical completion sweep: 57 files, ~235 sites. | ✅ Complete/Exited | `53bbfe3` |
| w2 L2a (identity.rs / opus-5) | Engine-owned resolver: fresh read per call, own/stale exclusion, case-insensitive match, the pin guard, `resolve_address` | ✅ Complete/Exited (25/25 + lib 670/670; 3 lead rulings accepted) | working tree (rides the W2 commit) |
| w2 L2b (engine/session/subagent / sonnet-5) | engine: `Arc<Identity>` + `with_socket_directory`, the self-name cell + `set_self_name` (ADJ-2). | 🔶 **Mid-implementation** — all 3 files underway (+124 lines; session.rs 116 = the mention seam is in) | |
| w3 L3a (TUI) | New `lister.rs` trait + the `lister` param on `run`; registration lifecycle on `Synced`; `/rename`; `@` menu roster/session rows; submit classification; insta snapshots | 🔜 After the W2 commit (parallel with L3b) | |


**Wave**


| Wave / Lane (model) | Description | Status | Commit |
|---|---|---|---|
| W1 (ganja-tool / fable-5) | New `registry.rs` | ✅ Complete/Exited (358/358) | `53bbfe3` |
| W2 (identity.rs / opus-5) | Engine-owned resolver | ✅ Complete/Exited (25/25 + lib 670/670; 3 lead rulings accepted) | working tree (rides the W2 commit) |
| W3 (TUI) | New `lister.rs` trait + the `lister` param on `run` | 🔜 After the W2 commit (parallel with L3b) | |
| W4-1 docs | Root AGENTS.md | 🔜 | |
| W4-2 | module-doc audit | 🔜 | |
| W4-3 | beads | 🔜 | |
| W4-4 verify (verifier / sonnet-5) | Five gates + ban greps + AC-29 boundary + 5 citation + evidence bundle | 🔜 (separate lane, never an author's context) | |


**AC parity (42 items, with descriptions)**


| AC | Description (summary) | Status |
|---|---|---|
| AC-1 | Record serde round-trip, typed-case preserved, tolerant read, format gate, non-hex-stem never read | ✅ `53bbfe3` |
| AC-2 | Record appears on bind / removed at teardown / moved on rebind (staging never visible, F8) | 🔜 L3a |
| AC-3 | A refused bind writes no record (refused rebind: old removal only, P2) | 🔜 L3a |
| AC-4 | Default name = project basename sanitized (empty → `ganja`), source recorded | ✅ sanitizer half `53bbfe3` / rest L3a·L3b |
| AC-5 | Name grammar refuses every clause by name (at `--name` and `/rename` alike) | ✅ predicate `53bbfe3` / surfaces W3 |
| AC-6 | Collision = notice naming the holder (stem+cwd); registration always succeeds | 🔜 L3a |
| AC-7 | `/rename` rewrites in place + collision notice + grammar refusals | 🔜 L3a |
| AC-8 | `--live` GCs stale records + orphan-record arm (N4); no `.lock` ever removed | 🔜 L3b |
| AC-9 | Liveness probe: lock held = live / free = stale; unlinks nothing | ✅ `53bbfe3` |
| AC-10 | Roster precedence (same-name live unreachable by bare name, reachable by `uds:`) | 🔶 L2b |
| AC-11 | Unique live name delivers; `Sent.to` composes all three identities (N6) | 🔶 L2b |
| AC-12 | Pin guard: name moved to another session ⇒ `NameMoved`, no delivery, no re-pin | ✅ resolver half / 🔶 delivery half L2b |
| AC-13 | Two live same-name ⇒ `Ambiguous` listing every candidate; nothing delivered or pinned | ✅ resolver / 🔶 delivery L2b |
| AC-14 | Both misses ⇒ `Unknown` naming both | ✅ resolver / 🔶 delivery L2b |
| AC-15 | Unreadable listing ⇒ `Failed` (refuse-don't-guess, never `Unknown`) | ✅ resolver / 🔶 delivery L2b |
| AC-16 | Eight rungs, `parse_address`, `vet_address` byte-identical | 🔶 diff evidence at W4 |
| AC-17 | Own session's record never resolves or lists | ✅ resolver / 🔶 L2b |
| AC-18 | A stale same-name does not force ambiguity | ✅ resolver / 🔶 L2b |
| AC-19 | Under `--socket-dir`, binder/resolver/lister read one directory (two-process E2E) | 🔜 L3b |
| AC-20 | `NewSession` clears pins; volatility stated in the module doc | ✅ resolver / 🔶 seam L2b |
| AC-21 | `session_mentions` round-trips; empty serializes byte-identically to today | ✅ `53bbfe3` |
| AC-22 | Submit classification: file wins / roster·listing match / `uds:` as typed / else literal | 🔜 L3a |
| AC-23 | Menu roster/session rows, duplicate stems, `@uds:` splice snapshot-pinned, shadow marker | 🔜 L3a |
| AC-24 | All six reminder arms byte-pinned (uds miss = address-miss rendering, R3) | ✅ landed in working tree (L2a) |
| AC-25 | A mention transmits nothing (spy postbox + pin map byte-unchanged + no dialog/hook) | 🔶 L2b |
| AC-26 | Peer text never reaches resolution (engine + TUI halves) | 🔶 L2b / 🔜 L3a |
| AC-27 | No lister ⇒ files + roster only, nothing regresses | 🔜 L3a |
| AC-28 | Partial listing ⇒ incomplete marker, completion still works | 🔜 L3a |
| AC-29 | Five gates + serve/team/binder untouched (**amended: serve = the one mechanical line**) + zero new deps | 🔶 ongoing (final ruling W4) |
| AC-30 | No new path logs a message body | 🔶 per lane + W4 audit |
| AC-31 | Teamless tool registration (no-team description), bare name + `uds:` deliver, headless none, member `NoTransport` | ✅ description half / 🔶 L2b |
| AC-32 | Far inbox `from = <self-name>@solo`; zero receiver-side changes (inbound.rs untouched) | 🔶 L2b |
| AC-33 | Teamless success note carries the one-way clause; lead's doesn't; no reply-ability claims | 🔶 L2b |
| AC-34 | `teamless_send` parses exactly two values; refusal names the key; absent = unasked | ✅ `53bbfe3` |
| AC-35 | Posture lifecycle: ask ⇒ dialog / stored always silences / deny denies / mid-session spawn ⇒ unasked / in-team untouched | 🔶 L2b |
| AC-36 | Tier rule: project tightens only; `GANJA_CONFIG` outranks global | ✅ `53bbfe3` |
| AC-37 | Teamless TUI = session rows + reminders; member = roster only; binder stays lead-only | 🔜 L3a |
| AC-38 | Explicit rule and stored "always" outrank the handed-in default + `gate()` equivalence pin | ✅ `53bbfe3` |
| AC-39 | Incumbent's collision notice (30s-throttled re-scan, once per new collider) | 🔜 L3a |
| AC-40 | Teamless `/rename` → cell update → next send's `from` changes (read from far inbox) + notice | 🔶 engine half L2b / 🔜 TUI half L3a |
| AC-41 | Post-`NameMoved` `uds:` send succeeds, pin map unchanged (F3) | ✅ resolver half / 🔶 L2b |
| AC-42 | Team end swaps solo back; `TEAM_GONE` structurally unreachable for never-teamed | 🔶 L2b |


**Mid-flight rulings ledger (all settled, not reviewable)**


| Ruling | Content |
|---|---|
| AC-29 amendment (user-ratified 2026-08-27) | serve/routes.rs may carry exactly one mechanical line (`session_mentions: Vec::new(),`); recorded in the W4 ledger + verify evidence |
| Model routing (user directive 2026-08-27) | All lanes follow the OMC config's agent models (no overrides; config-driven escalation included). Replaces the all-Fable-5 directive |
| L2a rulings ×3 (lead) | `of_address` defensive fold / `Mentioned::Unchecked` as the 7th rendering (ganja-inferred, W4 records it) / no `of_roster` helper by design |
| runner.rs ownership (lead) | Mechanical completion folded into W1 integration; behavioral changes belong to L2b |

---

- Current stop: none — L2b mid-implementation (all three files underway, +124 lines; the session.rs mention seam is largely in)
- Next: L2b report → W2 workspace gates → commit + push → W3 in parallel (L3a TUI / L3b CLI) → W4 (docs → ledger → beads → verify bundle)
- Independent pending: none
```
</example>

---

<!-- TODO(zchee): Make the [mode] in argument-hint more explicit -->
