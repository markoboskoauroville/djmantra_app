#!/usr/bin/env python3
"""DJ Mantra UI test on a connected device (phone or emulator).

Starts the app with --ui-test (it logs every skin widget with its place on the
screen, and every change of the controls they show) and two test tones on the
decks. Then, in portrait and in landscape, on every page (mixer, waveforms,
pads, deck 1, deck 2, library, full-screen waveforms):

  - taps every button, drags every fader and turns every knob through adb,
    and checks in the log that its control changed;
  - double taps the waveforms (full screen and back);
  - saves a screenshot of the page.

  tools/android/ui_test.py --serial SERIAL --out testing/results/<stamp>_ui_<device>

Writes report.md (a table of every control: pass / fail), report.json and the
screenshots to --out. Exit status 1 if a control failed.
Needs adb and python3 (no other modules).
"""
import argparse
import json
import math
import os
import re
import struct
import subprocess
import sys
import tempfile
import time
import wave

PACKAGE = "com.djmantra.app"
ACTIVITY = PACKAGE + "/org.qtproject.qt.android.bindings.QtActivity"
TONES = ["/sdcard/Music/djmantra-test-tone.wav", "/sdcard/Music/djmantra-test-tone-2.wav"]

# Widgets that only show something
DISPLAY_TYPES = {"Number", "NumberPos", "Key", "Overview", "TrackProperty", "Label",
                 "VuMeter", "VuMeterGL", "VuMeterGLSL", "CoverArt", "StatusLight"}
# Buttons that switch the page: tested by the page walk, not one by one
PAGE_OBJECTS = {"TabButton", "LibraryButton", "SelectorButton", "Chevron", "BackButton",
                "GearButton"}
# Two-state buttons: tapped twice to leave the deck as it was
TOGGLES = ("play", "keylock", "sync_enabled", "pfl", "beatloop_activate")
# Controls that are skipped (they would leave the app or load the library)
SKIP_KEYS = {"[DJMantra],show_preferences", "[DJMantra],show_controller_map"}

MAP_BEGIN = re.compile(r"UI map begin (\d+) (\d+) (\d+)")
MAP_END = re.compile(r"UI map end (\d+)")
WIDGET = re.compile(r"UI widget (\S+) (\S+) (\S+) (-?\d+) (-?\d+) (\d+) (\d+) (\S+)")


class Device:
    def __init__(self, serial):
        self.serial = serial

    def adb(self, *args, check=False, text=True):
        cmd = ["adb", "-s", self.serial] + list(args)
        r = subprocess.run(cmd, capture_output=True, text=text)
        if check and r.returncode != 0:
            raise RuntimeError("%s: %s" % (" ".join(cmd), r.stderr))
        return r.stdout

    def shell(self, command):
        return self.adb("shell", command)

    def log(self):
        return self.adb("logcat", "-d", "-v", "brief", "DJMantra:I", "*:S")

    def clear_log(self):
        self.adb("logcat", "-c")

    def screenshot(self, path):
        data = subprocess.run(["adb", "-s", self.serial, "exec-out", "screencap", "-p"],
                              capture_output=True).stdout
        with open(path, "wb") as f:
            f.write(data)

    def rotate(self, landscape):
        self.shell("settings put system accelerometer_rotation 0")
        self.shell("settings put system user_rotation %d" % (1 if landscape else 0))


def make_tone(path, freq):
    rate, seconds = 44100, 240
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(rate)
        frames = bytearray()
        beat = rate // 2  # 120 BPM clicks over the tone: the analyser finds a beat grid
        for i in range(rate * seconds):
            click = 0.6 * math.exp(-(i % beat) / 300.0)
            v = 0.25 * math.sin(2 * math.pi * freq * i / rate) + click * math.sin(
                2 * math.pi * 60 * i / rate)
            s = int(max(-1.0, min(1.0, v)) * 32767)
            frames += struct.pack("<hh", s, s)
        w.writeframes(bytes(frames))


def parse_last_map(log):
    """The newest complete widget map: (number, width, height, [widgets])."""
    maps = []
    current = None
    for line in log.splitlines():
        m = MAP_BEGIN.search(line)
        if m:
            current = (int(m.group(1)), int(m.group(2)), int(m.group(3)), [])
            continue
        if current is None:
            continue
        m = WIDGET.search(line)
        if m:
            wtype, name, key, x, y, w, h, vis = m.groups()
            current[3].append(dict(type=wtype, name=name, key=key, x=int(x), y=int(y),
                                   w=int(w), h=int(h), visible=vis))
            continue
        m = MAP_END.search(line)
        if m and int(m.group(1)) == current[0]:
            maps.append(current)
            current = None
    return maps[-1] if maps else None


class UiTest:
    def __init__(self, device, out):
        self.dev = device
        self.out = out
        self.results = []
        self.tested = set()
        self.map = None

    # -- helpers -----------------------------------------------------------

    def wait_map(self, after=0, timeout=10.0):
        """Waits for a widget map newer than `after`."""
        end = time.time() + timeout
        while time.time() < end:
            m = parse_last_map(self.dev.log())
            if m and m[0] > after:
                self.map = m
                return m
            time.sleep(0.7)
        return self.map

    def control_changed(self, key, since_log):
        log = self.dev.log()
        new = log[len(since_log):] if log.startswith(since_log) else log
        pattern = "UI control %s = " % key
        values = [line.split(" = ")[-1].strip() for line in new.splitlines() if pattern in line]
        return values

    def record(self, orientation, page, widget, action, values, ok, note=""):
        self.results.append(dict(orientation=orientation, page=page, type=widget["type"],
                                 name=widget["name"], key=widget["key"], action=action,
                                 values=values[:6], result="pass" if ok else "FAIL", note=note))
        print("%-9s %-10s %-14s %-48s %-7s %s" % (orientation, page, widget["name"],
                                                    widget["key"], action,
                                                    "pass" if ok else "FAIL " + note))

    def center(self, w):
        return w["x"] + w["w"] // 2, w["y"] + w["h"] // 2

    # -- actions ---------------------------------------------------------------

    def tap(self, x, y):
        self.dev.shell("input tap %d %d" % (x, y))

    def double_tap(self, x, y):
        self.dev.shell("input tap %d %d; input tap %d %d" % (x, y, x, y))

    def swipe(self, x1, y1, x2, y2, ms=400):
        self.dev.shell("input swipe %d %d %d %d %d" % (x1, y1, x2, y2, ms))

    def test_widget(self, orientation, page, w):
        key = w["key"]
        item = key.split(",")[-1]
        if w["type"] in DISPLAY_TYPES or w["name"] in PAGE_OBJECTS or key in SKIP_KEYS:
            return
        if (orientation, key, w["name"]) in self.tested:
            return
        self.tested.add((orientation, key, w["name"]))
        x, y = self.center(w)
        before = self.dev.log()
        if w["type"] == "WaveformViewer":
            self.double_tap(x, y)
            time.sleep(1.5)
            values = self.control_changed("[DJMantra],waveform_fullscreen", before)
            ok = "1" in values
            shot = os.path.join(self.out, "%s-fullscreen-waveforms.png" % orientation)
            if ok and not os.path.exists(shot):
                self.wait_map(self.map[0])
                self.dev.screenshot(shot)
            self.record(orientation, page, w, "2x tap", values, ok)
            if ok:
                # back: double tap the waveform of the full-screen page
                fs = [v for v in self.map[3] if v["type"] == "WaveformViewer"]
                if fs:
                    fx, fy = self.center(fs[0])
                    self.double_tap(fx, fy)
                    time.sleep(1.5)
                self.wait_map(self.map[0])
            return
        if w["type"] in ("SliderComposed", "Slider"):
            vertical = w["h"] > w["w"]
            span = (w["h"] if vertical else w["w"]) // 4
            if vertical:
                self.swipe(x, y, x, y - span)
                time.sleep(1.0)
                values = self.control_changed(key, before)
                self.swipe(x, y - span, x, y)
            else:
                self.swipe(x, y, x + span, y)
                time.sleep(1.0)
                values = self.control_changed(key, before)
                self.swipe(x + span, y, x, y)
            self.record(orientation, page, w, "drag", values, bool(values))
            return
        if w["type"] in ("KnobComposed", "Knob", "EffectParameterKnobComposed"):
            self.swipe(x, y, x, y - max(60, w["h"]))
            time.sleep(1.0)
            values = self.control_changed(key, before)
            self.swipe(x, y - max(60, w["h"]), x, y)
            self.record(orientation, page, w, "turn", values, bool(values))
            return
        # buttons and pads
        self.tap(x, y)
        time.sleep(1.2)
        values = self.control_changed(key, before)
        if item in TOGGLES and values:
            self.tap(x, y)
            time.sleep(0.8)
        self.record(orientation, page, w, "tap", values, bool(values))

    def test_page(self, orientation, page):
        shot = os.path.join(self.out, "%s-%s.png" % (orientation, page))
        self.dev.screenshot(shot)
        for w in list(self.map[3]):
            self.test_widget(orientation, page, w)

    def switch(self, orientation, name, key, page, expect="1"):
        """Taps the page button `name` with control `key`; tests the new page."""
        cands = [w for w in self.map[3] if w["name"] == name and w["key"] == key]
        if not cands:
            return False
        w = cands[0]
        x, y = self.center(w)
        before = self.dev.log()
        number = self.map[0]
        self.tap(x, y)
        time.sleep(1.5)
        values = self.control_changed(key, before)
        ok = expect in values
        self.record(orientation, page, w, "page", values, ok)
        if ok:
            self.wait_map(number)
            self.test_page(orientation, page)
        return ok

    def run_orientation(self, orientation):
        self.dev.rotate(orientation == "landscape")
        number = self.map[0] if self.map else 0
        time.sleep(4)
        m = self.wait_map(number, timeout=15)
        if not m:
            print("no widget map in %s" % orientation)
            return
        prefix = "l" if orientation == "landscape" else "p"
        self.test_page(orientation, "mixer")
        for page in ("waveforms", "pads", "mixer"):
            self.switch(orientation, "TabButton", "[DJMantra],%s_%s" % (prefix, page), page)
        if orientation == "portrait":
            for page, key in (("deck1", "p_deck1"), ("deck2", "p_deck2"), ("mixer", "p_mixer")):
                self.switch(orientation, "SelectorButton", "[DJMantra],%s" % key, page)
        # the library, then back
        if self.switch(orientation, "LibraryButton", "[DJMantra],show_library", "library"):
            self.switch(orientation, "BackButton", "[DJMantra],show_library", "decks", expect="0")

    def run(self):
        os.makedirs(self.out, exist_ok=True)
        tmp = tempfile.mkdtemp()
        for path, freq in zip(TONES, (440, 330)):
            local = os.path.join(tmp, os.path.basename(path))
            make_tone(local, freq)
            self.dev.adb("push", local, path)
        self.dev.shell("input keyevent KEYCODE_WAKEUP")
        self.dev.shell("am force-stop %s" % PACKAGE)
        self.dev.clear_log()
        args = "--ui-test --play %s %s" % (TONES[0], TONES[1])
        self.dev.shell("am start -W -n %s --es applicationArguments \"'%s'\"" % (ACTIVITY, args))
        time.sleep(12)
        self.wait_map(0, timeout=40)
        for orientation in ("portrait", "landscape"):
            self.run_orientation(orientation)
        self.dev.shell("settings put system accelerometer_rotation 1")
        log = self.dev.log()
        with open(os.path.join(self.out, "log.txt"), "w") as f:
            f.write(log)
        sound = [l for l in log.splitlines() if "Output level" in l]
        self.write_report(sound, "Deck 1 playing" in log)

    def write_report(self, sound, playing):
        failed = [r for r in self.results if r["result"] != "pass"]
        with open(os.path.join(self.out, "report.json"), "w") as f:
            json.dump(dict(results=self.results, sound=sound[-3:], playing=playing), f, indent=1)
        lines = ["# UI test %s" % self.dev.serial, "",
                 "Controls tested: %d, failed: %d. Deck 1 playing: %s. Sound: %s" % (
                     len(self.results), len(failed), "yes" if playing else "no",
                     sound[-1].split("Output level:")[-1].strip() if sound else "no level logged"),
                 "", "| Orientation | Page | Widget | Control | Action | Values | Result |",
                 "|---|---|---|---|---|---|---|"]
        for r in self.results:
            lines.append("| %s | %s | %s | `%s` | %s | %s | %s |" % (
                r["orientation"], r["page"], r["name"], r["key"], r["action"],
                " ".join(r["values"]), "pass" if r["result"] == "pass" else "**FAIL**"))
        lines += ["", "Screenshots: one per orientation and page (`<orientation>-<page>.png`)."]
        with open(os.path.join(self.out, "report.md"), "w") as f:
            f.write("\n".join(lines) + "\n")
        print("\n%d controls, %d failed" % (len(self.results), len(failed)))
        return failed


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--serial", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    test = UiTest(Device(args.serial), args.out)
    test.run()
    failed = [r for r in test.results if r["result"] != "pass"]
    sys.exit(1 if failed or not test.results else 0)


if __name__ == "__main__":
    main()
