#!/usr/bin/env python3
"""Do deck 1's EQ (HIGH, MID, LOW) and FILTER sound as the engine predicts?

Runs on Marko's Mac with the phone over adb and its sound forwarded into
BlackHole (scrcpy --audio, observe.py devices: the BlackHole number). Starts
the app with pink noise on deck 1, playing, then turns each knob step by step
like a DJ (swipes on the screen), records 3 s of the app's output after every
step and measures the level of each octave band against the knob's centre.

The measured change per band is compared with the engine's own curves
(docs/eq-curves/eq_curves.csv and filter_curves.csv, made by the test
DJMantraEqCurveTest with DJMANTRA_CURVES_OUT): a step passes when every band
the curve keeps above -30 dB is within +-3 dB of the prediction.

  tools/android/eq_sweep_test.py --serial SERIAL --mic 1 --out testing/results/<stamp>_eq_sweep

Writes report.md, report.json and the recordings (step_*.wav) to --out.
Exit status 1 when a step does not match. Needs adb, ffmpeg, python3.
"""
import argparse
import csv
import json
import math
import os
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import compare_audio  # noqa: E402
import observe  # noqa: E402
import ui_test  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
CURVES = os.path.join(ROOT, "docs", "eq-curves")
NOISE = "/sdcard/Music/djmantra-test-pink.wav"
KNOBS = [  # (name, control, which curve)
    ("HIGH", "[EqualizerRack1_[Channel1]_Effect1],parameter3", "high"),
    ("MID", "[EqualizerRack1_[Channel1]_Effect1],parameter2", "mid"),
    ("LOW", "[EqualizerRack1_[Channel1]_Effect1],parameter1", "low"),
    ("FILTER", "[QuickEffectRack1_[Channel1]],super1", "filter"),
]
BANDS = [63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
TOLERANCE_DB = 3.0
FLOOR_DB = -30.0
STEP_PX = 30     # one swipe of a knob, in screen pixels
STEPS = 6        # swipes to each side of the centre
RECORD_S = 3.0


def make_pink(path):
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-f", "lavfi",
                    "-i", "anoisesrc=color=pink:amplitude=0.35:sample_rate=44100:duration=600",
                    "-ac", "2", path], check=True)


def load_curves():
    """{band: [(gain_value, {freq: dB})]} and [(knob, {freq: dB})] for the filter (Q 0.707)."""
    eq, flt = {}, []
    with open(os.path.join(CURVES, "eq_curves.csv")) as f:
        for row in csv.DictReader(f):
            freqs = {float(k): float(v) for k, v in row.items()
                     if k not in ("band", "knob", "gain_value")}
            eq.setdefault(row["band"], []).append((float(row["gain_value"]), freqs))
    with open(os.path.join(CURVES, "filter_curves.csv")) as f:
        for row in csv.DictReader(f):
            if abs(float(row["resonance_q"]) - 0.707) > 0.01:
                continue
            freqs = {float(k): float(v) for k, v in row.items()
                     if k not in ("knob", "resonance_q", "lpf_hz", "hpf_hz")}
            flt.append((float(row["knob"]), freqs))
    for v in eq.values():
        v.sort(key=lambda t: t[0])
    flt.sort(key=lambda t: t[0])
    return eq, flt


def predict(table, x, band_hz):
    """The curve's dB at band_hz for knob value x (linear between the rows; the
    octave band = the mean power of the 1/3 octave points inside it)."""
    def at(freqs):
        pts = [db for f, db in freqs.items() if band_hz / 1.41 <= f <= band_hz * 1.41]
        p = sum(10 ** (d / 10) for d in pts) / max(1, len(pts))
        return 10 * math.log10(p) if p > 0 else -200
    if x <= table[0][0]:
        return at(table[0][1])
    for (x0, f0), (x1, f1) in zip(table, table[1:]):
        if x0 <= x <= x1:
            t = 0 if x1 == x0 else (x - x0) / (x1 - x0)
            return at(f0) * (1 - t) + at(f1) * t
    return at(table[-1][1])


def bands_of(path):
    samples = compare_audio.decode(path)
    levels = compare_audio.band_levels(samples, blocks=12)
    return {b: levels.get(b, -200) for b in BANDS}


class Sweep:
    def __init__(self, dev, mic, out):
        self.dev = dev
        self.mic = mic
        self.out = out
        self.ui = ui_test.UiTest(dev, out)
        self.steps = []

    def value_of(self, key, timeout=3.0):
        values = self.ui.control_changed(key, timeout=timeout)
        return float(values[-1]) if values else None

    def capture(self, name):
        path = os.path.join(self.out, "step_%s.wav" % name)
        time.sleep(0.6)  # the knob's ramp
        if not observe.record(self.mic, RECORD_S, path):
            return None
        return bands_of(path)

    def widget(self, key):
        for w in self.ui.map[3]:
            if w["key"] == key and w["visible"] == "full":
                return w
        return None

    def run(self):
        os.makedirs(self.out, exist_ok=True)
        tmp = tempfile.mkdtemp()
        local = os.path.join(tmp, "pink.wav")
        make_pink(local)
        self.dev.adb("push", local, NOISE)
        self.dev.shell("input keyevent KEYCODE_WAKEUP")
        self.dev.rotate(False)
        self.dev.shell("am force-stop %s" % ui_test.PACKAGE)
        self.dev.adb("logcat", "-G", "16M")
        self.dev.clear_log()
        self.dev.adb("shell", "am", "start", "-W", "-n", ui_test.ACTIVITY, "--es",
                     "applicationArguments", "'--ui-test --play %s'" % NOISE)
        time.sleep(12)
        self.ui.wait_map(0, timeout=40)
        eq, flt = load_curves()
        reference = self.capture("reference")
        if reference is None:
            print("no recording: is the BlackHole number right (observe.py devices)?")
            return 2
        for name, key, curve in KNOBS:
            w = self.widget(key)
            if w is None:
                # the mixer page: tap its tab
                for t in self.ui.map[3]:
                    if t["key"].endswith("_mixer") and t["visible"] == "full":
                        self.ui.tap(*self.ui.center(t))
                        time.sleep(1.5)
                        self.ui.wait_map(self.ui.map[0], timeout=8)
                w = self.widget(key)
            if w is None:
                self.steps.append(dict(knob=name, step="-", result="FAIL",
                                       note="knob not on screen"))
                continue
            x, y = self.ui.center(w)
            for direction in (+1, -1):  # up (boost / high-pass), then down (cut / low-pass)
                pos = 0
                for _ in range(STEPS):
                    self.ui.swipe(x, y, x, y - direction * STEP_PX, 250)
                    pos += direction
                    value = self.value_of(key)
                    levels = self.capture("%s_%+d" % (name.lower(), pos))
                    self.judge(name, curve, pos, value, levels, reference, eq, flt)
                # back to the centre
                for _ in range(STEPS):
                    self.ui.swipe(x, y, x, y + direction * STEP_PX, 250)
                self.value_of(key, timeout=1.5)
        return self.report()

    def judge(self, name, curve, pos, value, levels, reference, eq, flt):
        if value is None or levels is None:
            self.steps.append(dict(knob=name, step=pos, value=value, result="FAIL",
                                   note="no value logged" if value is None else "no recording"))
            return
        table = flt if curve == "filter" else eq[curve]
        rows, worst = [], 0.0
        for b in BANDS:
            measured = levels[b] - reference[b]
            predicted = predict(table, value, b)
            judged = predicted > FLOOR_DB and reference[b] > -90
            diff = measured - predicted
            if judged:
                worst = max(worst, abs(diff))
            rows.append(dict(band=b, measured=round(measured, 1),
                             predicted=round(predicted, 1), judged=judged))
        ok = worst <= TOLERANCE_DB
        self.steps.append(dict(knob=name, step=pos, value=value, worst=round(worst, 1),
                               bands=rows, result="pass" if ok else "FAIL"))
        print("%-6s step %+d value %.3f worst %.1f dB %s" % (name, pos, value, worst,
                                                            "pass" if ok else "FAIL"))

    def report(self):
        failed = [s for s in self.steps if s["result"] != "pass"]
        with open(os.path.join(self.out, "report.json"), "w") as f:
            json.dump(self.steps, f, indent=1)
        lines = ["# EQ and filter sweep, deck 1, %s" % self.dev.serial, "",
                 "Pink noise; each knob turned in %d swipes up and down from the centre; "
                 "3 s recorded after each step. Measured = band level against the knob's "
                 "centre; predicted = the engine's curve (docs/eq-curves). Pass: every band "
                 "above %d dB within ±%.0f dB." % (STEPS, FLOOR_DB, TOLERANCE_DB), "",
                 "Steps: %d, failed: %d." % (len(self.steps), len(failed)), "",
                 "| Knob | Step | Value | " + " | ".join("%g" % b for b in BANDS) +
                 " | Worst | Result |",
                 "|---|---|---|" + "---|" * len(BANDS) + "---|---|"]
        for s in self.steps:
            if "bands" not in s:
                lines.append("| %s | %s | %s |%s | | **FAIL** %s |" % (
                    s["knob"], s["step"], s.get("value"), " |" * len(BANDS), s.get("note", "")))
                continue
            cells = ["%+.1f (%+.1f)" % (r["measured"], r["predicted"]) if r["judged"]
                     else "%+.1f" % r["measured"] for r in s["bands"]]
            lines.append("| %s | %+d | %.3f | %s | %.1f | %s |" % (
                s["knob"], s["step"], s["value"], " | ".join(cells), s["worst"],
                "pass" if s["result"] == "pass" else "**FAIL**"))
        lines += ["", "Cells: measured dB (predicted dB); bands below the floor show only "
                  "the measurement."]
        with open(os.path.join(self.out, "report.md"), "w") as f:
            f.write("\n".join(lines) + "\n")
        print("\n%d steps, %d failed" % (len(self.steps), len(failed)))
        return 1 if failed else 0


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--serial", required=True)
    p.add_argument("--mic", default="1", help="BlackHole's number (observe.py devices)")
    p.add_argument("--out", required=True)
    args = p.parse_args()
    return Sweep(ui_test.Device(args.serial), args.mic, args.out).run()


if __name__ == "__main__":
    sys.exit(main())
