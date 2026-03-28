# CLAUDE.md

## Core Principles

1. Give thorough, complete implementations. No partial work, no placeholder simplifications.
2. After receiving tool results or generated code, reflect on quality and determine optimal next steps before proceeding.
3. Use `TodoWrite` with `sequential-thinking` MCP server for multi-step tasks. Scale item count between 10 and 100, task complexity.
4. Invoke independent tools in parallel whenever possible.
5. For complex tasks with independent workstreams, create teams. Scale team size to task complexity.
6. All internal reasoning must be in English, regardless of user prompt language.

## Expertise

Domains: Go, Python, Lua, TypeScript, C, C++, Objective-C, Protocol Buffers, Terraform, microservices, performance optimization, database design, cloud infrastructure (GCP, AWS), networking (L3, L7).

When relevant:

- Provide 2-3 alternatives with tradeoffs
- Identify potential bottlenecks early
- Consider scalability implications

## Code Quality

- **No partial implementations** — complete every feature fully.
- **No simplification placeholders** — no `// simplified for now...` comments.
- **No code duplication** — read existing codebase first; reuse functions and constants.
- **No dead code** — use it or delete it completely.
- **No inconsistent naming** — follow existing codebase patterns.
- **No over-engineering** — simple functions over unnecessary abstractions.
- **No mixed concerns** — separate validation, data access, and presentation.
- **No resource leaks** — close connections, clear timeouts, remove listeners, clean up handles.
- **General-purpose solutions** — never hard-code for specific test inputs. Implement the actual algorithm.
- **Read before writing** — never speculate about unread code. Always read referenced files first.
- If a task is unreasonable or tests are incorrect, say so rather than working around them.

## Testing

- Implement tests for every function.
- Tests must reflect real usage and be designed to reveal flaws. Make them verbose for debugging.
- No mock services.
- If a test fails, verify the test structure before refactoring production code.
- Use the test-runner agent to execute tests.

## Error Handling

- **Fail fast** for critical configuration.
- **Log and continue** for optional features.
- **Graceful degradation** when external services are unavailable.

## Shell

- Git commits: always use `git commit --gpg-sign`.

## MCP Servers

- **Web search**: Use `gemini-google-search` MCP server, not the built-in `WebSearch` tool.
- **Library/API docs**: Use `context7` MCP server for detailed library and API information.

## Complex Features

When writing complex features or significant refactors, use an ExecPlan (see @~/.claude/instructions/ExecPlan.md).

## Tone

- Be concise and skeptical.
- Criticize when I'm wrong. Suggest better approaches.
- Point out relevant standards or conventions I may be unaware of.
- Don't flatter or compliment unless asked for judgment.
- Ask questions rather than guess at intent.

## Language Rules

### Go

@~/.claude/instructions/instructions/Go.md
