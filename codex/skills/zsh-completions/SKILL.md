---
name: zsh-completions
description: Guide for creating effective skills. This skill should be used when users want to create, update, review, and debug native Zsh completion scripts built on the modern completion system (`compsys`). Use when Codex needs to create a new `_command` completion function, extend subcommand, option, or argument completion, fix completion parity against a CLI's current help output, migrate legacy `compctl` behavior to `#compdef` or `compdef`, or diagnose matcher, style, tag, or context issues in existing Zsh completion code.
metadata:
  short-description: Create or update a Zsh completion script
---

# Zsh Completions

Use this skill to work on native Zsh completion functions without falling back
to Bash-completion assumptions or ad hoc shell parsing.

## Workflow

1. Establish the contract before editing.
   - Identify the command name, aliases, install path, and existing completion
     file if one already exists.
   - Treat current CLI help, command source, or generated command metadata as
     the primary contract for options, subcommands, and argument shapes.
   - If a completion already exists, diff behavior before rewriting it.
2. Prefer modern `compsys` primitives.
   - Use `#compdef` or `compdef` for registration.
   - Start with `_arguments -C` for option and position parsing.
   - Use `->state` branches plus `case $state in ...` for nested subcommands or
     mode-specific arguments.
   - Drop to `compadd` or `compset` only when the higher-level helpers cannot
     express the behavior cleanly.
3. Reuse the standard helpers.
   - Use `_values` for enumerated values.
   - Use `_describe` for labeled groups.
   - Use `_alternative` when multiple completion sources are valid.
   - Use `_files` or `_path_files` for file and path completion.
   - Use `_guard` and `_message` for validation and clear user feedback.
4. Keep parsing state explicit.
   - Track `words`, `CURRENT`, `PREFIX`, `SUFFIX`, `opt_args`, `line`,
     `state`, and `curcontext` deliberately.
   - Use `compset` only when you truly need to strip prefixes or suffixes,
     split quoted text, or narrow the effective `words` range.
   - Do not hard-code fragile position checks when the CLI already has nested
     subcommands, repeated options, or value-bearing flags.
5. Separate semantics from user styling.
   - Keep completion behavior in the function.
   - Treat tags and `zstyle` as the user-facing configuration layer for
     ordering, matching, grouping, and formatting.
   - Do not ship user-specific `zstyle` defaults unless the owning project
     explicitly manages shell configuration too.
6. Validate in layers.
   - Run `zsh -n` on the completion file.
   - Load the function from the intended `fpath` in a clean Zsh process with
     `compinit`.
   - Run repository tests if the target project already has completion tests.
   - For parity work, compare the completion surface against the current CLI
     help or authoritative command definitions before declaring success.
7. Treat `compctl` as legacy.
   - Prefer `compsys` for new work and for touched code unless compatibility
     requirements say otherwise.
   - If legacy `compctl` behavior must stay, isolate that decision and explain
     it in the summary.

## Preferred Pattern

Use this as the default shape for a command with subcommands and
state-dependent arguments:

```zsh
#compdef mycmd

local curcontext="$curcontext" state line context
typeset -A opt_args

_arguments -C \
  '(-h --help)'{-h,--help}'[show help]' \
  '1:subcommand:->subcommand' \
  '*::argument:->argument' || return

case $state in
  subcommand)
    _describe -t commands 'mycmd subcommand' \
      'sync:synchronize state' \
      'status:show current status'
    ;;
  argument)
    case $words[2] in
      sync) _files ;;
      status) _arguments '--json[emit JSON]' ;;
    esac
    ;;
esac
```

Adapt the helpers to the command shape instead of forcing everything through
raw `compadd`.

## Debugging Rules

- When completion acts on the wrong token, inspect `words`, `CURRENT`,
  `PREFIX`, `IPREFIX`, `SUFFIX`, and `ISUFFIX`.
- When branches misfire, inspect `state`, `line`, `opt_args`, and
  `compstate[context]`.
- When candidates are missing or misordered, inspect `curcontext`, tags, and
  the active `zstyle` rules before changing the function logic.
- When matching is too strict or too loose, inspect `matcher-list`, `matcher`,
  and any explicit `compadd -M` usage.
- When autoloading fails, confirm the file name matches `_command`, the first
  line uses the correct `#compdef`, and the containing directory is in `fpath`
  before `compinit` runs.

## Validation Commands

Use the narrowest command that proves the change:

```zsh
zsh -n path/to/_mycmd
```

```zsh
zsh -fc '
  fpath=(path/to/completions $fpath)
  autoload -U compinit
  compinit -D
  autoload -U _mycmd
  whence -w _mycmd
'
```

If the completion is installed through a plugin manager or package layout,
repeat the load test from that exact install path.

## Reference

Read `references/compsys-reference.md` when you need exact reminders for:

- `#compdef`, `compdef`, `compinit`, `compaudit`, and `fpath`
- `words`, `CURRENT`, `PREFIX`, `IPREFIX`, `SUFFIX`, `ISUFFIX`, and
  `compstate`
- `compset`, `compadd`, matching control, tags, and styles
- legacy `compctl` behavior that must be interpreted or migrated
