#!/usr/bin/env bash

# UserPromptSubmit hook: switch to ABC only when a human is typing into this session.
# TISSelectInputSource is system-wide, so a prompt injected while the user is in
# another app (background-task notifications, cross-session messages, headless `claude -p`)
# would otherwise switch that app's input source.

[ "${CLAUDE_CODE_ENTRYPOINT:-cli}" = cli ] || exit 0

prompt=$(jq -r '.prompt // "" | .[0:64]')
case ${prompt} in
  '<task-notification>'* | '<cross-session-message'* | '<teammate-message'*) exit 0 ;;
esac

if [ -n "${TMUX_PANE:-}" ]; then
  [ "$(tmux display -p -t "${TMUX_PANE}" '#{pane_active}#{window_active}#{?session_attached,1,0}' 2> /dev/null)" = 111 ] || exit 0
fi

front=$(osascript -l JavaScript -e 'ObjC.import("AppKit"); $.NSWorkspace.sharedWorkspace.frontmostApplication.bundleIdentifier.js' 2> /dev/null)
[ "${front}" = "${IMECTL_HOOK_TERMINAL_BUNDLE_ID:-net.kovidgoyal.kitty}" ] || exit 0

exec imectl set -q com.apple.keylayout.ABC
