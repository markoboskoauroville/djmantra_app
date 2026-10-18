#!/usr/bin/env python3
"""Runs the rbxport port (tools/analysis/rbx_analyse) over every song of a folder, in parallel, and
keeps its raw output: OUT_DIR/<song>.json. Usage: rbx_run.py SONGS_DIR RBX_ANALYSE OUT_DIR [jobs]"""
import json, os, subprocess, sys, tempfile
from concurrent.futures import ThreadPoolExecutor
songs, exe, out = sys.argv[1:4]
jobs = int(sys.argv[4]) if len(sys.argv) > 4 else 4
os.makedirs(out, exist_ok=True)
def one(name):
    dst = os.path.join(out, name + '.json')
    if os.path.exists(dst):
        return name, 'cached'
    with tempfile.NamedTemporaryFile(suffix='.f32') as f:
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-i', os.path.join(songs, name), '-ac', '1', '-ar', '44100',
                        '-f', 'f32le', f.name], check=True)
        r = json.loads(subprocess.run([exe, f.name, '44100'], capture_output=True, text=True, check=True).stdout)
        r['duration'] = os.path.getsize(f.name) / 4 / 44100
    json.dump(r, open(dst, 'w'))
    return name, '%.2f BPM' % r['bpm']
names = sorted(n for n in os.listdir(songs) if n.lower().endswith(('.mp3', '.m4a', '.flac', '.wav', '.aiff')))
with ThreadPoolExecutor(jobs) as pool:
    for i, (name, msg) in enumerate(pool.map(one, names)):
        print(i + 1, len(names), name[:50], msg, flush=True)
