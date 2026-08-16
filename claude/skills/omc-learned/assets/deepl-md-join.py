#!/usr/bin/env python3
"""Rebuild the markdown from skel.json + a units file, re-wrapping prose.

usage: mdjoin.py <dir> <units-file.json> <out.md>
"""
import json
import re
import sys
import unicodedata

D, UF, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
skel = json.load(open(D + "/skel.json"))
segs, prot = skel["segs"], skel["prot"]
tr = json.load(open(UF))

PLACE_RE = re.compile(r"⟦\s*(\d+)\s*⟧")
WRAP = 78


def restore(s):
    return PLACE_RE.sub(lambda m: prot[int(m.group(1))], s)


def text(u):
    return restore(tr[u]) if u is not None else ""


def cols(s):
    return sum(2 if unicodedata.east_asian_width(c) in "WF" else 1 for c in s)


ATOM = re.compile(r"``.+?``|`[^`\n]+`|\[[^\]\n]*\]\([^)\n]*\)"
                  r"|https?://[^\s)\]]+")
# no break BEFORE these; no break AFTER the openers
NO_BREAK_BEFORE = "。、，．）」』】〕｝〉》・？！：；ー〜%,.);:!?」』]}*"
NO_BREAK_AFTER = "（「『【〔｛〈《([{*"


def is_cjk(c):
    return bool(c) and ord(c) >= 0x2E80


def tokens(s):
    """(token, may_break_before) pairs."""
    out = []
    i, ln = 0, len(s)
    while i < ln:
        m = ATOM.match(s, i)
        if m:
            a = m.group(0)
            # A long code span or link is atomic against arbitrary breaks, but
            # a space already inside it is a legal break point: a space and a
            # newline render identically, so this changes nothing.
            if " " in a and cols(a) > 40:
                for k, piece in enumerate(a.split(" ")):
                    if k:
                        out.append([" ", True])
                    out.append([piece, True])
            else:
                out.append([a, True])
            i = m.end()
            continue
        c = s[i]
        if c == " ":
            i += 1
            if out:
                out[-1][1] = out[-1][1]
            # mark next token breakable, and record the space
            if i < ln:
                out.append([" ", True])
            continue
        if ord(c) < 0x2E80 and c not in "⟦⟧":
            j = i
            while j < ln and s[j] != " " and ord(s[j]) < 0x2E80 \
                    and not ATOM.match(s, j):
                j += 1
            out.append([s[i:j], not out or out[-1][0] == " "])
            i = j
            continue
        out.append([c, True])
        i += 1
    return out


def wrap(s, first, cont, width=WRAP):
    s = s.strip()
    if not s:
        return [first.rstrip()] if first.strip() else []
    toks = tokens(s)
    lines, cur, pre = [], "", first
    for k, (t, _brk) in enumerate(toks):
        if t == " ":
            continue
        prev = toks[k - 1][0] if k else ""
        sep = " " if (k and toks[k - 1][0] == " ") else ""
        # Break only where the source had a space, or at a CJK boundary
        # (either side). Breaking between two glued ASCII tokens (`a`/`b`)
        # would render as an inserted space — a real content change.
        can = cur.strip() != "" and (
            sep == " " or is_cjk(prev[-1:]) or is_cjk(t[:1]))
        if can and prev and prev[-1] in NO_BREAK_AFTER:
            can = False
        if can and t and t[0] in NO_BREAK_BEFORE:
            can = False
        cand = cur + sep + t
        if can and cols(pre + cand) > width and cur.strip():
            lines.append((pre + cur).rstrip())
            pre, cur = cont, t
        else:
            cur = cand
    if cur.strip():
        lines.append((pre + cur).rstrip())
    return lines


out = []
for s in segs:
    t = s["t"]
    if t == "lit":
        out.extend(s["v"])
    elif t == "head":
        out.append((s["p"] + text(s["u"])).rstrip())
    elif t == "para":
        out.extend(wrap(text(s["u"]), "", ""))
    elif t == "li":
        out.extend(wrap(text(s["u"]), s["pre"], s["cont"]))
    elif t == "quote":
        for it in s["items"]:
            if "lit" in it:
                out.append(it["lit"])
            else:
                out.extend(wrap(text(it["u"]), it["pre"], it["cont"]))
    elif t == "table":
        for r in s["rows"]:
            if "lit" in r:
                out.append(r["lit"])
            else:
                cs = []
                for c in r["cells"]:
                    if c["u"] is None:
                        cs.append(c["raw"])
                    else:
                        pad = " " if c["raw"].startswith(" ") else ""
                        pad2 = " " if c["raw"].endswith(" ") else ""
                        cs.append(pad + text(c["u"]).strip() + pad2)
                out.append("|".join(cs))

open(OUT, "w").write("\n".join(out).rstrip("\n") + "\n")
print("wrote", OUT, len(out), "lines")
