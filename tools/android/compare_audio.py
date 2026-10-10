#!/usr/bin/env python3
"""Does DJ Mantra play a song as it is? Compares the original file with what the
app put out (recorded through BlackHole, observe.py listen) band by band.

  compare_audio.py --original SONG --recorded listen.wav --out DIR [--start S]

Both are decoded with ffmpeg to 48 kHz mono. The recording is lined up with the
original (cross-correlation of the loudness envelopes, so the app's start delay
does not matter), then over the common part:
  - the level in each octave band from 31.5 Hz to 16 kHz, original and app,
    after matching the overall level (a volume or gain change is not an error)
  - the difference per band: within +-3 dB from 63 Hz to 12.5 kHz = no
    filtering or colouring
  - the playback speed (lining up at the start and at the end: 1.000 = right)
  - clipping (samples at full scale) and the correlation of the two signals
Writes compare.json and compare.md; exit code 0 = the same sound, 1 = not.
--start: where in the original the recording begins, if known (seconds).
Plain Python 3 and ffmpeg.
"""

import argparse
import cmath
import json
import math
import os
import struct
import subprocess
import sys

RATE = 48000
BANDS = [31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
CHECKED = (63, 12500)  # bands judged: the speaker-free digital path should be flat here
TOLERANCE_DB = 3.0
FFT_N = 8192


def decode(path, seconds=None, start=0.0):
    cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-ss", str(start), "-i", path]
    if seconds:
        cmd += ["-t", str(seconds)]
    cmd += ["-vn", "-ac", "1", "-ar", str(RATE), "-f", "s16le", "-"]
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout
    return [v / 32768.0 for v in struct.unpack("<%dh" % (len(raw) // 2), raw)]


def envelope(samples, hop=480):
    return [math.sqrt(sum(x * x for x in samples[i:i + hop]) / hop)
            for i in range(0, len(samples) - hop, hop)]


def best_offset(ref_env, rec_env, max_shift, centre=0):
    """Shift of rec against ref (in envelope steps, within centre +- max_shift)
    with the best correlation."""
    def corr(shift):
        pairs = [(ref_env[i], rec_env[i + shift]) for i in range(len(ref_env))
                 if 0 <= i + shift < len(rec_env)]
        if len(pairs) < 20:
            return -1
        ma = sum(a for a, _ in pairs) / len(pairs)
        mb = sum(b for _, b in pairs) / len(pairs)
        num = sum((a - ma) * (b - mb) for a, b in pairs)
        den = math.sqrt(sum((a - ma) ** 2 for a, _ in pairs) * sum((b - mb) ** 2 for _, b in pairs))
        return num / den if den else -1
    scores = [(corr(s), s) for s in range(centre - max_shift, centre + max_shift + 1)]
    return max(scores)


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


def band_levels(samples, blocks=24):
    """Average power per octave band (dB) over up to `blocks` FFT blocks."""
    window = [0.5 - 0.5 * math.cos(2 * math.pi * i / (FFT_N - 1)) for i in range(FFT_N)]
    power = [0.0] * (FFT_N // 2)
    step = max(FFT_N, (len(samples) - FFT_N) // blocks)
    count = 0
    for start in range(0, len(samples) - FFT_N, step):
        spectrum = fft([s * w for s, w in zip(samples[start:start + FFT_N], window)])
        for k in range(FFT_N // 2):
            power[k] += abs(spectrum[k]) ** 2
        count += 1
    levels = {}
    for centre in BANDS:
        lo, hi = centre / math.sqrt(2), centre * math.sqrt(2)
        ks = range(max(1, int(lo * FFT_N / RATE)), min(FFT_N // 2, int(hi * FFT_N / RATE) + 1))
        total = sum(power[k] for k in ks) / max(1, count)
        levels[centre] = 10 * math.log10(total) if total > 0 else -200.0
    return levels


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--original", required=True)
    p.add_argument("--recorded", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--start", type=float, default=0.0)
    args = p.parse_args()

    rec = decode(args.recorded)
    seconds = len(rec) / RATE
    # the original from a little before the recording's start to after its end
    lead = min(args.start, 5.0)
    ref = decode(args.original, seconds + 10, args.start - lead)

    ref_env, rec_env = envelope(ref), envelope(rec)
    score, shift = best_offset(ref_env, rec_env, int((lead + 5) * 100))
    # the same at the end of the recording: a different shift = a different speed
    # rec_env[i + shift] matches ref_env[i]; half way through the recording
    # the shift is the same at the right speed, smaller when it plays fast
    tail = len(rec_env) // 2
    speed = 1.0
    if tail > 200:
        score2, shift2 = best_offset(ref_env[tail:], rec_env[tail:], 80, shift)
        speed = tail / max(1, tail + shift2 - shift)
    offset = -shift * 480  # samples of ref before the recording's first sample

    # the common part
    start_ref = max(0, offset)
    start_rec = max(0, -offset)
    n = min(len(ref) - start_ref, len(rec) - start_rec)
    a = ref[start_ref:start_ref + n]
    b = rec[start_rec:start_rec + n]
    if n < FFT_N * 2:
        print("too little common audio (%.1f s)" % (n / RATE), file=sys.stderr)
        return 2

    la, lb = band_levels(a), band_levels(b)
    # match the overall level (volume, gain, replay gain are not distortion)
    judged = [c for c in BANDS if CHECKED[0] <= c <= CHECKED[1] and la[c] > -150]
    shift_db = sum(lb[c] - la[c] for c in judged) / max(1, len(judged))
    diff = {c: round(lb[c] - la[c] - shift_db, 1) for c in BANDS}
    worst = max((abs(diff[c]) for c in judged), default=0.0)
    clipped = sum(1 for x in b if abs(x) >= 0.999)
    ma, mb = sum(a) / n, sum(b) / n
    num = sum((x - ma) * (y - mb) for x, y in zip(a, b))
    den = math.sqrt(sum((x - ma) ** 2 for x in a) * sum((y - mb) ** 2 for y in b))
    correlation = num / den if den else 0.0

    same = worst <= TOLERANCE_DB and clipped == 0 and abs(speed - 1.0) < 0.003
    result = dict(
        original=os.path.basename(args.original), recorded=os.path.basename(args.recorded),
        common_seconds=round(n / RATE, 1), alignment_score=round(score, 3),
        level_change_db=round(shift_db, 1), speed=round(speed, 4),
        band_original_db={str(c): round(la[c], 1) for c in BANDS},
        band_app_db={str(c): round(lb[c], 1) for c in BANDS},
        band_difference_db={str(c): diff[c] for c in BANDS},
        worst_difference_db=round(worst, 1), clipped_samples=clipped,
        waveform_correlation=round(correlation, 3), same_sound=same)
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "compare.json"), "w") as f:
        json.dump(result, f, indent=1)
    lines = ["# %s: app vs original" % result["original"], "",
             "Common audio %.1f s, lined up with score %.2f. Level change %+.1f dB, speed %.4f, "
             "clipped samples %d, waveform correlation %.3f." % (
                 n / RATE, score, shift_db, speed, clipped, correlation), "",
             "| Band (Hz) | Original dB | App dB | Difference dB |", "|---|---|---|---|"]
    for c in BANDS:
        mark = "" if not (CHECKED[0] <= c <= CHECKED[1]) else (" **!**" if abs(diff[c]) > TOLERANCE_DB else "")
        lines.append("| %g | %.1f | %.1f | %+.1f%s |" % (c, la[c], lb[c], diff[c], mark))
    lines += ["", "**%s** (every band from 63 Hz to 12.5 kHz within ±%.0f dB, no clipping, "
              "speed 1.000 ± 0.3%%)." % ("The same sound" if same else "NOT the same sound",
                                         TOLERANCE_DB)]
    with open(os.path.join(args.out, "compare.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines))
    return 0 if same else 1


if __name__ == "__main__":
    sys.exit(main())
