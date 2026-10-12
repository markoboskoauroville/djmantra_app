import json, os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

songs, kf, out = sys.argv[1:4]
files = sorted(f for f in os.listdir(songs) if f.lower().endswith('.mp3'))

def key(f):
    dec = subprocess.Popen(['ffmpeg', '-v', 'error', '-i', os.path.join(songs, f), '-ac', '1', '-ar', '44100',
                            '-f', 'f32le', '-'], stdout=subprocess.PIPE)
    r = subprocess.run([kf], stdin=dec.stdout, capture_output=True, text=True)
    dec.wait()
    return f, r.stdout.strip()

with ThreadPoolExecutor(8) as ex:
    result = dict(ex.map(key, files))
json.dump(result, open(out, 'w'), indent=1, ensure_ascii=False)
print(len(result), 'keys ->', out, '| empty:', [f for f, k in result.items() if not k])
