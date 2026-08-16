---
name: deepl-mcp-markdown-translation
description: Translate a large structured Markdown document with the DeepL MCP server without damaging its structure, using sentinel-delimited batches, U+27E6 placeholders, and an identity round-trip proof.
triggers:
  - translate a markdown doc with DeepL
  - regenerate the .ja.md from the .en.md
  - mcp__deepl__translate-text on a whole file
  - translate-document refused MARKDOWN
  - keep the translation's tables/code fences/tags intact
---

# DeepL MCP → structured Markdown translation

Translating a long reference document (tables, fenced code, callouts, evidence
tags, section refs) through the **DeepL MCP server** while guaranteeing the
translated file has *exactly* the same structure as the source.

Discovered translating `docs/references/claude-teammates.en.md` (2183 lines,
122 KB, 60 fences, 133 table rows, 121 §-refs) into its `.ja.md` counterpart.

## When to use

- A `<doc>.<lang>.md` pair must be regenerated from its source-language twin.
- The user requires **DeepL specifically** as the engine (not your own prose).
- The document is too large / too structured to translate by hand safely.

Do **not** use for short prose with no structure — a single `translate-text`
call is enough there.

## Hard-won facts about this MCP server (verify, but expect these)

1. **`translate-document` can refuse Markdown outright.** Observed error:
   `Authorization failure … You are not allowed to translate MARKDOWN files`.
   The API key still works (`list-glossaries` succeeds) — it is a per-account
   document-type entitlement. Do not retry it on `.md`; go to `translate-text`.
2. **The MCP `translate-text` schema is NOT the REST API's.** It exposes only
   `text, targetLangCode, sourceLangCode, context, customInstructions,
   formality, glossaryId, styleId, preserveFormatting, splitSentences`.
   There is **no `tag_handling`, `ignore_tags`, `non_splitting_tags`, or
   `outline_detection`.** Every XML-tag-protection recipe from DeepL's docs is
   therefore unavailable. Plan for placeholders instead.
3. **Array `text` results come back concatenated with no separator.** Passing
   `text: [a, b, c]` returns one blob you cannot split. **Arrays are unusable
   for multi-segment work.** Send one string with your own sentinels.
4. **Placeholder survival (probe these before trusting them):**

   | Syntax | Result |
   |---|---|
   | `<x0/>` | ✗ rewritten to `<x0></x0>` |
   | `{0}` | ~ survives, erratic surrounding spaces |
   | `⟦0⟧` (U+27E6/U+27E7) | ✓ **verbatim, in order, every time** |

5. **Inline Markdown survives untouched** — backticks, `**bold**`, `*italic*`,
   `§10.9`, and `[text](url)` (URL kept, link text translated). Do not waste
   placeholders protecting emphasis; protect *content*, not syntax.
6. `formality: "less"` → Japanese plain form (である/だ). `"more"` → です・ます.
7. With no account glossary or style rule, inline `customInstructions`
   (max 10 × 300 chars) is the substitute — and it does measurably work.

## Workflow

### 1. Probe first, always

One `translate-text` call containing the *same* sample paragraph rendered in
each candidate placeholder syntax, plus one copy with raw Markdown left in.
Read the result and pick the syntax that came back byte-identical. Never run
the full document on an unprobed assumption.

Also call `list-glossaries` / `list-style-rules` and **use any existing
en→target glossary or style rule**; record its name/id in your report. Do not
create new ones.

### 2. Mine the existing translation for terminology

The file you are about to overwrite is the terminology spec. Count, per term,
whether the established translation keeps it in Latin script or renders it:

```sh
for t in teammate lead worker pane inbox mailbox frame turn queue hook; do
  printf '%-10s en=%-4s ja=%-4s\n' "$t" \
    "$(grep -oiw "$t" doc.en.md | wc -l)" "$(grep -oiw "$t" doc.ja.md | wc -l)"
done
```

A term with a high count in **both** files is kept in Latin → **protect it as
a placeholder** so DeepL cannot touch it. A term with ~0 count in the target
file is translated → leave it exposed and pin its rendering via
`customInstructions`.

> This is the highest-leverage step. `lead` unprotected came back as
> 先頭 / リードサイド / 主導 in three different sentences of one batch.

### 3. Split → translate → join, with an identity proof

Use `assets/deepl-md-split.py` and `assets/deepl-md-join.py` (saved beside
this skill).

- **split** parses the source into literal segments (fences, alignment rows,
  rules — never sent) and translatable units (heading text after the `#`s and
  the numbering, table cells, list-item text after the marker, blockquote
  inner text, paragraphs), replacing protected spans with `⟦N⟧`.
- **join** restores `⟦N⟧`, re-emits the skeleton, and re-wraps prose.

**Before sending one character to the API, run the identity round-trip:**

```sh
python3 deepl-md-split.py doc.en.md .
cp units.json units.identity.json          # "translate" to itself
python3 deepl-md-join.py . units.identity.json rt.en.md
# normalize wrapping, then diff — MUST be 0 differing lines
```

If the round-trip is not lossless, your parser is wrong and the translation
will be wrong in ways you cannot see through a foreign language. This test
caught three real wrapper bugs (breaking inside ``` ``…`` ``` spans, breaking
after `(`, and breaking between `**` and CJK content).

### 4. Batch with two-level sentinels

Join units into ~5–6 KB requests, one unit per line:

```
⟦#0⟧<unit 0 text with ⟦0⟧ ⟦1⟧ placeholders>
⟦#1⟧<unit 1 text…>
```

`⟦#N⟧` = unit boundary, `⟦N⟧` = protected span. Split the response on the
**sentinel**, never on newlines — DeepL freely merges and re-adds line breaks.

Standard call parameters:

```
targetLangCode: ja, sourceLangCode: en
preserveFormatting: true, splitSentences: "1", formality: "less"
context: <one sentence naming the domain and the register>
customInstructions: [plain-form rule, placeholder-integrity rule,
                     one-unit-per-line rule, markdown-byte-identity rule,
                     term-rendering table]
```

### 5. Expect and handle these response defects

- **A sentinel emitted twice** — once with the untranslated source, once
  translated. Take the copy containing target-language characters.
- **A duplicated clause** inside one unit, with its placeholders repeated.
  Detect by comparing the placeholder *multiset* per unit against the source.
- **Merged units** on one output line — harmless, the sentinel split handles it.

≤3 attempts per batch on transient errors; on quota errors wait and shrink.
If the service stays down, **report the honest failure and commit nothing**.

### 6. Bounded post-pass only

Permitted: (a) normalize recurring terms to the established renderings,
(b) enforce the target register, (c) repair structural damage. **Do not
rewrite DeepL's sentences** — the engine is the deliverable. Keep a count per
fix category for the report.

### 7. Wrapping rule that is easy to get wrong

Re-wrap prose to ~78 **display columns** (East Asian Wide = 2). Break **only**
at a real space or at a CJK boundary. Breaking between two glued ASCII tokens
(`` `a`/`b` ``) inserts a rendered space — a genuine content change, not a
cosmetic one. Honor kinsoku (no line starting with 。、）」, none ending with
（「), and never break inside inline code, a link, or between `**` and its
content.

## Success criteria

Verify all of these before committing, fence-aware (exclude code blocks):

- heading list identical (same count, same numbering) source vs target
- code-fence count equal, fence contents byte-identical
- per-table row and column counts equal
- every evidence tag / decision id / §-ref appears the **same number of times**
- zero leftover placeholders — grep for `⟦`
- no line of untranslated source-language prose
- no secrets introduced; prose wrapped ~78 display columns

## Pitfalls

- Sending arrays and trying to split the reply (finding #3) — silent data loss.
- Trusting `tag_handling` because DeepL's public docs describe it (finding #2).
- Translating fenced code, or letting comments inside fences drift.
- Overwriting the target file before mining it for terminology (step 2).
- Protecting a term that is used as a **verb** (`spawn`, `steer`, `queue`) —
  the placeholder becomes an opaque noun and the grammar breaks. Protect
  unambiguous nouns only; fix verbs in the post-pass.
- `cp` may be aliased `-i` — use `/bin/cp -f` inside scripts or it hangs.
- Cost: the text crosses your context ~3× (read batch, send batch, write
  reply). ~90 KB of source ≈ 200 K tokens end to end. Budget for it.

## Two traps in the verification itself

- **`\b` does not work against CJK.** Python's `\w` includes CJK, so
  `\bD\d{3}\b` silently fails to match `D492以降` and your checker will report
  content loss that isn't there. Use a negative lookahead (`D\d{3}(?![0-9])`).
- **Count asterisks, not just `**`.** Bold can balance while a single `*…*`
  italic pair is quietly dropped. Compare the total `*` count too.

## Assets

- `assets/deepl-md-split.py` — Markdown → units.json + skel.json
- `assets/deepl-md-join.py` — units + skeleton → re-wrapped Markdown
- `assets/deepl-md-merge.py` — out/*.txt → units.ja.json, reporting every
  missing unit, duplicate sentinel and placeholder-multiset mismatch

All are generic over the protection list; edit the `PROT` table in the
splitter to add project-specific protected terms. The joiner breaks a long
inline-code span only at spaces already inside it — the one break that costs
nothing, since a space and a newline render the same.
