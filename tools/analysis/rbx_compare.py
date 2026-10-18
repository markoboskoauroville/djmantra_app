#!/usr/bin/env python3
"""Scores the rbxport port's raw results (tools/analysis/rbx_run.py) and the app's QM grid against
rekordbox 7 on Marko's test songs (djmantra-test-songs/rekordbox/rekordbox.xml: TEMPO = the grid,
Battito = the beat number; testing/sections/rekordbox.json: the phrases).

rekordbox's clock runs ~50 ms after ffmpeg's on these MP3s (it keeps the decoder delay; measured
18.10.2026: the port's beats sit on the kick's click in the decoded audio, rekordbox's 50 ms
later). ONE offset for every song and both grids (--offset, default: the port's median), so a grid
half a beat off still counts as off.
Usage: rbx_compare.py SONGS_DIR RAW_DIR OUT.md [--offset ms]"""
import json, os, re, sys, html, urllib.parse
import numpy as np

songs_dir, raw_dir, out_md = sys.argv[1:4]
fixed = float(sys.argv[sys.argv.index('--offset') + 1]) / 1000 if '--offset' in sys.argv else None
here = os.path.dirname(os.path.abspath(__file__))
sect = os.path.join(here, '..', '..', 'testing', 'sections')
xml = open(os.path.join(songs_dir, 'rekordbox', 'rekordbox.xml'), encoding='utf-8').read()
phr = json.load(open(os.path.join(sect, 'rekordbox.json')))
qm = {r['song']: r for r in json.load(open(os.path.join(sect, 'rekordbox_vs_mixxx.json')))}
tracks = {}
for m in re.finditer(r'<TRACK [^>]*?Location="([^"]+)"[^>]*>(.*?)</TRACK>', xml, re.S):
    name = os.path.basename(urllib.parse.unquote(html.unescape(m.group(1))))
    t = [(float(a), float(b), int(c)) for a, b, c in
         re.findall(r'<TEMPO Inizio="([\d.]+)" Bpm="([\d.]+)" Metro="[^"]*" Battito="(\d)"', m.group(2))]
    if t:
        tracks[name] = t

def rb_beats(tempos, end):
    beats, nums = [], []
    for i, (t0, bpm, bat) in enumerate(tempos):
        t1 = tempos[i + 1][0] if i + 1 < len(tempos) else end
        per, k = 60.0 / bpm, 0
        while t0 + k * per < t1 - 1e-6:
            beats.append(t0 + k * per); nums.append((bat - 1 + k) % 4 + 1); k += 1
    return np.array(beats), np.array(nums)

def near(a, b, tol):
    if len(a) == 0 or len(b) == 0:
        return np.zeros(len(a), bool)
    idx = np.clip(np.searchsorted(b, a), 1, len(b) - 1)
    return np.minimum(np.abs(a - b[idx - 1]), np.abs(a - b[idx])) <= tol

def offset_of(a, rb):
    idx = np.clip(np.searchsorted(rb, a), 1, len(rb) - 1)
    nearest = np.where(np.abs(a - rb[idx - 1]) < np.abs(a - rb[idx]), rb[idx - 1], rb[idx])
    return float(np.median(a - nearest))

# coarse classes for the names: rekordbox's phrase kinds and the page's names
def cls_rb(kind):
    k = kind.lower()
    return 'intro' if k.startswith('intro') else 'outro' if k.startswith('outro') else 'low' if k in ('down', 'bridge') else 'full'
def cls_ours(name):
    return {'INTRO': 'intro', 'OUTRO': 'outro', 'BREAKDOWN': 'low', 'BUILD-UP': 'low', 'AMBIENT': 'low'}.get(name, 'full')

songs = []
for name, tempos in sorted(tracks.items()):
    path = os.path.join(raw_dir, name + '.json')
    if os.path.exists(path):
        songs.append((name, tempos, json.load(open(path))))
if fixed is None:
    offs = []
    for name, tempos, r in songs:
        rb, _ = rb_beats(tempos, r['duration'])
        b = np.array(r['beats'])
        if len(tempos) == 1 and abs(r['bpm'] - tempos[0][1]) < 0.05 and len(b):
            offs.append(offset_of(b, rb))
    fixed = float(np.median(offs))

rows = []
for name, tempos, r in songs:
    rb, nums = rb_beats(tempos, r['duration'])
    b = np.array(r['beats']) - fixed
    lo, hi = max(rb[0], b[0]) - 0.05, min(rb[-1], b[-1]) + 0.05
    o = b[(b >= lo) & (b <= hi)]
    row = {'song': name, 'constant': len(tempos) == 1, 'rb_bpm': tempos[0][1], 'bpm': round(r['bpm'], 3),
           'beats': float(near(o, rb, 0.025).mean()) if len(o) else 0.0}
    d = b[r['first_downbeat']::4]; d = d[(d >= lo) & (d <= hi)]
    row['downbeats'] = float(near(d, rb[nums == 1], 0.025).mean()) if len(d) else 0.0
    q = qm.get(name)
    if q and q.get('mixxx_bpm'):
        per = 60.0 / q['mixxx_bpm']
        qb = q['mixxx_first'] + per * np.arange(int((r['duration'] - q['mixxx_first']) / per) + 1) - fixed
        qb = qb[(qb >= lo) & (qb <= hi)]
        row['qm_bpm'] = q['mixxx_bpm']
        row['qm_beats'] = float(near(qb, rb, 0.025).mean()) if len(qb) else 0.0
    p = phr.get(name)
    if p and row['constant']:
        ref = np.array([x['s'] for x in p['phrases']]) + 0.0
        bar = 240.0 / tempos[0][1]
        mine = np.array(r['phrases']) - fixed
        if len(mine) and len(ref):
            row['phr_prec'] = float(near(mine, ref, bar / 2).mean()); row['phr_rec'] = float(near(ref, mine, bar / 2).mean())
        # the names, time-weighted, in coarse classes
        ts = np.arange(0, r['duration'], 0.5)
        rk = [x['s'] for x in p['phrases']]; rn = [cls_rb(x['kind']) for x in p['phrases']]
        ok_ = [s[0] - fixed for s in r['sections']]; on = [cls_ours(s[1]) for s in r['sections']]
        if ok_ and rk:
            a = [rn[max(0, np.searchsorted(rk, t, 'right') - 1)] for t in ts]
            c = [on[max(0, np.searchsorted(ok_, t, 'right') - 1)] for t in ts]
            row['names_agree'] = float(np.mean([x == y for x, y in zip(a, c)]))
        row['mood'] = p.get('mood')
    rows.append(row)

def pct(xs):
    return '%d / %d (%.0f %%)' % (sum(xs), len(xs), 100.0 * sum(xs) / max(1, len(xs)))
const = [r for r in rows if r['constant']]
lines = ['# The rbxport port against rekordbox 7 (%d of Marko\'s songs, %d with a constant rekordbox grid)' % (len(rows), len(const)), '',
         'One offset for every song and both grids: %.1f ms (rekordbox\'s clock against ffmpeg\'s).' % (fixed * 1000), '',
         '| measure (constant rekordbox grids) | rbxport port | app now (QM) |', '|---|---|---|']
qmc = [r for r in const if 'qm_bpm' in r]
lines.append('| BPM within 0.05 | %s | %s |' % (pct([abs(r['bpm'] - r['rb_bpm']) < 0.05 for r in const]), pct([abs(r['qm_bpm'] - r['rb_bpm']) < 0.05 for r in qmc])))
lines.append('| BPM within 0.5 | %s | %s |' % (pct([abs(r['bpm'] - r['rb_bpm']) < 0.5 for r in const]), pct([abs(r['qm_bpm'] - r['rb_bpm']) < 0.5 for r in qmc])))
lines.append('| 90 %% of beats within 25 ms | %s | %s |' % (pct([r['beats'] >= 0.9 for r in const]), pct([r['qm_beats'] >= 0.9 for r in qmc])))
lines.append('| 90 %% of downbeats within 25 ms | %s | (no downbeat) |' % pct([r['downbeats'] >= 0.9 for r in const]))
pr = [r for r in const if 'phr_prec' in r]
if pr:
    lines.append('| phrase starts: precision / recall within half a bar | %.0f %% / %.0f %% | |' % (100 * np.mean([r['phr_prec'] for r in pr]), 100 * np.mean([r['phr_rec'] for r in pr])))
na = [r for r in const if 'names_agree' in r]
if na:
    lines.append('| names, time agreeing (intro / low / full / outro) | %.0f %% | |' % (100 * np.mean([r['names_agree'] for r in na])))
lines += ['', 'Per song: tools/analysis output JSON next to this file.']
open(out_md, 'w').write('\n'.join(lines) + '\n')
json.dump(rows, open(out_md.replace('.md', '.json'), 'w'), indent=1)
print('\n'.join(lines))
