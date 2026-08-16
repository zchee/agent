---
name: serde-config-to-json-schema
description: Derive a JSON Schema (draft 2020-12) for a Rust application's config file from the serde types themselves, then prove it by replaying the crate's own test fixtures through it.
triggers:
  - "derive a JSON Schema for the config"
  - "generate config schema from the Rust code"
  - "write a $schema for ganja.jsonc / <app>.json"
  - "regenerate the config schema after config.rs changed"
  - "document the config file's shape authoritatively"
  - "editor autocomplete for our config file"
---

# Serde config → JSON Schema

Produce a JSON Schema that describes a Rust app's config file, deriving every
constraint from the code rather than from prose docs or a README. The output is
only trustworthy if it accepts what the loader accepts and rejects what the
loader rejects — so the last step is not "does the JSON parse" but "do the
crate's own positive and negative test fixtures classify identically".

Use this when a config struct is the source of truth and somebody wants editor
completion, a published schema, or an audit of the config surface.

## Inputs

- The config module (`config.rs` or equivalent) — the type carrying the
  top-level struct and its nested types.
- The consumer crates for any field the config module keeps as a **raw string
  or raw map**. Those are where the real vocabulary lives; the config crate
  deliberately does not know it.
- The crate's own tests, which are the fixture corpus for step 5.
- A target path for the schema file.

## Ordered steps

1. **Establish and record the state you derived from.** `git log --oneline -1`
   plus `git status --short <config file>`. If the file is dirty, say so — a
   schema derived from an uncommitted working tree is a schema whose provenance
   the reader has to be told. Re-check at the end: HEAD can move under you.

2. **Read the config module whole.** Not grep — whole. The doc comments carry
   the "absent means X" semantics that no type signature shows, and the
   load-time validator functions at the bottom of the file carry roughly half
   the real constraints.

3. **Enumerate, per type:** every field's serde name (check for `rename`,
   `rename_all`), whether the container has `deny_unknown_fields`, every
   `#[serde(default)]` and default fn with its **numeric value**, every enum's
   exact serde spellings, and every hand-written `Deserialize` impl (read the
   `Visitor`, not the derive — a hand-written one usually means the field takes
   two different shapes).

4. **Cross-check the consumers** for each raw-string / raw-map field. A config
   crate that says "which action names exist is the frontend's question" is
   telling you exactly which other crate to open. Cite `file:line` for each
   vocabulary you import, at the top of the `$def` — not per property.

5. **Map serde → schema** using the table below, then write the schema.

6. **Validate against the crate's own fixtures.** Lift positive cases from the
   "every key parses" style test and negative cases from the refusal tables
   (`check_*` unit tests). Run a real validator, not a JSON parse.

## The serde → JSON Schema mapping that actually matters

| Rust / serde | Schema | Why it is not the obvious thing |
|---|---|---|
| `#[serde(deny_unknown_fields)]` | `additionalProperties: false` | Absent it, the container is **open** — say so rather than guessing closed. |
| Enum with an `#[serde(untagged)] Other(String)` fallback | `anyOf: [{const A},{const B},{type: string}]` | A closed `enum` would **reject valid configs**. The anyOf still gives editors the suggestions. |
| Internally tagged enum (`#[serde(tag = "type")]`) | `oneOf` of variants, each with `type: {const: "..."}` | The variant struct's own `deny_unknown_fields` still accepts the tag — serde strips it before the struct sees it. Do not omit `type` from `properties`. |
| `Option<NonZeroU64>` | `type: integer, minimum: 1` | Zero is a parse error, not a "use the default". |
| `Option<bool>` read as `x != Some(false)` | `type: boolean, default: true` | The effective default is the **opposite** of `bool::default()`. Read the accessor method, never the field type. |
| `Option<bool>` read as `x == Some(true)` | `type: boolean, default: false` | Same trap, other direction. |
| Blank check via `.trim().is_empty()` | `pattern: "\\S"` | `minLength: 1` accepts `"  "`, which the loader refuses. |
| Validator refusing a name from a known list | `propertyNames: {not: {enum: [...]}}` | Encodes "an id this build already ships is refused" structurally instead of in prose. |
| Validator whose rule depends on the **key name** | `properties` for the known keys + `additionalProperties` for the rest, each with its own `if/then/else` | e.g. builtin servers inherit `extensions`, custom ones must declare them. |
| "Required unless disabled" | `if: {properties:{disabled:{const:true}}, required:["disabled"]}, else: {required:[...]}` | The `required: ["disabled"]` inside `if` is load-bearing — without it, an absent `disabled` makes the `if` vacuously true. |
| Default that depends on the **call site**, not the field | no `default` keyword; state the values in `description` | e.g. one timeout field whose fallback differs for a call vs a listing. A single `default` would be a lie. |
| Hand-written `Deserialize` accepting scalar-or-object | `anyOf: [scalar, object]` | The derive tells you nothing; the `Visitor`'s `visit_str` / `visit_map` tell you everything. |
| Map whose key order is semantic (last-match-wins) | plain object + a `description` saying so | JSON Schema cannot express order-significance. Say it in words rather than pretending. |

## Verification (the actual success criteria)

Write a throwaway `uv` script and run it. All four must hold:

1. `Draft202012Validator.check_schema(schema)` raises nothing.
2. Every positive fixture lifted from the crate's tests validates clean.
3. Every negative fixture lifted from the crate's refusal tables is rejected.
4. **Zero mismatches** — report the counts, not a vibe.

```python
#!/usr/bin/env -S uv run --quiet --script
# /// script
# dependencies = ["jsonschema>=4.23"]
# ///
import json
from jsonschema import Draft202012Validator

schema = json.load(open(SCHEMA_PATH))
Draft202012Validator.check_schema(schema)
v = Draft202012Validator(schema)

fail = 0
for name, doc in GOOD:            # from the "every key parses" test
    errs = sorted(v.iter_errors(doc), key=str)
    if errs:
        fail += 1
        print(f"UNEXPECTED REJECT [{name}]: {errs[0].message}")
for name, doc in BAD:             # from each check_* refusal table
    if v.is_valid(doc):
        fail += 1
        print(f"UNEXPECTED ACCEPT [{name}]")
print(f"{len(GOOD)} positive, {len(BAD)} negative, {fail} mismatches")
```

A schema that has not been round-tripped against the loader's own negative
cases is a guess with syntax highlighting.

## Pitfalls

- **Prose docs are not the authority; the code is.** Where a README and the
  struct disagree, the struct wins and the disagreement is worth reporting.
- **Half the constraints are not in the types.** `check_mcp`, `check_lsp`,
  `check_providers`, `check_hooks` — the load-time validators are where "empty
  command", "https or loopback", "not a builtin id" and "must be a valid regex"
  live. Grep for the functions the reader calls after deserializing.
- **Do not close a vocabulary the code leaves open.** An untagged fallback
  variant, or a map the config crate documents as "stays open so a config
  written for a later build still loads", must stay open in the schema.
- **`format` is annotation-only** in most validators (`uri`, `regex`). Use it
  for editor value, never as the thing enforcing a rule you care about.
- **Concurrent edits.** If another lane owns the config file, derive from what
  you read, state the commit, and re-check `git diff --stat` at the end to
  confirm the file did not move under you.
- **Respect file ownership.** Write only to the path you were given. Deriving a
  schema is a read-only act on the source repo.
- **Every judgment call gets reported.** Any place the code was ambiguous and
  you chose — list it with the reason in the final report, not silently in the
  file.

## Output contract

- The schema file at the requested path (`$schema`, `$id`, `title`,
  `description` naming the file's dialect, `$defs` per nested type).
- A table of top-level keys → type / default / vocabulary.
- An explicit list of ambiguities and what you chose.
- The validation counts.
- The commit/state derived from, and whether the source file was dirty.

## Open questions to resolve per project

- Should the schema be **generated** (via `schemars`) rather than hand-derived?
  Hand-derivation wins when the config type is not `JsonSchema`-derivable, when
  load-time validators carry constraints the derive cannot see, or when the
  descriptions are meant to be distilled prose. It loses on drift — so if the
  schema is checked in, pair it with a test that re-validates the fixtures.
- Where does the file live and who keeps it fresh when `config.rs` changes?
