#!/usr/bin/env bash
# Behaviour tests for hooks/src/cc-shim-hook.c.
#
# Builds the source into a temporary directory and drives it with synthetic
# PreToolUse payloads, comparing stdout against the exact bytes the hook is
# supposed to emit. Every case pins CLAUDE_CONFIG_DIR and HOME at fixture
# directories and unsets CC_SHIM_*, so the developer's own hooks/shim-tools.json
# cannot change the result.
#
#   ./claude/hooks/src/tests/cc-shim-hook.test.sh

set -uo pipefail

src=${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/cc-shim-hook.c}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
hook=$work/cc-shim-hook
T=$work/tree

cc -O1 -Wall -Wextra -Wconversion -o "$hook" "$src" || exit 1
mkdir -p "$T/cfgdir/hooks" "$T/home" "$T/empty"

pass=0 fail=0
chk() { # label, expected substring, actual
  if [[ "$3" == *"$2"* ]]; then
    pass=$((pass + 1))
    printf 'ok   %s\n' "$1"
  else
    fail=$((fail + 1))
    printf 'FAIL %s\n       want: %s\n       got:  %s\n' "$1" "$2" "${3//$'\n'/ | }"
  fi
}
chk_eq() { # label, expected exact, actual
  if [[ "$3" == "$2" ]]; then
    pass=$((pass + 1))
    printf 'ok   %s\n' "$1"
  else
    fail=$((fail + 1))
    printf 'FAIL %s\n       want: %s\n       got:  %s\n' "$1" "$2" "${3//$'\n'/ | }"
  fi
}
chk_no() { # label, forbidden substring, actual
  if [[ "$3" != *"$2"* ]]; then
    pass=$((pass + 1))
    printf 'ok   %s\n' "$1"
  else
    fail=$((fail + 1))
    printf 'FAIL %s\n       must not contain: %s\n       got:  %s\n' "$1" "$2" "${3//$'\n'/ | }"
  fi
}

# The developer's own CC_SHIM_* must not reach the hook: a config path or the
# debug flag in the ambient environment would silently rewrite these results.
clean="env -u CC_SHIM_CONFIG -u CC_SHIM_DEBUG"
# $1 command, $2 optional CC_SHIM_CONFIG, $3 optional config dir (default: empty)
run() {
  local dir=${3:-$T/cfgdir}
  printf '{"session_id":"s","tool_name":"Bash","tool_input":{"command":"%s"}}' "$1" |
    $clean HOME="$T/home" CLAUDE_CONFIG_DIR="$dir" ${2:+CC_SHIM_CONFIG="$2"} "$hook" 2>/dev/null
}
run_dbg() {
  local dir=${3:-$T/cfgdir}
  printf '{"session_id":"s","tool_name":"Bash","tool_input":{"command":"%s"}}' "$1" |
    $clean HOME="$T/home" CLAUDE_CONFIG_DIR="$dir" ${2:+CC_SHIM_CONFIG="$2"} CC_SHIM_DEBUG=1 "$hook" 2>&1 >/dev/null
}

# ---------------------------------------------------------------- built-ins --
# With no config anywhere the emitted bytes must stay exactly what this hook
# produced before the config existed. These three strings are that contract;
# changing them changes what every Bash call runs.
bi_grep='grep(){ command \"${CLAUDE_CODE_UGREP:-/opt/homebrew/bin/ugrep}\" ${CLAUDE_CODE_UGREP_ARGS--G --ignore-files --hidden -I --exclude-dir=.git --exclude-dir=.svn --exclude-dir=.hg --exclude-dir=.bzr --exclude-dir=.jj --exclude-dir=.sl} \"$@\"; }; '
bi_find='find(){ command \"${CLAUDE_CODE_BFS:-/usr/local/bin/bfs}\" ${CLAUDE_CODE_BFS_ARGS--S dfs -regextype findutils-default} \"$@\"; }; '
bi_rg='rg(){ command \"${CLAUDE_CODE_RG:-/opt/homebrew/bin/rg}\" ${CLAUDE_CODE_RG_ARGS-} \"$@\"; }; '

out=$(run 'grep -n foo bar.txt')
chk_eq 'builtin: grep injection is byte-exact' \
  "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":{\"command\":\": __cc_shim_override; ${bi_grep}grep -n foo bar.txt\"}}}" "$out"

out=$(run 'find . -name x')
chk_eq 'builtin: find injection is byte-exact' \
  "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":{\"command\":\": __cc_shim_override; ${bi_find}find . -name x\"}}}" "$out"

out=$(run 'rg pat')
chk_eq 'builtin: rg injection is byte-exact' \
  "{\"hookSpecificOutput\":{\"hookEventName\":\"PreToolUse\",\"updatedInput\":{\"command\":\": __cc_shim_override; ${bi_rg}rg pat\"}}}" "$out"

out=$(run 'find . | grep x')
chk 'builtin: two tools inject in table order' "${bi_grep}${bi_find}find . | grep x" "$out"

# ------------------------------------------------------------------ no-ops --
chk_eq 'noop: command names no tool' '{}' "$(run 'echo hello')"
chk_eq 'noop: already rewritten' '{}' "$(run ': __cc_shim_override; grep x')"
chk_eq 'noop: substring is not a word' '{}' "$(run 'echo regrep findings rgx')"
chk_eq 'noop: no tool_input' '{}' "$(printf '{"tool_name":"Bash"}' | $clean HOME="$T/home" CLAUDE_CONFIG_DIR="$T/cfgdir" "$hook")"
chk_eq 'noop: other tool' '{}' \
  "$(printf '{"tool_name":"Read","tool_input":{"command":"grep x"}}' | $clean HOME="$T/home" CLAUDE_CONFIG_DIR="$T/cfgdir" "$hook")"
chk_eq 'noop: empty stdin' '{}' "$(: | $clean HOME="$T/home" CLAUDE_CONFIG_DIR="$T/cfgdir" "$hook")"

# tool_input members other than command must survive untouched.
out=$(printf '{"tool_name":"Bash","tool_input":{"command":"grep x","description":"d","timeout":5000}}' |
  $clean HOME="$T/home" CLAUDE_CONFIG_DIR="$T/cfgdir" "$hook")
chk 'builtin: sibling tool_input members are preserved' '"description":"d","timeout":5000' "$out"

# ------------------------------------------------------------- config: good --
cat >"$T/cfgdir/hooks/shim-tools.json" <<'J'
{
  "tools": {
    "sed": { "bin": "/opt/homebrew/opt/gnu-sed/libexec/gnubin/sed" },
    "awk": { "bin": "/opt/homebrew/opt/gawk/libexec/gnubin/awk", "args": "--posix" },
    "grep": { "bin": "/opt/homebrew/bin/ugrep", "args": "-G", "bin_env": "MY_GREP", "args_env": "MY_GREP_ARGS" }
  }
}
J
out=$(run 'sed -e s/a/b/ f')
chk 'config: bin only, emitted literally' 'sed(){ command \"/opt/homebrew/opt/gnu-sed/libexec/gnubin/sed\" \"$@\"; }; ' "$out"
chk_no 'config: bin only has no ${} substitution' 'sed(){ command \"${' "$out"

out=$(run 'awk 1 f')
chk 'config: args without args_env are literal' 'awk(){ command \"/opt/homebrew/opt/gawk/libexec/gnubin/awk\" --posix \"$@\"; }; ' "$out"

out=$(run 'grep x f')
chk 'config: bin_env uses :- and args_env uses -' \
  'grep(){ command \"${MY_GREP:-/opt/homebrew/bin/ugrep}\" ${MY_GREP_ARGS--G} \"$@\"; }; ' "$out"

chk_eq 'config: a built-in absent from the config is no longer shadowed' '{}' "$(run 'find . -name x')"
chk 'config: debug names the adopted table' 'tools: sed awk grep' "$(run_dbg 'grep x')"

# The config decides the token set, so a name it adds is scanned for and a
# name it drops is not: this is the whole point of the table being data.
out=$(run 'sed -i s/a/b/ f && awk 1 g')
chk 'config: two configured tools inject together' 'sed(){ command' "$out"
chk 'config: ...and the second one too' 'awk(){ command' "$out"

# --------------------------------------------------------- config: override --
cat >"$T/explicit.json" <<'J'
{ "tools": { "tar": { "bin": "/opt/homebrew/opt/gnu-tar/libexec/gnubin/tar" } } }
J
out=$(run 'tar -xf a.tar' "$T/explicit.json")
chk 'config: CC_SHIM_CONFIG overrides the config dir' 'tar(){ command \"/opt/homebrew/opt/gnu-tar/libexec/gnubin/tar\"' "$out"
chk_eq 'config: override replaces, not merges' '{}' "$(run 'sed -e x f' "$T/explicit.json")"

# HOME/.claude is the fallback when CLAUDE_CONFIG_DIR is unset.
mkdir -p "$T/home/.claude/hooks"
cat >"$T/home/.claude/hooks/shim-tools.json" <<'J'
{ "tools": { "xz": { "bin": "/opt/homebrew/bin/xz" } } }
J
out=$(printf '{"tool_name":"Bash","tool_input":{"command":"xz -d a.xz"}}' |
  $clean -u CLAUDE_CONFIG_DIR HOME="$T/home" "$hook")
chk 'config: falls back to $HOME/.claude/hooks/shim-tools.json' 'xz(){ command \"/opt/homebrew/bin/xz\"' "$out"

# --------------------------------------------------------- config: rejected --
# Every rejection must land on the built-in table, never on a partial one.
reject() { # label, config body
  local d=$work/rej
  rm -rf "$d"; mkdir -p "$d/hooks"
  printf '%s' "$2" >"$d/hooks/shim-tools.json"
  local o
  o=$(run 'grep -n foo bar.txt' '' "$d")
  chk "reject: $1 falls back to built-ins" "${bi_grep}grep -n foo bar.txt" "$o"
  chk "reject: $1 says so under debug" 'rejected config' "$(run_dbg 'grep x' '' "$d")"
}
reject 'truncated JSON'        '{ "tools": { "sed": { "bin": "/bin/sed"'
reject 'not an object'         '[]'
reject 'no tools member'       '{ "other": 1 }'
reject 'empty tools'           '{ "tools": {} }'
reject 'tool without bin'      '{ "tools": { "sed": { "args": "-e" } } }'
reject 'empty bin'             '{ "tools": { "sed": { "bin": "" } } }'
reject 'name with a space'     '{ "tools": { "se d": { "bin": "/bin/sed" } } }'
reject 'name with a semicolon' '{ "tools": { "sed;rm -rf /": { "bin": "/bin/sed" } } }'
reject 'name starting a digit' '{ "tools": { "7z": { "bin": "/bin/7z" } } }'
reject 'bin_env with a brace'  '{ "tools": { "sed": { "bin": "/bin/sed", "bin_env": "A}$(id)" } } }'
reject 'args_env with a dash'  '{ "tools": { "sed": { "bin": "/bin/sed", "args_env": "A-B" } } }'

# 17 tools is one past MAX_TOOLS.
many='{ "tools": {'
for i in $(seq 1 17); do
  [[ $i -gt 1 ]] && many+=','
  many+="\"t$i\": { \"bin\": \"/bin/t$i\" }"
done
many+='} }'
reject 'more tools than MAX_TOOLS' "$many"

# A string that merely reads like the tools key must not derail the scan.
d=$work/decoy; mkdir -p "$d/hooks"
cat >"$d/hooks/shim-tools.json" <<'J'
{ "note": "the \"tools\" member below is the real one", "tools": { "sed": { "bin": "/bin/sed" } } }
J
chk 'config: a decoy "tools" string is skipped' 'sed(){ command \"/bin/sed\"' "$(run 'sed x' '' "$d")"

# Unknown members are ignored rather than rejected.
d=$work/unknown; mkdir -p "$d/hooks"
cat >"$d/hooks/shim-tools.json" <<'J'
{ "note": "hi", "tools": { "sed": { "bin": "/bin/sed", "future": {"a":[1,2]}, "n": 3 } } }
J
chk 'config: unknown members are ignored' 'sed(){ command \"/bin/sed\"' "$(run 'sed x' '' "$d")"

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[[ $fail -eq 0 ]]
