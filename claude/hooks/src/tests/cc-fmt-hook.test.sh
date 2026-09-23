#!/usr/bin/env bash
# Behaviour tests for hooks/src/cc-fmt-hook.c.
#
# Builds the source into a temporary directory and drives it with synthetic
# PostToolUse payloads, reading CC_FMT_DRYRUN output to see which rule won.
# Every case pins CLAUDE_CONFIG_DIR at a fixture directory so the developer's
# own ~/.claude/fmt-hooks.json cannot change the result.
#
#   ./claude/hooks/src/tests/cc-fmt-hook.test.sh

set -uo pipefail

src=${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/cc-fmt-hook.c}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
hook=$work/cc-fmt-hook
T=$work/tree

cc -O1 -Wall -Wextra -Wconversion -o "$hook" "$src" || exit 1

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
# $1 path, $2 optional CC_FMT_CONFIG, $3 optional CC_FMT_TRUST
# The developer's own CC_FMT_* must not reach the hook: CC_FMT_ASYNC=1 or
# CC_FMT_TRUST in the ambient environment would silently rewrite these results.
# Every case starts from a clean slate and sets only what it is testing.
clean="env -u CC_FMT_CONFIG -u CC_FMT_TRUST -u CC_FMT_ASYNC -u CC_FMT_DRYRUN"
run() {
  printf '{"session_id":"s","tool_input":{"file_path":"%s"}}' "$1" |
    $clean PATH="$PATH" HOME="$HOME" CLAUDE_CONFIG_DIR="$T/cfgdir" \
      ${2:+CC_FMT_CONFIG="$2"} ${3:+CC_FMT_TRUST="$3"} CC_FMT_DRYRUN=1 "$hook" |
    grep -v '^# config: '
}

mkdir -p "$T/cfgdir" "$T/empty" "$T/repo/.claude" "$T/repo/sub/deep" "$T/repo/vendor/x" "$T/repository/.claude"

cat >"$T/cfgdir/fmt-hooks.json" <<'J'
{
  "rs": ["echo", "GLOBAL-rs"],
  "toml": ["echo", "GLOBAL-toml"],
  "py": ["echo", "GLOBAL-py"],
  "Makefile": ["echo", "GLOBAL-make"],
  ".clang-format": ["echo", "GLOBAL-dotfile"],
  "CMakeLists.txt": ["echo", "GLOBAL-cmake"],
  "My Notes.md, README.md": ["echo", "GLOBAL-notes"],
  "*_pb2.py": ["echo", "GLOBAL-pb2"],
  "*/vendor/*": []
}
J
echo '{ "rs": ["echo", "PROJECT-rs"], "Cargo.toml": [] }' >"$T/repo/.claude/fmt-hooks.json"
echo '{ "rs": ["echo", "SUB-rs"] }' >"$T/repo/sub/.fmt-hooks.json"
echo '{ "rs": ["echo", "NEIGHBOUR-rs"] }' >"$T/repository/.claude/fmt-hooks.json"
echo '{ "rs": ["echo", "OVERRIDE-rs"] }' >"$T/override.json"

echo '# layering: the closest config wins'
chk "global applies with no project config" GLOBAL-rs "$(run "$T/other/a.rs")"
chk "project overrides global" PROJECT-rs "$(run "$T/repo/a.rs")"
chk "subproject overrides project" SUB-rs "$(run "$T/repo/sub/a.rs")"
chk "rules inherit further down" SUB-rs "$(run "$T/repo/sub/deep/a.rs")"
chk "a sibling directory is unaffected" PROJECT-rs "$(run "$T/repo/other/a.rs")"
chk "CC_FMT_CONFIG beats every file" OVERRIDE-rs "$(run "$T/repo/sub/a.rs" "$T/override.json")"

echo '# pass 1: exact file name, ahead of the extension'
chk "a name rule carves out an exception" "disabled: Cargo.toml" "$(run "$T/repo/Cargo.toml")"
chk "the extension rule still covers the rest" GLOBAL-toml "$(run "$T/repo/x.toml")"
chk "the exception is scoped to the project" GLOBAL-toml "$(run "$T/elsewhere/Cargo.toml")"
chk "extensionless names match" GLOBAL-make "$(run "$T/repo/Makefile")"
chk "dotfiles match by name" GLOBAL-dotfile "$(run "$T/repo/.clang-format")"
chk "names containing dots match" GLOBAL-cmake "$(run "$T/repo/CMakeLists.txt")"
chk "names containing spaces match" GLOBAL-notes "$(run "$T/repo/My Notes.md")"
chk "a comma list registers every rule" GLOBAL-notes "$(run "$T/repo/README.md")"

echo '# pass 2: glob, on the base name or the whole path'
chk "a base-name glob beats the extension" GLOBAL-pb2 "$(run "$T/repo/api_pb2.py")"
chk "a non-matching name falls through" GLOBAL-py "$(run "$T/repo/api.py")"
chk "a path glob disables a subtree" "disabled: a.rs" "$(run "$T/repo/vendor/x/a.rs")"

echo '# pass 3: extension'
chk "extensions are case insensitive" GLOBAL-rs "$(run "$T/other/A.RS")"
chk "only the last extension counts" GLOBAL-py "$(run "$T/other/a.tar.py")"
chk "a leading dot is a name, not an extension" "no rule matches: .rs" "$(run "$T/other/.rs")"
chk "an unknown extension does nothing" "no rule matches: a.xyz" "$(run "$T/other/a.xyz")"
chk "an unmatched bare name does nothing" "no rule matches: LICENSE" "$(run "$T/other/LICENSE")"

echo '# a broken config rolls back rather than half-applying'
printf 'not json at all' >"$T/repo/sub/deep/.fmt-hooks.json"
out=$(printf '{"tool_input":{"file_path":"%s"}}' "$T/repo/sub/deep/a.rs" |
  $clean PATH="$PATH" HOME="$HOME" CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_DRYRUN=1 "$hook")
chk "the file is reported under dryrun" "rejected (not an object" "$out"
chk "rules loaded before it survive" SUB-rs "$out"
printf '{"rs": ["echo","DEEP"], "toml": }' >"$T/repo/sub/deep/.fmt-hooks.json"
chk "a half-valid file is discarded whole" SUB-rs "$(run "$T/repo/sub/deep/a.rs")"
rm "$T/repo/sub/deep/.fmt-hooks.json"

echo '# no config at all means no formatting at all'
chk "an empty config dir is a no-op" "no rule matches: a.rs" \
  "$(printf '{"tool_input":{"file_path":"/none/a.rs"}}' |
    $clean PATH="$PATH" CLAUDE_CONFIG_DIR="$T/empty" CC_FMT_DRYRUN=1 "$hook")"

echo '# turning a rule off'
echo '{ "rs": null, "go": [] }' >"$T/off.json"
chk "null disables" "disabled: a.rs" "$(run "$T/other/a.rs" "$T/off.json")"
chk "an empty array disables" "disabled: a.go" "$(run "$T/other/a.go" "$T/off.json")"

echo '# ${VAR} substitution and a missing binary'
cat >"$T/var.json" <<'J'
{
  "rs": ["echo", "-n", "home=${HOME}"],
  "go": ["echo", "${CC_FMT_NO_SUCH_VAR}", "kept"],
  "py": ["${CC_FMT_NO_SUCH_VAR}", "x"],
  "lua": ["definitely-not-installed-xyz", "-w"],
  "sh": [""]
}
J
chk "a set variable is substituted" "home=$HOME" "$(run "$T/other/a.rs" "$T/var.json")"
chk "an unset variable drops the argument" kept "$(run "$T/other/a.go" "$T/var.json")"
chk "an unset variable in argv[0] aborts" 'unset ${VAR} in the program name' "$(run "$T/other/a.py" "$T/var.json")"
chk "a formatter that is not installed" "not on PATH: definitely-not-installed-xyz" "$(run "$T/other/a.lua" "$T/var.json")"
chk "an empty program name is not a program" "not on PATH: " "$(run "$T/other/a.sh" "$T/var.json")"

echo '# CC_FMT_TRUST fences off project configs'
chk "the project itself is trusted" PROJECT-rs "$(run "$T/repo/a.rs" "" "$T/repo")"
chk "a trusted parent covers it" PROJECT-rs "$(run "$T/repo/a.rs" "" "$T")"
chk "a trailing slash is tolerated" PROJECT-rs "$(run "$T/repo/a.rs" "" "$T/repo/")"
chk "an untrusted path keeps the global rules" GLOBAL-rs "$(run "$T/repo/a.rs" "" /nowhere)"
chk "and says so under dryrun" "project configs skipped" "$(run "$T/repo/a.rs" "" /nowhere)"
chk "a prefix must end on a separator" GLOBAL-rs "$(run "$T/repository/a.rs" "" "$T/repo")"
chk "one of several prefixes may match" NEIGHBOUR-rs "$(run "$T/repository/a.rs" "" "/nowhere:$T/repository")"
chk "CC_FMT_CONFIG is never fenced" OVERRIDE-rs "$(run "$T/repo/a.rs" "$T/override.json" /nowhere)"

echo '# the formatter really runs'
if command -v rustfmt >/dev/null && rustfmt --version >/dev/null 2>&1; then
  printf 'fn main(){let x=1;}\n' >"$T/repo/real.rs"
  echo '{ "rs": ["rustfmt", "--edition", "2024"] }' >"$T/real.json"
  printf '{"tool_input":{"file_path":"%s"}}' "$T/repo/real.rs" |
    $clean CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/real.json" "$hook"
  chk "the file is rewritten in place" '    let x = 1;' "$(cat "$T/repo/real.rs")"
else
  echo 'skip the file is rewritten in place (no usable rustfmt)'
fi

echo '# CC_FMT_ASYNC really does not wait'
# A formatter that takes 2 s, and records the process group it ran in.
cat >"$T/slowfmt" <<'F'
#!/bin/sh
ps -o pgid= -p $$ | tr -d ' ' >"$2.pgid"
sleep 2
printf 'formatted\n' >>"$2"
F
chmod +x "$T/slowfmt"
printf '{ "rs": ["%s", "x"] }\n' "$T/slowfmt" >"$T/slow.json"
mypgid=$(ps -o pgid= -p $$ | tr -d ' ')

# Command substitution reads the hook's stdout until EOF, so it blocks for as
# long as anything holds the write end -- which is exactly what an inherited
# descriptor would do. It is the shell's version of waiting for pipe close.
fast_or_slow() { [[ $1 -le 1 ]] && echo fast || echo "slow:${1}s"; }

# Redirected to a file there is no pipe, so this times the hook's exit alone.
printf 'orig\n' >"$T/async.rs"
t0=$SECONDS
printf '{"tool_input":{"file_path":"%s"}}' "$T/async.rs" |
  $clean CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/slow.json" CC_FMT_ASYNC=1 "$hook" >"$T/async.out"
chk "the hook process exits without waiting" fast "$(fast_or_slow $((SECONDS - t0)))"

# Command substitution instead reads stdout until EOF, so it blocks for as long
# as anything holds the write end -- which is what an inherited descriptor does.
# It is the shell's version of waiting for the pipe to close.
printf 'orig\n' >"$T/async2.rs"
t0=$SECONDS
_=$(printf '{"tool_input":{"file_path":"%s"}}' "$T/async2.rs" |
  $clean CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/slow.json" CC_FMT_ASYNC=1 "$hook")
chk "its stdio pipe closes with it" fast "$(fast_or_slow $((SECONDS - t0)))"
chk "the file is untouched on return" orig "$(cat "$T/async.rs")"
sleep 3
chk "the formatter still finished its work" formatted "$(cat "$T/async.rs")"
chk "an async formatter gets its own process group" separate \
  "$([[ "$(cat "$T/async.rs.pgid")" != "$mypgid" ]] && echo separate || echo same)"

printf 'orig\n' >"$T/sync.rs"
t0=$SECONDS
printf '{"tool_input":{"file_path":"%s"}}' "$T/sync.rs" |
  $clean CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/slow.json" "$hook"
sync_elapsed=$((SECONDS - t0))
chk "without it the hook waits" waited "$([[ $sync_elapsed -ge 2 ]] && echo waited || echo "returned in ${sync_elapsed}s")"
chk "and the work is done on return" formatted "$(cat "$T/sync.rs")"
chk "a synchronous formatter stays in the group" same \
  "$([[ "$(cat "$T/sync.rs.pgid")" == "$mypgid" ]] && echo same || echo separate)"

echo '# a rule may chain several formatters'
cat >"$T/chain.json" <<'J'
{
  "go": [["echo", "FIRST"], ["echo", "SECOND", "-x"], ["echo", "THIRD"]],
  "rs": [["echo", "ONLY"]],
  "py": [["definitely-not-installed-xyz", "-w"], ["echo", "AFTER-MISSING"]],
  "lua": [["${CC_FMT_NO_SUCH_VAR}"], ["echo", "AFTER-UNSET"]],
  "toml": [["definitely-not-installed-xyz"], ["${CC_FMT_NO_SUCH_VAR}"]]
}
J
out=$(run "$T/other/a.go" "$T/chain.json")
chk "every link is listed, in order" "FIRST $T/other/a.go"$'\n'"$(type -P echo) SECOND -x $T/other/a.go"$'\n'"$(type -P echo) THIRD $T/other/a.go" "$out"
chk "a one-link chain is a single command" "ONLY $T/other/a.rs" "$(run "$T/other/a.rs" "$T/chain.json")"
out=$(run "$T/other/a.py" "$T/chain.json")
chk "a missing program is reported" "not on PATH: definitely-not-installed-xyz" "$out"
chk "and the links after it still run" "AFTER-MISSING $T/other/a.py" "$out"
out=$(run "$T/other/a.lua" "$T/chain.json")
chk "an unset \${VAR} program is reported" 'unset ${VAR} in the program name' "$out"
chk "and the links after it still run too" "AFTER-UNSET $T/other/a.lua" "$out"
out=$(run "$T/other/a.toml" "$T/chain.json")
chk "a chain with nothing runnable runs nothing" "not on PATH" "$out"
chk "and prints no command line" nothing "$([[ "$out" == *"$T/other/a.toml"* ]] && echo printed || echo nothing)"

echo '# malformed chains reject the file; an over-long rule turns itself off'
for bad in '{"rs": [["echo","a"], []]}' '{"rs": ["echo", ["a"]]}' '{"rs": [["echo"], "a"]}' '{"rs": [[]]}' '{"rs": [["echo","a"],]}' '{"rs": ["echo",]}' '{"rs": [["echo" , ]]}'; do
  printf '%s' "$bad" >"$T/bad.json"
  out=$(run "$T/other/a.rs" "$T/bad.json")
  chk "rejected: $bad" "rejected (not an object" "$out"
  chk "  and the rule before it survives" GLOBAL-rs "$out"
done
long=$(printf '"x%d",' $(seq 1 40))
printf '{"rs": ["echo", %s "end"]}' "$long" >"$T/long.json"
out=$(run "$T/other/a.rs" "$T/long.json")
chk "an over-long rule is named" "rule too long, turned off: rs" "$out"
chk "  and turned off, not truncated" "disabled: a.rs" "$out"
printf '{"rs": [["echo","a"], ["echo", %s "end"]]}' "$long" >"$T/long.json"
chk "an over-long chain is turned off whole" "disabled: a.rs" "$(run "$T/other/a.rs" "$T/long.json")"
printf '{"rs, toml": ["echo", %s "end"]}' "$long" >"$T/long.json"
chk "  under every name it was given" "disabled: x.toml" "$(run "$T/other/x.toml" "$T/long.json")"
words() { # a single command of $1 tokens: ["echo","x2",...,"x$1"]
  local s='"echo"' i
  for ((i = 2; i <= $1; i++)); do s+=",\"x$i\""; done
  printf '{"rs": [%s]}' "$s"
}
words 31 >"$T/edge.json"
chk "a 31-token command fills the rule exactly" "x31 $T/other/a.rs" "$(run "$T/other/a.rs" "$T/edge.json")"
words 32 >"$T/edge.json"
chk "a 32-token command is one too many" "disabled: a.rs" "$(run "$T/other/a.rs" "$T/edge.json")"
links() { # $1 links of one word each: {"rs": [["s1"],["s2"],...]}
  local s='' i
  for ((i = 1; i <= $1; i++)); do s+="[\"s$i\"],"; done
  printf '{"rs": [%s]}' "${s%,}"
}
links 16 >"$T/wide.json"
chk "sixteen one-word links fit" "not on PATH: s16" "$(run "$T/other/a.rs" "$T/wide.json")"
links 17 >"$T/wide.json"
out=$(run "$T/other/a.rs" "$T/wide.json")
chk "seventeen are turned off" "rule too long, turned off: rs" "$out"
chk "  rather than rejected" "disabled: a.rs" "$([[ "$out" != *rejected* ]] && printf '%s' "$out" || echo rejected)"

echo '# ${VAR} inside a chain'
cat >"$T/chainvar.json" <<'J'
{
  "go": [["echo", "${CC_FMT_NO_SUCH_VAR}", "KEPT-IN-LINK"], ["echo", "NEXT-LINK"]],
  "rs": [["echo", "${CC_FMT_BIG}"], ["echo", "--x=${HOME}", "AFTER-BIG"]],
  "py": [["echo", "a=${CC_FMT_BIG}", "b=${CC_FMT_BIG}"], ["echo", "c=${CC_FMT_BIG}"], ["echo", "AFTER-TWO"]]
}
J
out=$(run "$T/other/a.go" "$T/chainvar.json")
chk "an unset variable drops one argument of a link" "echo KEPT-IN-LINK $T/other/a.go" "$out"
chk "  and the chain goes on" "NEXT-LINK $T/other/a.go" "$out"
export CC_FMT_BIG
CC_FMT_BIG=$(head -c 9000 /dev/zero | tr '\0' a)
out=$(run "$T/other/a.rs" "$T/chainvar.json")
chk "an expansion that cannot fit skips its whole command" 'too long after ${VAR} expansion, command skipped' "$out"
chk "  never a shortened one" nothing "$([[ "$out" == *"echo a"* ]] && echo shortened || echo nothing)"
chk "  and the next link still expands" "--x=$HOME AFTER-BIG $T/other/a.rs" "$out"
CC_FMT_BIG=$(head -c 3600 /dev/zero | tr '\0' a)
out=$(run "$T/other/a.py" "$T/chainvar.json")
chk "the arena fills across links" "command skipped" "$out"
chk "  the first link fits" "b=${CC_FMT_BIG} $T/other/a.py" "$out"
chk "  the third still runs" "AFTER-TWO $T/other/a.py" "$out"
unset CC_FMT_BIG

echo '# a chain really runs one link at a time'
# Link one takes 2 s before it writes, so a concurrent link two would win the
# race and land first. Each link also records its process group.
cat >"$T/step" <<'F'
#!/bin/sh
[ "$1" = one ] && sleep 2
ps -o pgid= -p $$ | tr -d ' ' >>"$2.pgid"
printf '%s\n' "$1" >>"$2"
F
chmod +x "$T/step"
printf '{ "rs": [["%s", "one"], ["%s", "two"]] }\n' "$T/step" "$T/step" >"$T/steps.json"

: >"$T/chain-sync.rs"
t0=$SECONDS
printf '{"tool_input":{"file_path":"%s"}}' "$T/chain-sync.rs" |
  $clean CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/steps.json" "$hook"
chk "synchronously, the hook waits for the whole chain" waited \
  "$([[ $((SECONDS - t0)) -ge 2 ]] && echo waited || echo returned)"
chk "  and the links ran in order" $'one\ntwo' "$(cat "$T/chain-sync.rs")"
chk "  in the hook's process group" same \
  "$([[ "$(sort -u "$T/chain-sync.rs.pgid")" == "$mypgid" ]] && echo same || echo separate)"

: >"$T/chain-async.rs"
t0=$SECONDS
_=$(printf '{"tool_input":{"file_path":"%s"}}' "$T/chain-async.rs" |
  $clean CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/steps.json" CC_FMT_ASYNC=1 "$hook")
chk "asynchronously, the hook and its pipes return at once" fast "$(fast_or_slow $((SECONDS - t0)))"
chk "  with nothing written yet" nothing "$([[ -s "$T/chain-async.rs" ]] && echo written || echo nothing)"
sleep 3
chk "  the links still ran in order" $'one\ntwo' "$(cat "$T/chain-async.rs")"
pgids=$(sort -u "$T/chain-async.rs.pgid")
chk "  sharing one process group of their own" separate \
  "$([[ $(wc -l <<<"$pgids") -eq 1 && "$pgids" != "$mypgid" ]] && echo separate || echo "pgids: ${pgids//$'\n'/,} mine: $mypgid")"

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[[ $fail -eq 0 ]]
