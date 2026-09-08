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
run() {
  printf '{"session_id":"s","tool_input":{"file_path":"%s"}}' "$1" |
    env PATH="$PATH" HOME="$HOME" CLAUDE_CONFIG_DIR="$T/cfgdir" \
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
  env PATH="$PATH" HOME="$HOME" CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_DRYRUN=1 "$hook")
chk "the file is reported under dryrun" "rejected (not an object" "$out"
chk "rules loaded before it survive" SUB-rs "$out"
printf '{"rs": ["echo","DEEP"], "toml": }' >"$T/repo/sub/deep/.fmt-hooks.json"
chk "a half-valid file is discarded whole" SUB-rs "$(run "$T/repo/sub/deep/a.rs")"
rm "$T/repo/sub/deep/.fmt-hooks.json"

echo '# no config at all means no formatting at all'
chk "an empty config dir is a no-op" "no rule matches: a.rs" \
  "$(printf '{"tool_input":{"file_path":"/none/a.rs"}}' |
    env PATH="$PATH" CLAUDE_CONFIG_DIR="$T/empty" CC_FMT_DRYRUN=1 "$hook")"

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
  "lua": ["definitely-not-installed-xyz", "-w"]
}
J
chk "a set variable is substituted" "home=$HOME" "$(run "$T/other/a.rs" "$T/var.json")"
chk "an unset variable drops the argument" kept "$(run "$T/other/a.go" "$T/var.json")"
chk "an unset variable in argv[0] aborts" 'unset ${VAR} in the program name' "$(run "$T/other/a.py" "$T/var.json")"
chk "a formatter that is not installed" "not on PATH: definitely-not-installed-xyz" "$(run "$T/other/a.lua" "$T/var.json")"

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
    env CLAUDE_CONFIG_DIR="$T/cfgdir" CC_FMT_CONFIG="$T/real.json" "$hook"
  chk "the file is rewritten in place" '    let x = 1;' "$(cat "$T/repo/real.rs")"
else
  echo 'skip the file is rewritten in place (no usable rustfmt)'
fi

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[[ $fail -eq 0 ]]
