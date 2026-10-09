#!/usr/bin/env python3
"""Writes mix-ultra-layout.json: the Hercules DJControl Mix Ultra as the Mac app Controller Mapper (Mantra)
draws it (~/Developer/MIX_ULTRA_MAPPER, layouts/hercules-djcontrol-mix-ultra.json), with the MIDI messages
read from the capture of 8.10.2026 (djmantra_app testing/results/mix-ultra-capture.json). Units: 1 = 40 px of
the mapper's camera photo; x, y are the CENTRE of a control, as in the mapper.

    python3 make_mix_ultra_layout_json.py <mapper layout.json> <out.json>
"""
import json, sys

src = json.load(open(sys.argv[1]))
DST = sys.argv[2]

TYPE = {"round": "button", "button": "button", "pad": "pad", "knob": "knob", "encoder": "encoder",
        "fader-v": "fader", "fader-h": "fader", "jog": "jog", "led": "led", "text": "label", "body": "body"}

# From the capture: deck 1 notes 0x91 / CC 0xB1, SHIFT layer 0x94 / 0xB4; deck 2 0x92 / 0xB2, SHIFT 0x95 / 0xB5;
# master 0x90 / 0xB0, SHIFT 0x93 / 0xB3; pads 0x96 (deck 1), 0x97 (deck 2).
def note(st, n): return {"type": "note", "status": st, "data1": n}
def cc(st, n, lsb=None, mode="absolute"):
    m = {"type": "cc", "status": st, "data1": n, "mode": mode}
    if lsb is not None: m["lsb_data1"] = lsb
    return m

REL = "relative: 1 = clockwise/right, 127 = counter-clockwise/left"
MIDI = {
    "browser": (cc(0xB0, 0x01, mode=REL), cc(0xB3, 0x01, mode=REL)),
    "browser press": (note(0x90, 0x00), note(0x93, 0x00)),
    "neural logo": (note(0x90, 0x01), note(0x93, 0x01)),
    "master": (cc(0xB0, 0x03, 0x23), None),
    "crossfader": (cc(0xB0, 0x00, 0x20), None),
    "button 1": (note(0x91, 0x0D), note(0x94, 0x0D)),
    "button 2": (note(0x92, 0x0D), note(0x95, 0x0D)),
    "phones 1": (note(0x91, 0x0C), note(0x94, 0x0C)),
    "phones 2": (note(0x92, 0x0C), note(0x95, 0x0C)),
}
PAD_MODES = [("hot cue", 0x00), ("loop", 0x10), ("fx", 0x20), ("neural mix", 0x30),
             ("pitch play", 0x40), ("bounce loop", 0x50), ("slicer", 0x60), ("sampler", 0x70)]
for d, n, s, pads in (("A", 0x91, 0x94, 0x96), ("B", 0x92, 0x95, 0x97)):
    c, cs = 0xB0 + (n - 0x90), 0xB0 + (s - 0x90)
    MIDI.update({
        f"{d} shift": (note(n, 0x04), None),
        f"{d} sync": (note(n, 0x05), note(s, 0x05)),
        f"{d} cue": (note(n, 0x06), note(s, 0x06)),
        f"{d} play": (note(n, 0x07), note(s, 0x07)),
        f"{d} hot cue": (note(n, 0x0F), note(n, 0x13)),     # SHIFT: pitch play, same channel
        f"{d} loop": (note(n, 0x10), note(n, 0x14)),        # SHIFT: bounce loop
        f"{d} fx": (note(n, 0x11), note(n, 0x15)),          # SHIFT: slicer
        f"{d} neural mix": (note(n, 0x12), note(n, 0x16)),  # SHIFT: sampler
        f"{d} volume": (cc(c, 0x00, 0x20), None),
        f"{d} filter": (cc(c, 0x01, 0x21), None),
        f"{d} low": (cc(c, 0x02, 0x22), None),
        f"{d} mid": (cc(c, 0x03, 0x23), None),
        f"{d} high": (cc(c, 0x04, 0x24), cc(cs, 0x04, 0x24)),  # SHIFT: gain
        f"{d} tempo": (cc(c, 0x08, 0x28), None),
    })
    MIDI[f"{d} jog"] = {
        "touch": note(n, 0x08),
        "top": cc(c, 0x0A, mode=REL),
        "ring": cc(c, 0x09, mode=REL),
        "shift": {"top": cc(cs, 0x0A, mode=REL), "ring": cc(cs, 0x09, mode=REL)},
    }
    for i in range(8):
        MIDI[f"{d} pad{i+1}"] = {
            "modes": {m: note(pads, base + i) for m, base in PAD_MODES},
            "shift": {m: note(pads, base + 8 + i) for m, base in PAD_MODES},
        }

controls = []
for c in src["controls"]:
    t = TYPE[c["shape"]]
    if t in ("body", "label"): continue
    i = c["id"]
    if i.startswith("A ") or i in ("button 1", "phones 1"): deck = 1
    elif i.startswith("B ") or i in ("button 2", "phones 2"): deck = 2
    else: deck = "master"
    o = {"id": i, "label": c["label"], "type": t, "deck": deck, "shape": c["shape"],
         "x": c["x"], "y": c["y"], "w": c["w"], "h": c["h"]}
    if c["shape"].startswith("fader"):
        o["orientation"] = "vertical" if c["shape"] == "fader-v" else "horizontal"
    m = MIDI.get(i)
    if isinstance(m, tuple):
        o["midi"] = m[0]
        if m[1]: o["midi_shift"] = m[1]
    elif isinstance(m, dict):
        o["midi"] = m
    if i == "browser": o["midi_press"], o["midi_press_shift"] = MIDI["browser press"]
    if "led" in c:
        o["led"] = {"type": "note", "status": c["led"][0], "data1": c["led"][1], "values": "0 = off, 127 = on"}
    controls.append(o)

body = next(c for c in src["controls"] if c["id"] == "body")
out = {
    "name": src["name"],
    "source": {"app": "Controller Mapper (Mantra) v4", "path": "~/Developer/MIX_ULTRA_MAPPER",
               "repo": "https://github.com/markoboskoauroville/mix-ultra-mapper",
               "layout": "layouts/hercules-djcontrol-mix-ultra.json (written by tools/make_mix_ultra_layout.py)",
               "midi": "testing/results/mix-ultra-capture.json (8.10.2026, read from the controller over USB)"},
    "units": "1 unit = 40 px of mix-ultra-photo.jpg (1280x720 = 32 x 18 units, placed at 0,0); "
             "x, y = centre of each control; w, h = its size (round controls: w = diameter)",
    "canvas": {"w": src["w"], "h": src["h"]},
    "outline": {"x": body["x"], "y": body["y"], "w": body["w"], "h": body["h"], "corner_radius": 0.5},
    "background_image": {"file": "mix-ultra-photo.jpg", "x": 0, "y": 0, "w": 32, "h": 18,
                         "note": "camera photo of 8.10.2026; the mapper draws shapes, not the photo"},
    "midi_port_contains": src["port"],
    "init": [{"bytes": b, "note": "B0 7F 7F asks the controller for the position of every knob and fader"}
             for b in src["init"]],
    "shift": [{"type": "note", "status": s, "data1": n} for s, n in src["shift"]],
    "notes": [
        "Buttons send note on 127 when pressed, 0 when released.",
        "Knobs and faders are 14-bit: MSB on data1, LSB on lsb_data1 (data1 + 0x20).",
        "With SHIFT held, deck controls move to the SHIFT channel (deck 1 0x94/0xB4, deck 2 0x95/0xB5, "
        "master 0x93/0xB3); the four pad-mode buttons stay on the deck channel with notes 0x13-0x16 "
        "(pitch play, bounce loop, slicer, sampler).",
        "Pads: deck 1 status 0x96, deck 2 0x97; note = mode base + pad index (0-7), +8 with SHIFT.",
        "Deck 1 jog with SHIFT (0xB4) is inferred from deck 2's capture (0xB5); deck 1's SHIFT capture missed the SHIFT.",
        "LEDs: the note on the button's own status, 127 on / 0 off; pad LEDs 0x96/0x97 notes 0x30+i (from the mapper).",
    ],
    "style": {"from": "MixUltraMapper.swift", "ground": "#0B0D10", "body": "#0E0F11", "cap": "#16171A",
              "groove": "#26282C", "jog_platter": "#121316", "label": "#F2DDB4 at 45 %", "text": "#F2DDB4",
              "done": "#33D17A", "todo": "#7A8087", "current": "#E8A33D", "disabled": "#33383E",
              "font": "monospaced"},
    "controls": controls,
}
json.dump(out, open(DST, "w"), indent=1, ensure_ascii=False)
print(DST, len(controls), "controls")
