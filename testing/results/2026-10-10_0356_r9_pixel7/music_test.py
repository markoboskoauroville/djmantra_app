import subprocess, json, os, sys, time, re
repo = os.path.expanduser("~/Developer/djmantra_app"); R = os.path.join(repo, sys.argv[1]); SER = "41241FDH2S10SJ"
EXT = {"mp3","m4a","opus","wav","mp4","flac","ogg","aac","aiff","aif","mkv","webm","mov"}
def adb(*a, **k): return subprocess.run(["adb","-s",SER,*a], capture_output=True, text=True, **k)
files = [l for l in adb("shell","find /storage/emulated/0/Music -type f").stdout.splitlines()
         if l.rsplit(".",1)[-1].lower() in EXT and "/.djtest/" not in l and "djmantra-test-tone" not in l]
os.makedirs(R+"/files", exist_ok=True); adb("shell","mkdir -p /sdcard/Music/.djtest")
rows = []
for n, f in enumerate(files, 1):
    ext = f.rsplit(".",1)[-1].lower(); t = f"/sdcard/Music/.djtest/test.{ext}"
    size = adb("shell", f"stat -c %s '{f.replace(chr(39), chr(39)+chr(92)+chr(39)+chr(39))}'").stdout.strip()
    adb("shell","rm -f /sdcard/Music/.djtest/*")
    cp = adb("shell", "cp '" + f.replace("'", "'\\''") + "' " + t)
    adb("shell","am force-stop com.djmantra.app"); adb("logcat","-c")
    adb("shell","am start -n com.djmantra.app/org.qtproject.qt.android.bindings.QtActivity --es applicationArguments '--play " + t + "'")
    playing = False
    for _ in range(20):
        time.sleep(1)
        log = adb("logcat","-d","-s","DJMantra").stdout
        if "Deck 1 playing" in log: playing = True; break
    time.sleep(2)
    ears = subprocess.run(["python3","tools/android/observe.py","listen","--mic","1","--seconds","3","--out",f"{R}/files/{n:03d}"], cwd=repo, capture_output=True, text=True).stdout.strip().splitlines()
    try: e = json.loads(ears[-1])
    except Exception: e = {}
    log = adb("logcat","-d","-s","DJMantra").stdout
    peaks = re.findall(r"Output level: peak ([0-9.e-]+)", log)
    err = [l.split(": ",2)[-1][:160] for l in log.splitlines() if re.search(r"Couldn't load|Failed to open|Giving up|[Cc]ritical|could not", l)]
    loaded = playing or not any("load" in x.lower() for x in err)
    rows.append(dict(n=n, name=f.replace("/storage/emulated/0/Music/",""), ext=ext, size=size, copied=cp.returncode==0,
                     playing=playing, peak=peaks[-1] if peaks else "", mic=e.get("level_dbfs",""), err=err[0] if err else ""))
    print(n, len(files), rows[-1]["name"][:60], playing, rows[-1]["peak"], rows[-1]["mic"], flush=True)
    json.dump(rows, open(R+"/music-files.json","w"), indent=1)
adb("shell","rm -rf /sdcard/Music/.djtest"); adb("shell","am force-stop com.djmantra.app")
ok = [r for r in rows if r["playing"] and isinstance(r["mic"], (int,float)) and r["mic"] > -60]
with open(R+"/music-files.md","w") as o:
    o.write(f"# The phone's Music files through DJ Mantra (step F), {len(rows)} files\n\n")
    o.write(f"Played with sound on BlackHole (> -60 dBFS): **{len(ok)}**. Deck 1 playing in the log: {sum(r['playing'] for r in rows)}.\n")
    o.write("Each file copied to /sdcard/Music/.djtest/test.<ext> and started with --play; 3 s recorded from BlackHole after 'Deck 1 playing'.\n\n")
    from collections import Counter
    for ext, c in Counter(r["ext"] for r in rows).most_common():
        g=[r for r in rows if r["ext"]==ext]; o.write(f"- {ext}: {c} files, {sum(r in ok for r in g)} played\n")
    o.write("\n| # | File | Type | Size | Playing | Engine peak | BlackHole dBFS | Error |\n|---|---|---|---|---|---|---|---|\n")
    for r in rows:
        o.write(f"| {r['n']} | {r['name'].replace('|','/')} | {r['ext']} | {r['size']} | {'yes' if r['playing'] else '**no**'} | {r['peak']} | {r['mic']} | {r['err'].replace('|','/')} |\n")
print("DONE", len(rows), "played", len(ok))
