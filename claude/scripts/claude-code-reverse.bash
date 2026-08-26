#!/usr/bin/env bash
set -eo pipefail

# sudo /usr/bin/osascript ./binja_send_cmd_w.scpt

restore() {
  if [[ -e ../.agents ]]; then
    mv ../.agents ./
  fi
  if [[ -e ../ENV.md ]]; then
    mv ../ENV.md ./
  fi
}

handle_signal() {
  local sig="$1"
  echo "Received signal: ${sig}" >&2

  restore

  # Reset the trap handler and re-send the signal to the current process
  # (Propagates the correct exit status '128 + signal_number' to the parent process)
  trap - "$sig"
  kill -s "$sig" "$$"
}

mv ./.agents ./ENV.md ../

# Set up signal traps
# trap handle_signal EXIT
# trap 'handle_signal INT' INT
# trap 'handle_signal TERM' TERM

VER_TIMESTAMP=$(cat ../claude-code-reverse-hack/scripts/claude-code.timestamp.txt)
for vt in ${VER_TIMESTAMP}; do
  if [[ "$vt" == "#"* ]]; then
    break
  fi

  # while true; do
  #   read -r -p "Do extract $(echo -n "${vt}" | cut -d'|' -f 1) Claude Code binary? [y|n] " yn
  #   case $yn in
  #     [Yy]* ) break;;
  #     [Nn]* ) mv ../.agents ../ENV.md ./; exit 0;;
  #     * ) echo "Please answer [y] or [n]";;
  #   esac
  # done

  fd --max-depth=1 --exclude='.git' --exclude='binja_send_cmd_w.scpt' --exclude='.omx' --exclude='.omc' --exclude='CLAUDE.md' --exclude='.gitattributes' --exclude='.gitignore' --exclude='hack' --exclude='init.bash' --exec rm -rf {}
  mkdir tmp
  tar xf "../claude-code-reverse-hack/tarballs/claude-code-$(echo -n "${vt}" | cut -d'|' -f 1).tgz" -C ./tmp
  mv -f $(find ./tmp/package -mindepth 1 -maxdepth 1) ./
  rm -rf ./tmp

  jq 'del(.scripts.prepare)' package.json | sponge package.json
  npm i

  # open -a 'Binary Ninja.app' ./bin/claude.exe
  claude --effort=xhigh --model=claude-sonnet-5 --dangerously-skip-permissions --debug --verbose --include-partial-messages --output-format stream-json -p "/extract-cli-from-claude-exe"
  # claude --effort=xhigh --model=claude-sonnet-5 --dangerously-skip-permissions --debug --verbose --include-partial-messages --output-format stream-json -p "/oh-my-claudecode:execute Extract \`cli.js\` from the ./bin/claude.exe binary to @./cli.js without the binary_ninja_mcp MCP server"
  # omc --yolo --effort=xhigh --model=claude-sonnet-5 --output-format stream-json -p "/oh-my-claudecode:execute Extract \`cli.js\` from the ./bin/claude.exe binary to @./cli.js without the binary_ninja_mcp MCP server"
  NODE_OPTIONS='--max-old-space-size=8192' webcrack --no-jsx cli.js >cli.unpack.js && rm -f cli.js

  rm -rf node_modules package-lock.json

  git add .
  GIT_COMMITTER_DATE=$(echo -n "${vt}" | cut -d'|' -f 2) git commit -m "$(echo -n "${vt}" | cut -d'|' -f 1)"
  git tag --force -a "$(echo -n "${vt}" | cut -d'|' -f 1)" -m "$(echo -n "${vt}" | cut -d'|' -f 1)"
done

mv ../.agents ../ENV.md ./
