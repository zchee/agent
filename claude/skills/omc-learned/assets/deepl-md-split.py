#!/usr/bin/env python3
"""Split the en markdown into literal segments + translatable units.

Units carry protected spans replaced by U+27E6 N U+27E7 placeholders.
Emits units.json (list of protected source strings) and skel.json
(reconstruction plan).
"""
import json
import re
import sys

SRC = sys.argv[1]
OUT = sys.argv[2]

TAGS = r"SRC|OBS|INF|GANJA|CODEX|CLI|GROK-SRC|AGY-BIN|LIVE"

# Ordered: earlier patterns win.
PROT = [
    re.compile(r"``[^`]+``"),                    # double-backtick code
    re.compile(r"`[^`\n]+`"),                    # inline code
    re.compile(r"\[(?:" + TAGS + r")\]"),        # evidence tags
    re.compile(r"https?://[^\s)\]]+"),           # bare URLs
    re.compile(r"\bD\d{3}\b"),                   # decision ids
    re.compile(r"§[0-9]+(?:\.[0-9]+)*(?:-[0-9]+)?"),  # section refs
    re.compile(r"\b\d+\.\d+\.\d+\b"),            # version strings
    # Latin-kept terminology (established by the existing ja file). `lead`
    # matters most: DeepL otherwise renders it 先頭 / リードサイド / 主導.
    re.compile(r"\bteammates\b|\bteammate\b|\bworkers\b|\bworker\b|\binbox\b"
               r"|\blead\b|\bhooks\b|\bhook\b|\bworktree\b", re.I),
]

PLACE = "⟦{}⟧"
PLACE_RE = re.compile(r"⟦(\d+)⟧")


class Prot:
    """Global placeholder table shared by every unit."""

    def __init__(self):
        self.vals = []

    def sub(self, text):
        spans = []
        for pat in PROT:
            for m in pat.finditer(text):
                if any(not (m.end() <= a or m.start() >= b) for a, b in spans):
                    continue
                spans.append((m.start(), m.end()))
        spans.sort()
        out, last = [], 0
        for a, b in spans:
            out.append(text[last:a])
            self.vals.append(text[a:b])
            out.append(PLACE.format(len(self.vals) - 1))
            last = b
        out.append(text[last:])
        return "".join(out)


P = Prot()
UNITS = []


def unit(text):
    """Register a translatable unit, return its index."""
    if not text.strip():
        return None
    UNITS.append(P.sub(text))
    return len(UNITS) - 1


lines = open(SRC).read().split("\n")
segs = []          # reconstruction plan
i = 0
n = len(lines)

RE_H = re.compile(r"^(#{1,6})\s+(.*)$")
RE_HNUM = re.compile(r"^((?:\d+(?:\.\d+)*\.?|Appendix\s+[A-Z]\.)\s+)(.*)$")
RE_LI = re.compile(r"^(\s*)([-*+]|\d+[.)])\s+(.*)$")
RE_ALIGN = re.compile(r"^\|[\s:|-]+\|$")


def split_cells(row):
    """Split a table row on pipes, honouring inline-code spans."""
    parts, cur, tick = [], [], False
    for ch in row:
        if ch == "`":
            tick = not tick
        if ch == "|" and not tick:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append("".join(cur))
    return parts


def gather_para(start, stop_pred):
    """Join consecutive lines into one logical paragraph."""
    j = start
    buf = []
    while j < n and lines[j].strip() and not stop_pred(lines[j]):
        buf.append(lines[j].strip())
        j += 1
    return " ".join(buf), j


def is_special(l):
    return (l.startswith("```") or l.startswith("|") or l.startswith(">")
            or RE_H.match(l) or RE_LI.match(l) or l.rstrip() in ("---", "***")
            or l.startswith("    "))


while i < n:
    l = lines[i]

    # fenced code: verbatim, never sent
    if l.startswith("```"):
        j = i + 1
        while j < n and not lines[j].startswith("```"):
            j += 1
        segs.append({"t": "lit", "v": lines[i:min(j + 1, n)]})
        i = j + 1
        continue

    if not l.strip():
        segs.append({"t": "lit", "v": [""]})
        i += 1
        continue

    if l.rstrip() in ("---", "***", "___"):
        segs.append({"t": "lit", "v": [l]})
        i += 1
        continue

    m = RE_H.match(l)
    if m:
        hashes, txt = m.group(1), m.group(2)
        mn = RE_HNUM.match(txt)
        num, rest = (mn.group(1), mn.group(2)) if mn else ("", txt)
        segs.append({"t": "head", "p": hashes + " " + num, "u": unit(rest)})
        i += 1
        continue

    # table
    if l.startswith("|"):
        j = i
        rows = []
        while j < n and lines[j].startswith("|"):
            if RE_ALIGN.match(lines[j].strip()):
                rows.append({"lit": lines[j]})
            else:
                cells = split_cells(lines[j])
                rows.append({"cells": [
                    {"raw": c, "u": unit(c.strip())} for c in cells]})
            j += 1
        segs.append({"t": "table", "rows": rows})
        i = j
        continue

    # blockquote / callout
    if l.startswith(">"):
        j = i
        blk = []
        while j < n and lines[j].startswith(">"):
            blk.append(lines[j])
            j += 1
        items = []
        k = 0
        while k < len(blk):
            inner = re.sub(r"^>\s?", "", blk[k])
            if re.match(r"^\[![A-Z]+\]", inner.strip()) or not inner.strip():
                items.append({"lit": blk[k]})
                k += 1
                continue
            mli = RE_LI.match(inner)
            if mli:
                ind, mk, txt = mli.groups()
                buf = [txt.strip()]
                k += 1
                while k < len(blk):
                    nxt = re.sub(r"^>\s?", "", blk[k])
                    if (not nxt.strip() or RE_LI.match(nxt)
                            or re.match(r"^\[![A-Z]+\]", nxt.strip())):
                        break
                    if not nxt.startswith(" "):
                        break
                    buf.append(nxt.strip())
                    k += 1
                items.append({"pre": "> " + ind + mk + " ",
                              "cont": "> " + ind + " " * (len(mk) + 1),
                              "u": unit(" ".join(buf))})
                continue
            buf = [inner.strip()]
            k += 1
            while k < len(blk):
                nxt = re.sub(r"^>\s?", "", blk[k])
                if (not nxt.strip() or RE_LI.match(nxt)
                        or re.match(r"^\[![A-Z]+\]", nxt.strip())):
                    break
                buf.append(nxt.strip())
                k += 1
            items.append({"pre": "> ", "cont": "> ",
                          "u": unit(" ".join(buf))})
        segs.append({"t": "quote", "items": items})
        i = j
        continue

    # list item
    mli = RE_LI.match(l)
    if mli:
        ind, mk, txt = mli.groups()
        cont_ind = ind + " " * (len(mk) + 1)
        buf = [txt.strip()]
        j = i + 1
        while j < n and lines[j].strip():
            if (lines[j].startswith("```") or lines[j].startswith("|")
                    or lines[j].startswith(">") or RE_H.match(lines[j])
                    or RE_LI.match(lines[j])):
                break
            if not lines[j].startswith(" "):
                break
            buf.append(lines[j].strip())
            j += 1
        segs.append({"t": "li", "pre": ind + mk + " ", "cont": cont_ind,
                     "u": unit(" ".join(buf))})
        i = j
        continue

    # indented code block (4-space), verbatim
    if l.startswith("    "):
        j = i
        while j < n and (lines[j].startswith("    ") or not lines[j].strip()):
            j += 1
        while j > i and not lines[j - 1].strip():
            j -= 1
        segs.append({"t": "lit", "v": lines[i:j]})
        i = j
        continue

    # paragraph
    txt, j = gather_para(i, is_special)
    segs.append({"t": "para", "u": unit(txt)})
    i = j

json.dump(UNITS, open(OUT + "/units.json", "w"), ensure_ascii=False, indent=0)
json.dump({"segs": segs, "prot": P.vals},
          open(OUT + "/skel.json", "w"), ensure_ascii=False, indent=0)

chars = sum(len(u) for u in UNITS)
print(f"units={len(UNITS)} chars={chars} protected_spans={len(P.vals)} "
      f"segments={len(segs)}")
