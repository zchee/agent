---
name: perf-plan
description: Investigate and drive major performance work using repository-local planning, LSP-assisted code navigation, and benchmark evidence. Use when the user asks for performance optimization planning, wants dramatic latency, throughput, memory, CPU, I/O, or cost improvements, or invokes `/perf-plan`-style work with an LSP, focus, or scope.
---

# Perf Plan

Use this skill to run a performance investigation from scoping through implementation without losing context between iterations.

## Resolve Inputs

- Determine effective `LSP`, `FOCUS`, and `SCOPE` from the request.
- If one is omitted, infer a conservative default from the repository and current conversation, then state the resolved values once.
- Follow the repository's planning convention when it exists. Otherwise, use `.agents/plans/<descriptive-name>.md` as the living plan file.

## Workflow

1. Rebuild context from repository instructions, any existing plan file, and the current performance-sensitive code paths.
2. Use the selected LSP MCP server for navigation, symbol references, and static analysis throughout the investigation.
3. Summarize the current performance context for the effective scope.
4. Identify high-leverage strategies and rank them by expected impact, risk, and implementation cost.
5. Preserve external contracts unless the user explicitly approves breaking changes.
6. Apply the `execplan` workflow for the implementation plan and keep the living plan current as work proceeds.
7. Continue through investigation, implementation, and validation unless blocked by missing information or an unsafe decision.
8. Benchmark before and after each material optimization using the language-appropriate toolchain.
9. If a change regresses performance or correctness, revise the approach instead of rationalizing the regression.

## Planning and Memory

- Save key findings, trade-offs, chosen strategy, and next actions in the living plan file after significant decisions.
- Treat the plan file as the resume point for future runs.
- Keep plan updates concrete enough that a fresh contributor can continue without chat history.

## Benchmark Rules

- For Go with `gopls`, capture baseline and current runs with identical benchmark parameters and compare them with `benchstat`.
- For Rust with `rust-analyzer`, use `cargo bench` and `criterion` with like-for-like comparisons.
- When the repository uses a different harness, follow that harness but keep baseline/current comparability strict.
- Do not claim an improvement without reproducible evidence.

## Response Shape

- State resolved `LSP`, `FOCUS`, and `SCOPE`.
- Summarize the current bottlenecks or likely bottlenecks.
- Present concrete improvement strategies tied to the requested focus.
- Provide the implementation sequence and the plan-file location updated.
- Note benchmark evidence, blockers, and remaining risks.
