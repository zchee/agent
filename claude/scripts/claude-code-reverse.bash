#!/usr/bin/env bash
set -eo pipefail

mv ./.agents ./ENV.md ../

VER_TIMESTAMP=$(cat ../claude-code-reverse-hack/scripts/claude-code.timestamp.txt)
for vt in ${VER_TIMESTAMP}; do
  if [[ "$vt" == "#"* ]]; then
    break
  fi

  version="$(echo -n "${vt}" | cut -d'|' -f 1)"

  fd --max-depth=1 --exclude='.git' --exclude='.omx' --exclude='.omc' --exclude='CLAUDE.md' --exclude='.gitattributes' --exclude='.gitignore' --exclude='hack' --exclude='init.bash' --exec rm -rf {}
  mkdir tmp
  tar xf "../claude-code-reverse-hack/tarballs/claude-code-${version}.tgz" -C ./tmp
  mv -f $(find ./tmp/package -mindepth 1 -maxdepth 1) ./
  rm -rf ./tmp

  jq 'del(.scripts.prepare)' package.json | sponge package.json
  npm i

  claude --effort=xhigh --model=claude-sonnet-5 --dangerously-skip-permissions --debug --verbose --include-partial-messages --output-format stream-json -p "/extract-cli-from-claude-exe"
  NODE_OPTIONS='--max-old-space-size=8192' webcrack --no-jsx cli.js >cli.unpack.js && rm -f cli.js

  rm -rf node_modules package-lock.json

  git add .
  GIT_COMMITTER_DATE=$(echo -n "${vt}" | cut -d'|' -f 2) git commit -m "${version}"
  git tag --force -a "${version}" -m "${version}"
done

mv ../.agents ../ENV.md ./
