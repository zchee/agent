#!/opt/homebrew/bin/bash
# PreToolUse(Bash) hook: rewrite the command so find/grep resolve to the
# non-embedded, full-featured Homebrew binaries instead of Claude Code's
# embedded bfs/ugrep multicall. Runs inside `eval`, after the shell snapshot
# is sourced, so these function definitions override the snapshot's shadows.
# No turn is wasted: the command is transformed in place, never rejected.
set -euo pipefail
in="$(cat)"
cmd="$(jq -r '.tool_input.command // empty' <<<"$in")"
# Nothing to do if there is no command.
if [[ -z "$cmd" ]]; then printf '{}'; exit 0; fi

# Idempotency: skip if already wrapped.
if [[ "$cmd" == *"__cc_search_override"* ]]; then printf '{}'; exit 0; fi

# grep -> Homebrew ugrep (grep-compatible). find -> Homebrew bfs (find-compatible).
# Both are non-embedded builds with full codec/feature support.
prefix=': __cc_search_override; '
prefix+='if [ -x /opt/homebrew/bin/ugrep ]; then grep(){ command /opt/homebrew/bin/ugrep "$@"; }; fi; '
prefix+='if [ -x /opt/homebrew/bin/bfs ]; then find(){ command /opt/homebrew/bin/bfs "$@"; }; fi; '

newcmd="${prefix}${cmd}"

# Return the FULL tool input with only .command replaced (updatedInput fully
# replaces the input and is validated against the Bash tool schema).
jq -nc --argjson orig "$(jq -c '.tool_input' <<<"$in")" --arg c "$newcmd" '
  {hookSpecificOutput:{hookEventName:"PreToolUse", updatedInput:($orig + {command:$c})}}'
