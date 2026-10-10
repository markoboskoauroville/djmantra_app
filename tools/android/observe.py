#!/usr/bin/env python3
"""The test loop's eyes and ears on Marko's Mac (macOS, ffmpeg with avfoundation).

The phone plays through its speaker, the Mac's microphone listens; a camera
looks at the Hercules DJControl Mix Ultra, so the controller's lights can be
checked. Plain Python 3 (no numpy, no PIL); ffmpeg does the capturing.

  observe.py devices
      the Mac's cameras and microphones with their numbers
  observe.py listen --mic 0 --seconds 4 --out DIR [--expect 440]
      records, writes listen.wav and listen.json: level (dBFS), peak, the
      strongest frequency; with --expect: pass when that tone is heard
  observe.py look --camera 0 --out DIR [--name frame]
      one picture (frame.jpg) and a small grey copy for comparing (frame.gray)
  observe.py lights --before DIR/a.gray --after DIR/b.gray --out DIR
      what changed between two pictures: changed.json (the cells of a 32x18
      grid that got brighter or darker) and changed.png-free text map
  observe.py session --camera 0 --mic 0 --out DIR [--expect 440]
      look, listen and look again in one go (the usual check after an action)

Exit code 0 when every check passes, 1 when one fails, 2 on a capture error.
The first use asks macOS for camera and microphone access for the terminal
(System Settings > Privacy & Security): that needs Marko once.
"""

import argparse
import cmath
import json
import math
import os
import struct
import subprocess
import sys
import wave

GRID_W, GRID_H = 32, 18
SMALL_W, SMALL_H = 160, 90


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def devices():
    r = run(["ffmpeg", "-hide_banner", "-f", "avfoundation", "-list_devices", "true", "-i", ""])
    lines = [l.split("] ", 1)[-1] for l in r.stderr.splitlines() if "AVFoundation" in l]
    print("\n".join(lines))
    return 0


# -- ears ------------------------------------------------------------------------


def record(mic, seconds, path):
    r = run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-f", "avfoundation",
             "-i", ":%s" % mic, "-t", str(seconds), "-ac", "1", "-ar", "48000",
             "-sample_fmt", "s16", path])
    if r.returncode != 0 or not os.path.exists(path):
        print("recording failed:", r.stderr.strip()[-400:], file=sys.stderr)
        return False
    return True


def fft(values):
    n = len(values)
    if n == 1:
        return [values[0]]
    even, odd = fft(values[0::2]), fft(values[1::2])
    out = [0] * n
    for k in range(n // 2):
        t = cmath.exp(-2j * math.pi * k / n) * odd[k]
        out[k], out[k + n // 2] = even[k] + t, even[k] - t
    return out


def analyse(path):
    with wave.open(path) as w:
        rate, frames = w.getframerate(), w.readframes(w.getnframes())
    samples = [s / 32768.0 for s in struct.unpack("<%dh" % (len(frames) // 2), frames)]
    # the first 0.3 s can hold the device starting up
    samples = samples[int(0.3 * rate):] or samples
    rms = math.sqrt(sum(s * s for s in samples) / max(1, len(samples)))
    peak = max((abs(s) for s in samples), default=0.0)
    # strongest frequency: a Hann-windowed FFT over 16384 samples (about 3 Hz bins)
    n = 16384
    start = max(0, len(samples) // 2 - n // 2)
    block = samples[start:start + n]
    block += [0.0] * (n - len(block))
    window = [0.5 - 0.5 * math.cos(2 * math.pi * i / (n - 1)) for i in range(n)]
    spectrum = fft([b * w for b, w in zip(block, window)])
    magnitudes = [abs(c) for c in spectrum[: n // 2]]
    low = int(40 * n / rate)  # ignore rumble below 40 Hz
    k = max(range(low, len(magnitudes)), key=lambda i: magnitudes[i])
    # parabolic interpolation for a finer frequency
    if 0 < k < len(magnitudes) - 1:
        a, b, c = magnitudes[k - 1], magnitudes[k], magnitudes[k + 1]
        d = (a - c) / (2 * (a - 2 * b + c)) if (a - 2 * b + c) else 0.0
    else:
        d = 0.0
    strongest = (k + d) * rate / n
    noise = sorted(magnitudes[low:])[len(magnitudes[low:]) // 2] or 1e-12
    return dict(rate=rate, seconds=round(len(samples) / rate, 2),
                level_dbfs=round(20 * math.log10(rms), 1) if rms > 0 else -120.0,
                peak_dbfs=round(20 * math.log10(peak), 1) if peak > 0 else -120.0,
                strongest_hz=round(strongest, 1),
                tone_over_noise_db=round(20 * math.log10(magnitudes[k] / noise), 1) if magnitudes[k] > 0 else 0.0)  # BlackHole gives exact digital silence


def listen(args):
    os.makedirs(args.out, exist_ok=True)
    path = os.path.join(args.out, "listen.wav")
    if not record(args.mic, args.seconds, path):
        return 2
    result = analyse(path)
    if args.expect:
        near = abs(result["strongest_hz"] - args.expect) <= max(4.0, 0.03 * args.expect)
        result["expect_hz"] = args.expect
        result["heard"] = bool(near and result["tone_over_noise_db"] >= 15)
    if args.silence:
        result["silent"] = result["level_dbfs"] < args.silence
    with open(os.path.join(args.out, "listen.json"), "w") as f:
        json.dump(result, f, indent=1)
    print(json.dumps(result))
    if args.expect and not result["heard"]:
        return 1
    if args.silence and not result["silent"]:
        return 1
    return 0


# -- eyes ------------------------------------------------------------------------


def look(args):
    os.makedirs(args.out, exist_ok=True)
    jpg = os.path.join(args.out, args.name + ".jpg")
    gray = os.path.join(args.out, args.name + ".gray")
    # 1.5 s of video, the last frame kept: the camera needs a moment to expose
    common = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-f", "avfoundation",
              "-framerate", "30", "-i", "%s:none" % args.camera, "-t", "1.5"]
    r = run(common + ["-update", "1", "-q:v", "3", jpg])
    if r.returncode != 0 or not os.path.exists(jpg):
        print("camera failed:", r.stderr.strip()[-400:], file=sys.stderr)
        return 2
    r = run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", jpg, "-vf",
             "scale=%d:%d,format=gray" % (SMALL_W, SMALL_H), "-f", "rawvideo", gray])
    if r.returncode != 0:
        print("grey copy failed:", r.stderr.strip()[-400:], file=sys.stderr)
        return 2
    print(jpg)
    return 0


def cells(path):
    data = open(path, "rb").read()
    cw, ch = SMALL_W // GRID_W, SMALL_H // GRID_H
    grid = []
    for gy in range(GRID_H):
        row = []
        for gx in range(GRID_W):
            total = 0
            for y in range(gy * ch, (gy + 1) * ch):
                base = y * SMALL_W + gx * cw
                total += sum(data[base:base + cw])
            row.append(total / (cw * ch))
        grid.append(row)
    return grid


def lights(args):
    before, after = cells(args.before), cells(args.after)
    changed = []
    lines = []
    for gy in range(GRID_H):
        line = ""
        for gx in range(GRID_W):
            d = after[gy][gx] - before[gy][gx]
            if d > args.threshold:
                changed.append(dict(x=gx, y=gy, change=round(d, 1), now="brighter"))
                line += "+"
            elif d < -args.threshold:
                changed.append(dict(x=gx, y=gy, change=round(d, 1), now="darker"))
                line += "-"
            else:
                line += "."
        lines.append(line)
    result = dict(grid="%dx%d" % (GRID_W, GRID_H), threshold=args.threshold,
                  brighter=sum(1 for c in changed if c["now"] == "brighter"),
                  darker=sum(1 for c in changed if c["now"] == "darker"),
                  cells=changed, map=lines)
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "changed.json"), "w") as f:
        json.dump(result, f, indent=1)
    print("\n".join(lines))
    print("brighter %d, darker %d" % (result["brighter"], result["darker"]))
    return 0


def session(args):
    os.makedirs(args.out, exist_ok=True)
    codes = []
    codes.append(look(argparse.Namespace(camera=args.camera, out=args.out, name="before")))
    codes.append(listen(argparse.Namespace(mic=args.mic, seconds=args.seconds, out=args.out,
                                           expect=args.expect, silence=args.silence)))
    codes.append(look(argparse.Namespace(camera=args.camera, out=args.out, name="after")))
    if codes[0] == 0 and codes[2] == 0:
        lights(argparse.Namespace(before=os.path.join(args.out, "before.gray"),
                                  after=os.path.join(args.out, "after.gray"),
                                  out=args.out, threshold=args.threshold))
    return max(codes)


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = p.add_subparsers(dest="cmd", required=True)
    sub.add_parser("devices")
    s = sub.add_parser("listen")
    s.add_argument("--mic", default="0")
    s.add_argument("--seconds", type=float, default=4)
    s.add_argument("--out", required=True)
    s.add_argument("--expect", type=float, default=0, help="the tone that must be heard (Hz)")
    s.add_argument("--silence", type=float, default=0,
                   help="pass only when the level stays below this (dBFS, e.g. -55)")
    s = sub.add_parser("look")
    s.add_argument("--camera", default="0")
    s.add_argument("--out", required=True)
    s.add_argument("--name", default="frame")
    s = sub.add_parser("lights")
    s.add_argument("--before", required=True)
    s.add_argument("--after", required=True)
    s.add_argument("--out", required=True)
    s.add_argument("--threshold", type=float, default=25)
    s = sub.add_parser("session")
    s.add_argument("--camera", default="0")
    s.add_argument("--mic", default="0")
    s.add_argument("--seconds", type=float, default=4)
    s.add_argument("--out", required=True)
    s.add_argument("--expect", type=float, default=0)
    s.add_argument("--silence", type=float, default=0)
    s.add_argument("--threshold", type=float, default=25)
    args = p.parse_args()
    return {"devices": lambda a: devices(), "listen": listen, "look": look,
            "lights": lights, "session": session}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
