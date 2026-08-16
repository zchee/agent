#!/usr/bin/env python3
"""Merge out/*.txt into units.ja.json, reporting every integrity defect.

Splits on the unit sentinel (never on newlines), prefers the CJK-bearing copy
when DeepL emits a sentinel twice, and compares the placeholder multiset of
every translated unit against its source.
"""
import glob
import json
import os
import re
import sys
from collections import Counter

D = os.path.dirname(os.path.abspath(__file__))
src = json.load(open(D + "/units.json"))
SENT = re.compile(r"⟦#(\d+)⟧")
PH = re.compile(r"⟦(\d+)⟧")

got = {}
dupes = []
for f in sorted(glob.glob(D + "/out/*.txt")):
    body = open(f).read()
    parts = SENT.split(body)
    # parts = [pre, id, text, id, text, ...]
    for i in range(1, len(parts), 2):
        n, txt = int(parts[i]), parts[i + 1]
        # A unit is one logical paragraph/cell: DeepL freely re-adds line
        # breaks inside it, so collapse every whitespace run to one space.
        txt = re.sub(r"\s+", " ", txt).strip()
        if n in got:
            dupes.append(n)
            cjk = sum(1 for c in txt if ord(c) >= 0x2E80)
            old = sum(1 for c in got[n] if ord(c) >= 0x2E80)
            if cjk <= old:
                continue          # keep the copy with more target-language text
        got[n] = txt

missing = [i for i in range(len(src)) if i not in got]
extra = [i for i in got if i >= len(src)]
bad = []
for i, s in enumerate(src):
    if i not in got:
        continue
    a, b = Counter(PH.findall(s)), Counter(PH.findall(got[i]))
    if a != b:
        lost = sorted((a - b).elements(), key=int)
        added = sorted((b - a).elements(), key=int)
        bad.append((i, lost, added))

print("units translated: %d/%d" % (len(got), len(src)))
if dupes:
    print("duplicate sentinels (resolved): %s" % sorted(set(dupes)))
if missing:
    print("MISSING units: %d (%s%s)" % (
        len(missing), missing[:12], " …" if len(missing) > 12 else ""))
if extra:
    print("EXTRA units: %s" % extra)
print("placeholder defects: %d" % len(bad))
for i, lost, added in bad:
    print("  unit %d  lost=%s  added=%s" % (i, lost, added))
    print("    src: %s" % src[i][:150])
    print("    ja : %s" % got[i][:150])

if "--write" in sys.argv and not missing and not bad:
    json.dump([got[i] for i in range(len(src))],
              open(D + "/units.ja.json", "w"), ensure_ascii=False, indent=0)
    print("wrote units.ja.json")
elif "--write" in sys.argv:
    print("NOT written — fix the defects above first")
