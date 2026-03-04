---
description: Investigate major performance improvements using the $LSP MCP server
argument-hint: [LSP="<LSP server>"] [FOCUS="<latency|throughput|memory|cpu|io|all>"] [SCOPE="<service-or-module>"]
---
You are helping improve performance in this repository.

Inputs (from slash-command arguments):

- LSP: $LSP (required). LSP server name.
- FOCUS: $FOCUS (optional). When provided, treat this as the primary performance dimension (for example: latency, throughput, memory, CPU I/O, or all).
- SCOPE: $SCOPE (optional). When provided, limit your analysis and plan to this service, module, or subsystem.

If either argument is omitted, infer a reasonable scope or performance focus from the repository and conversation.

Goals:
- Identify realistic opportunities for **dramatic performance improvements** within $SCOPE when supplied, otherwise across the relevant parts of the system.
- Prioritize changes that directly improve **$FOCUS** when supplied; otherwise balance latency, throughput, memory usage, and operational cost.
- Ensure that all proposed and implemented changes **preserve existing API contracts** unless the user explicitly approves breaking changes.

Requirements:
- **MUST USE `ExecPlan`**
  - **MUST BE implemented all tasks continuously without stopping.**
- Always use the $LSP MCP server for code navigation, symbol references, and static analysis when investigating performance issues and candidate changes.
- Treat `.agents/plans/<descriptive-name>.md` as the **long-term plan and memory** for this performance work:
  - MUST save and restore from `.agents/plans/<descriptive-name>.md`
  - **Before** drafting or updating the implementation plan, read `.agents/plans/<descriptive-name>.md` and restore your understanding of prior decisions, current $SCOPE, and $FOCUS.
  - **After** making significant design decisions or changes to the implementation plan, update `.agents/plans/<descriptive-name>.md` with:
    - the current values of $SCOPE and $FOCUS (or that they were inferred),
    - key findings and trade-offs,
    - selected strategies and their expected impact,
    - a concise list of next actions.

When you respond:
1. State the effective scope and focus you are using, showing the resolved values of $SCOPE and $FOCUS (or that they were inferred).
2. Summarize the current performance context for $SCOPE (or the overall system if not set).
3. Propose concrete performance improvement strategies, explicitly calling out how each relates to $FOCUS when it is provided.
4. Outline a step-by-step implementation plan that a human or agent could follow.
5. Describe how you updated `.agents/plans/<descriptive-name>.md` so that future runs can resume from the latest plan.

For the Go language:
* Take a benchmark using `sync; /usr/local/sbin/purge; go test -v -run='^$' -timeout (N) -count (N) -benchtime (N)x -benchmem -bench={TARGET_BENCHMARK_NAME} ./...`
* Before optimizing, run benchmarks against existing code and measure the difference with the `benchstat` command.
  - Use the `git stash ...` command if needed
  - If performance degrades, consider a different approach or revert the changes if none exist.
* If have `benchstat` result, including to commit message
