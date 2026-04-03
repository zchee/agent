# Compsys Reference

Use this file when the main skill needs exact reminders about Zsh's completion
mechanics. Source material is the Zsh HTML manual under
`/opt/homebrew/share/zsh/htmldoc/`.

## Source Map

- `Completion-System.html`: modern completion system, `compinit`, `compdef`,
  contexts, styles, tags, and directory scanning.
- `Completion-Widgets.html`: completion widgets, special parameters,
  `compadd`, `compset`, condition codes, and widget examples.
- `Completion-System-Configuration.html`: context strings plus `zstyle` lookup
  behavior.
- `Completion-Matching-Control.html`: matcher syntax and matching control.
- `Completion-System-Variables.html`: helper-managed state such as
  `curcontext`, `state`, `line`, and `opt_args`.
- `Completion-Special-Parameters.html`: special parameters used by completion
  code.
- `Completion-Directories.html`: directory layout under `fpath`.
- `Completion-Using-compctl.html`: legacy completion system. Read only when
  touching old code or migrating it.

## Core Model

- `compsys` is the modern completion system. Prefer it over `compctl` for new
  work.
- Completion behavior is driven by context, tags, and styles.
- Tags describe what a match is, such as `files`, `directories`, or
  `commands`.
- Styles describe how completion behaves, usually through `zstyle`.
- The full style lookup context is shaped like:

```text
:completion:function:completer:command:argument:tag
```

- `_complete_help` is useful when you need to discover the contexts and tags
  the system is actually using.

## File Registration and Loading

- Put the completion function in a file whose name matches the function, such
  as `_mycmd`.
- Use a first line such as `#compdef mycmd alias1 alias2`.
- Use `#compdef -p` or `#compdef -P` when pattern-based dispatch is required.
- Use `compdef _mycmd mycmd` when attaching an existing function after load.
- `compinit` scans directories in `fpath`, reads the first line, and wires
  completion from the `#compdef` metadata.
- `compaudit` checks whether directories in the completion path are safe to
  load.
- Load-test with a clean shell instead of assuming the current session has the
  right `fpath` or dump state.

## Default Authoring Shape

For most commands, start with:

```zsh
local curcontext="$curcontext" state line context
typeset -A opt_args

_arguments -C \
  '(-h --help)'{-h,--help}'[show help]' \
  '1:subcommand:->subcommand' \
  '*::argument:->argument' || return
```

Then dispatch on `$state`.

Why this shape works:

- `_arguments -C` lets later helpers refine context.
- `opt_args` captures option values for flags that take arguments.
- `state` and `line` let you branch cleanly instead of hard-coding positions.
- `curcontext` is available when you need tag or style-sensitive behavior.

## High-Value Helpers

- `_arguments`: parse options, positions, and state transitions.
- `_values`: complete from a fixed set of values.
- `_describe`: show labeled groups of matches.
- `_alternative`: try multiple completion strategies in order.
- `_files`: complete filesystem entries with filtering support.
- `_path_files`: path-focused file completion.
- `_guard`: reject invalid prefixes before expensive completion.
- `_message`: show an explanatory message when there are no candidates.

Prefer these helpers before writing low-level `compadd` code.

## Special Parameters and State

Track these before patching logic:

- `words`: tokenized command line.
- `CURRENT`: current index in `words`.
- `PREFIX` and `SUFFIX`: editable text around the cursor in the current word.
- `IPREFIX` and `ISUFFIX`: ignored prefix or suffix that should stay outside
  the completion match.
- `QIPREFIX` and `QISUFFIX`: quoted variants of the ignored prefix or suffix.
- `compstate`: associative array describing runtime state such as `context`,
  `nmatches`, `insert`, `list`, `pattern_match`, `quote`, and `redirect`.

Common helper-managed variables from the completion system:

- `curcontext`: current style-lookup context.
- `state`: branch label returned by helpers such as `_arguments`.
- `line`: accumulated state from helper parsing.
- `opt_args`: associative array of parsed option arguments.

When debugging, print or trace these variables before changing the grammar.

## `compset` Cheatsheet

Use `compset` sparingly and intentionally.

- `compset -P pattern`: strip a prefix that should move into `IPREFIX`.
- `compset -S pattern`: strip a suffix that should move into `ISUFFIX`.
- `compset -q`: split quoted text into effective words for nested completion.
- `compset -n begin [end]`: narrow the active `words` range by index.
- `compset -N beg-pat [end-pat]`: narrow the active `words` range by pattern.

Good use cases:

- completing `--flag=value` after peeling off `--flag=`
- splitting comma- or space-like nested values only when helper parsing cannot
  model them directly
- narrowing completion to the subcommand tail after a sentinel word

Bad use cases:

- replacing normal `_arguments` state handling
- hiding parsing mistakes caused by an incorrect command grammar

## `compadd` Cheatsheet

Use `compadd` as the low-level primitive when helpers are not enough.

- Use `-M` for a matcher specification when behavior must differ from the
  active styles.
- Use `-X` for an explanation line.
- Use `-J` or `-V` for group names when explicit grouping is needed.

If you find yourself building many display strings by hand, re-check whether
`_describe`, `_values`, or `_alternative` is the cleaner tool.

## Matching Control

Prefer user-configurable matching through styles before hard-coding matcher
rules in the function.

- `matcher-list` is the usual user-facing control surface.
- `matcher` is a narrower style for specific contexts.
- `compadd -M` is the function-local override when styles are not enough.

Typical use cases:

- case-insensitive matching
- punctuation equivalence
- mild substring or character-class tolerance

Keep generous matching in styles when possible so users can tune it without
editing the completion function.

## Styles, Tags, and Context

- Tags determine which category of matches is under consideration.
- `tag-order` controls which tags are tried and in what order.
- `verbose`, `format`, `group-name`, and related styles change display and
  grouping rather than the command grammar itself.
- A missing or surprising match order may be a `zstyle` issue, not a code bug.

When a function behaves differently on one machine than another, inspect
`zstyle` state before changing logic.

## Directory Notes

- The completion system loads functions from directories listed in `fpath`.
- Function names and file names should stay aligned.
- Packaging, plugin managers, or dotfile loaders may prepend or append `fpath`,
  so validate from the real install path as well as the source tree.

## `compctl` Migration Notes

- `compctl` is the older completion system. New users should prefer the newer
  system.
- Treat `compctl` as a compatibility surface for legacy code or shell setups.
- When migrating, map old behavior into `#compdef`, `_arguments`, helper
  functions, and explicit `state` dispatch instead of re-creating a large
  `compctl` rule tree.
- Only preserve direct `compctl` usage when the surrounding environment still
  depends on it and replacement would be risky.

## Common Failure Modes

- The file name does not match the function name.
- The first line lacks a correct `#compdef`.
- The directory is not present in `fpath` before `compinit`.
- `compaudit` rejects an insecure directory.
- `_arguments` returns early because the grammar consumed the branch already.
- Positional logic is hard-coded even though the command is really
  state-driven.
- User `zstyle` rules are changing ordering, matching, or grouping.
