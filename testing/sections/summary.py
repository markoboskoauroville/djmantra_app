"""Totals of a bench run: python3 -I summary.py OUTDIR  (the JSON files sections.py wrote)."""
import glob
import json
import os
import sys

rows = []
for p in sorted(glob.glob(os.path.join(sys.argv[1], "*.json"))):
    d = json.load(open(p))
    if "v2_vs_ref_major" in d:
        rows.append(d)
tot = {k: {"exact": 0, "off_1_4": 0, "missed": 0, "false": 0} for k in ("app", "v2")}
for d in rows:
    for k, f in (("app", "app_vs_ref_major"), ("v2", "v2_vs_ref_major")):
        for m, v in d[f].items():
            tot[k][m] += v
ref = sum(len(d["ref_major_bar"]) for d in rows)
print(f"{len(rows)} songs, {ref} reference major sections")
for k in ("app", "v2"):
    t = tot[k]
    print(f"  {k:3s}: exact {t['exact']} ({100 * t['exact'] / max(1, ref):.0f} %), off 1-4 bars {t['off_1_4']}, "
          f"missed {t['missed']}, false {t['false']}")
bad = lambda d: d["v2_vs_ref_major"]["missed"] + d["v2_vs_ref_major"]["false"] + 0.5 * d["v2_vs_ref_major"]["off_1_4"]
print("worst for v2:")
for d in sorted(rows, key=bad, reverse=True)[:10]:
    print(f"  {d['song'][:40]:40s} {d['bpm']:6.1f} BPM ref {d['ref_major_bar']} v2 {d['v2_major_bar']} {d['v2_vs_ref_major']}")
