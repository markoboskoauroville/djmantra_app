---
description: Capture every control, LED and pad mode of the Hercules DJControl Mix Ultra and push the results
---

You are running on the user's own computer with a Hercules DJControl Mix Ultra connected
by **USB-C** (preferred: no pairing, no drops) or paired over Bluetooth MIDI.
Goal: record exactly what every control sends and which message lights every LED, so the
cloud session can write a complete Mixxx mapping. Background: `docs/controllers/hercules-djcontrol-mix-ultra-research.md`
(read its control list and "open questions" first).

Tool: `tools/controller/midi_capture.py` (non-interactive; each call appends to
`testing/results/mix-ultra-capture.json`). You talk to the user; the tool listens.

## 1. Prepare
- `git fetch origin claude/admiring-feynman-hym3vp && git checkout claude/admiring-feynman-hym3vp && git pull`
- `pip install mido python-rtmidi` (in a venv if the system Python refuses).
- Connect the controller, then `python3 tools/controller/midi_capture.py ports`. Note the exact
  port names (they are part of the result). If the name does not contain "hercules", pass
  `--port <substring>` on every call.
- Linux only, extra facts: `lsusb -v -d 06f8:` (Hercules vendor id) → save VID/PID and the USB
  descriptors to the report; `aconnect -l`.
- Community mapping (Mixxx 2.5.2) from https://mixxx.discourse.group/t/hercules-djcontrol-mix-ultra/32257 :
  open the thread, download `Hercules_DJControl-Mix-Ultra.midi.xml` and `…-script.js` into
  `testing/results/mix-ultra-community/` together with a `SOURCE.md` (URL, author, date, the
  licence if stated). They are a cross-check only; the cloud session decides what to reuse.
- Close djay, Mixxx or any app that may hold the port.

## 2. Startup and handshake
1. `listen --label "power-on / connect" --seconds 10` while the user unplugs and replugs
   (or switches on) the controller. Records whatever it sends on connect.
2. `send B0 7F 7F --note "<what happened>"` then right away `listen --label "after B0 7F 7F" --seconds 4`:
   does it dump the positions of all knobs and faders?
3. `send F0 7E 7F 06 01 F7` then `listen --label "identity reply" --seconds 3`.
4. `send B0 7F 00` and ask: did all LEDs go off?

## 3. Every control (ask one at a time, run `listen` while the user does it)
Tell the user exactly what to do and to wait for "listening" before starting. For each item
use a clear label like `"A PLAY"`, `"A PLAY + SHIFT"`. Do every button **plain and with SHIFT
held** (also note whether SHIFT itself sends a message and whether it is one SHIFT or one per deck).

- Global: SHIFT; browser encoder (turn slowly right, then left, then press); STEMS/"Neural"
  button; master/main knob min→max; crossfader full left→right; any other top-panel button.
- For deck A, then deck B: LOAD; GAIN (if any, else SHIFT+HIGH); EQ HIGH/MID/LOW each min→max
  (and with STEMS mode on); FILTER left→centre→right (say when at centre); volume fader
  down→up; PFL/headphone; tempo fader top→bottom; PLAY; CUE; SYNC; VINYL (if present);
  every mode button (HOT CUE, LOOP, FX, NEURAL MIX/STEMS, …).
- Jog wheel per deck: touch the top and hold 2 s and release; spin the **top** exactly one
  full turn clockwise (use `--seconds 8`), then counter-clockwise; spin the **outer ring**
  one turn each way; one turn clockwise with SHIFT held. The `count` of messages gives ticks
  per revolution.
- Pads: for **each mode** (each mode button, and each mode with SHIFT: Pitch Play, Bounce
  Loop, Slicer, Sampler …) press pads 1–8 in order (top row left→right, then bottom row) in
  one `listen --seconds 12`. Also try SHIFT + pad in each mode. Ask whether the controller
  itself changes the mode LEDs when a mode button is pressed (before any software answers).

## 4. LEDs
For each button/pad note found above, light it with `send <status> <note> 7F --note "<what lit>"`
and ask the user what lit up (which LED, colour). Then `send <status> <note> 00`.
- If unsure where LEDs are, use `sweep 91 00 7F --delay 0.6` (and 92, 90, 94, 95, 96, 97 …):
  the user calls out the number printed when something lights; record with `send … --note`.
- Pads: test velocities `01 10 1C 03 60 7E 7F` on pad 1 in each mode and note colour and
  brightness: are pads RGB? single colour? dim/bright levels?
- Ask if there are VU meters, beat/BPM LEDs, battery or Bluetooth LEDs, jog-ring LEDs; find
  their messages with sweeps on note **and** CC (`sweep B1 00 7F`, values 7F and 40).

## 5. Bluetooth (if possible)
Pair it over BLE (Linux: `bluetoothctl`, then the ALSA BLE-MIDI port; macOS: Audio MIDI Setup
→ Bluetooth; or on the phone later). Repeat `ports` (record the BLE device name), one jog spin
of both wheels at once (`--seconds 6`), and note whether messages ever arrive late or drop.

## 6. Write and push the results
- `python3 tools/controller/midi_capture.py summary > testing/results/<YYYY-MM-DD_HHMM>_mix-ultra.md`
  and add at the top: OS, connection (USB/BLE), port names, USB VID/PID, firmware if shown,
  plus the user's answers to every question above (a table: question | answer).
- Commit `testing/results/mix-ultra-capture.json` and the report on
  `claude/admiring-feynman-hym3vp` and push. Never commit anything from `~/Downloads/API/`.
- Tell the user: "Results pushed. Tell the cloud session: read the latest Mix Ultra capture."
