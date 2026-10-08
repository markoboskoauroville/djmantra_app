# Hercules DJControl Mix Ultra: research for a Mixxx mapping

Status: research only, researched 2026-10-08. No mapping files have been written yet.
Context: `docs/ANDROID_PORT.md` section 5. The goal is to write
`res/controllers/Hercules DJControl Mix Ultra.midi.xml` and a script for it, running on the
Android port of Mixxx 2.5.6 over BLE MIDI.

## 0. How to read this report

Every fact carries a tag:

| Tag | Meaning |
|---|---|
| **[P]** | A primary source: Hercules (manual, support FAQ, product page). A URL is given. |
| **[S]** | A secondary source: forum post, VirtualDJ manual, review or retailer. A URL is given. |
| **[I]** | Inferred from sibling Hercules mappings that ship with Mixxx (files in `res/controllers/`). |
| **[?]** | Unknown. It must be captured on the hardware. |

**Network limits during this research.** The sandbox proxy blocked direct access to these hosts:
support.hercules.com, ts.hercules.com (the manual PDF), www.hercules.com, mixxx.discourse.group,
community.algoriddim.com, community.hercules.com, virtualdj.com, manualslib.com, djmag.com and
others. Both `curl` and WebFetch failed on them with a CONNECT 403 or DNS errors. GitHub worked.
I cloned `mixxxdj/mixxx` (main) and `mixxxdj/manual` (main) directly.

So every [P] and [S] fact about the Mix Ultra comes from **search-engine snippets of those pages,
not from reading them in full**. The URLs are real, but someone should open them in a browser and
re-check each [P] and [S] line before relying on it. The most useful item to fetch by hand is the
community Mixxx 2.5.2 mapping (section 6, item 1).

## 1. What the device is

- A two-deck, battery-powered controller with no built-in audio interface. There is no headphone
  jack and no master output. Audio comes from the phone; a splitter cable is included for mono
  master plus cue. [P] manual / [S] reviews.
  Sources:
  - https://ts.hercules.com/download/sound/manuals/DJC_Mix_Ultra/DJControl_Mix_Ultra_user_manual_EN.pdf
  - https://support.hercules.com/en/kb/1806-en/
  - https://www.digitaldjtips.com/hercules-djcontrol-mix-ultra-award/
- It was made with Algoriddim for djay on iOS and Android, and it was announced at NAMM in
  January 2025. [S] https://djmag.com/tech/new-mobile-wireless-controller-dj-control-mix-ultra-launched-hercules
- Battery: 1000 mAh, up to 10 h. It charges in 1 h when switched off and 2 h when switched on,
  over USB-C (USB-A to USB-C cable included). There is a power switch next to the USB-C port. [P]
  product page: https://www.hercules.com/en/product/djcontrol-mix-ultra/
- Size about 31.5 x 17.3 cm, weight about 850 g. [P]/[S]
- Other supported software:
  - VirtualDJ 2025 has a native factory mapping (manual updated March 2025), with a Stems and FX
    pad page that exists only for this controller. [S] https://virtualdj.com/manuals/hardware/hercules/mixultra/controls.html
  - DJUCED is reported by one retailer only. [S, weak]
  - djay Pro on desktop works over USB. [S]
- Mixxx status:
  - There is **no mapping in Mixxx main** (checked `res/controllers` in a fresh clone on 2026-10-08).
  - There is **no Mixxx manual page** (checked `mixxxdj/manual` main).
  - There is a **community mapping for Mixxx 2.5.2** in the forum thread
    https://mixxx.discourse.group/t/hercules-djcontrol-mix-ultra/32257. Its attachments are
    `Hercules-DJControl-Mix-Ultra-script.js` and `Hercules_DJControl-Mix-Ultra.midi.xml`, and the
    author says changes are still coming. [S]
  - A help thread posts raw codes: https://mixxx.discourse.group/t/i-need-midi-mapping-for-the-hercules-mix-ultra/32253 [S]

## 2. Physical control inventory

| Section | Control | Per deck? | What it does in djay (and other apps) | Tag |
|---|---|---|---|---|
| Global | SHIFT | one (shared) | Modifier. Unlocks secondary functions and secondary pad modes. | [P] manual, [S] VirtualDJ |
| Global | Crossfader | 1 | Blends deck 1 and 2. A review mentions a djay crossfader-FX transition feature. | [P]/[S] |
| Global | MASTER volume knob | 1 | Main output level. | [P] manual |
| Global | Browser encoder (push) | 1 | Scrolls the track list; a push switches focus. | [S] VirtualDJ, Mixxx forum |
| Global | STEMS MODE (also called "Neural") button | 1 | VirtualDJ: switches the EQ knobs between frequency EQ and stems (EQ knobs become stem volumes). djay does the same per reviews. The Mixxx 2.5.2 community mapping used it for AutoDJ. | [S] |
| Global | Bluetooth status LED | 1 | Steady = connected, flashing = not connected. | [P] manual |
| Global | Power switch, USB-C | 1 | Power and charging. USB MIDI is reported to work but is not documented (section 4). | [P]/[S] |
| Deck | LOAD button (labelled 1 / 2) | 2 | Loads the selected track. VirtualDJ: a double press clones from the other deck. | [S] |
| Deck | GAIN | 2 | Pre-fader trim. Probably SHIFT + EQ HIGH rather than its own knob: the Mixxx forum mapping uses "Shift + high = gain", and VirtualDJ lists GAIN together with the EQs. | [S] / [?] |
| Deck | EQ HIGH / MID / LOW | 2 x 3 | 3-band EQ. In stems mode they act as stem levels. | [P]/[S] |
| Deck | FILTER knob | 2 | Low-pass/high-pass filter ("Color FX" in VirtualDJ); off at centre. | [S] |
| Deck | Channel volume fader | 2 | Deck level. | [P]/[S] |
| Deck | Headphone/monitor (PFL) button | 2 | Sends the deck to cue. | [P] manual, [S] VirtualDJ |
| Deck | TEMPO (pitch) fader | 2 | Tempo; the centre is the original speed. | [P] manual |
| Deck | Jog wheel (touch-sensitive) | 2 | Has two zones: top surface and ring. See the table below. | [P] manual |
| Deck | PLAY, CUE, SYNC | 2 x 3 | Transport. SHIFT + SYNC = sync leader, SHIFT + CUE = go to start, SHIFT + PLAY = stutter (Mixxx forum mapping). | [P]/[S] |
| Deck | Pad-mode buttons: HOT CUE, LOOP, FX, NEURAL MIX | 2 x 4 | Select the pad mode. SHIFT + button selects the secondary mode (see below). | [P] manual |
| Deck | Performance pads | 2 x 8 (16 in total) | 2 rows x 4 per deck. Sources disagree: the eShop says "4 pads", Thomann says "eight pads", the product page says 16. 8 per deck fits the manual's Pitch Play text ("pads 2 through 8") and the Neural Mix text (top row / bottom row). | [P] mostly / [?] |
| Deck | VINYL / scratch button | ? | Not listed in any Mix Ultra source. (The non-Ultra Mix has VINYL; VirtualDJ mentions "Vinyl mode", which may be software-only.) | [?] |
| Global | Headphone volume, split cue | ? | Probably absent, because there is no headphone output. (The non-Ultra Mix has a HEADPHONE knob, CC B0 04.) | [?] |
| Global | VU meters, beatmatch guide LEDs | ? | No source mentions them. Assume they are absent until checked. | [?] |

### Pad modes (in djay) [P]

Sources:
- Manual: https://ts.hercules.com/download/sound/manuals/DJC_Mix_Ultra/DJControl_Mix_Ultra_user_manual_EN.pdf
- FAQ: https://support.hercules.com/en/kb/1806-en/

| Button | Primary mode | SHIFT + button gives |
|---|---|---|
| HOT CUE | Hot Cue: set/trigger cues 1–8; SHIFT + pad deletes a cue | **Pitch Play**: plays a hot cue at different pitches; pads 2–8 each hold a pitch |
| LOOP | Loop (auto-loop) | **Bounce Loop**: a roll/slip loop; the playhead keeps moving in the background |
| FX | FX (pad FX) | **Slicer**: splits the active loop into 8 slices |
| NEURAL MIX | Neural Mix (stems): top row solos a stem, bottom row mutes it | **Sampler** |

Suggested Mixxx 2.5.6 equivalents:
- Hot Cue: `hotcue_N_activate` / `hotcue_N_clear`.
- Pitch Play: the Inpulse 300 "Toneplay" code (`DJCi300.toneplay`) does the same thing.
- Loop: `beatloop_X_toggle`.
- Bounce Loop: `beatlooproll_X_activate`.
- FX: effect unit slots.
- Slicer: the Inpulse 300 slicer code.
- Neural Mix: Mixxx 2.5.6 has no stem separation (stems in Mixxx 2.6 need pre-separated
  `.stem.mp4` files). Map these pads to something else: samplers, or the quick effect / filter.
- Sampler: `[SamplerN] cue_gotoandplay`.

### Jog wheel (in djay) [P] manual, reconstructed from snippets

| Transport | Zone | Action |
|---|---|---|
| Playing | Top (touched) | Stops while touched and resumes on release. The manual also mentions "fast seek"; check the full table. |
| Playing | Ring | Pitch bend (faster/slower). |
| Paused | Top | Fast seek / scratch. |
| Paused | Ring | Slow seek. |
| Any | SHIFT + wheel | Fastest seek. |

On the MIDI side, djay forum users report "two different MIDI signals" from each jog: a touch
note and a rotation CC. [S] https://community.algoriddim.com/t/is-there-no-alternative-to-manual-midi-map-of-hercules/19470

## 3. MIDI implementation

No official MIDI implementation document for the Mix Ultra (or the Mix) turned up. Hercules
publishes one for older models, for example
`ts.hercules.com/download/sound/manuals/DJC_Jogvision/DJCJogvision_MIDI_Commands.pdf`.
So the table below combines three sources:
- **(a)** codes a user captured from a real Mix Ultra and posted on the Mixxx and Hercules forums [S];
- **(b)** the **Hercules DJControl MIX** (non-Ultra) mapping in this repo [I], which a forum
  reply says "nothing changes in comparison" for the basics;
- **(c)** the **Inpulse 300/500** mappings [I], which share the newer Hercules note/CC layout
  (8 pads, mode buttons, 14-bit knobs).

### 3.1 The Hercules channel convention (consistent across MIX, Starlight, Inpulse 200/300/500) [I]

| Status (Note / CC) | MIDI channel | Used for |
|---|---|---|
| 0x90 / 0xB0 | 1 | Global: browser, crossfader, master, global buttons |
| 0x91 / 0xB1 | 2 | Deck A controls |
| 0x92 / 0xB2 | 3 | Deck B controls |
| 0x93 / 0xB3 | 4 | Global + SHIFT |
| 0x94 / 0xB4 | 5 | Deck A + SHIFT |
| 0x95 / 0xB5 | 6 | Deck B + SHIFT |
| 0x96 | 7 | Deck A pads; the pad note encodes mode and shift |
| 0x97 | 8 | Deck B pads |

The controller sends SHIFT itself on these separate channels. The script only needs to track
SHIFT for LED changes. Buttons send note-on with 0x7F on press and 0x00 on release (confirmed for
the Mix Ultra by (a): `91 10 7F` / `91 10 00`).

### 3.2 Input map

The status is shown for deck A; deck B uses 0x92/0xB2, and the SHIFT layer adds +3 to the
status channel (0x94/0xB4).

| Control | Message | Encoding | Tag / source |
|---|---|---|---|
| Browser encoder turn | `B0 01 vv` | Relative: 0x01 = clockwise, 0x7F = counter-clockwise (the capture shows `B0 01 7F`). SHIFT version probably `B3 01`. | **[S] captured** (a); [I] Inpulse 300 |
| Browser encoder push | `90 00 7F/00` | Button | [I] Inpulse 300/500 |
| STEMS/"Neural" button | `90 01 7F/00` | Button | **[S] captured** (a) |
| Crossfader | `B0 00 vv` | 7-bit; 14-bit LSB `B0 20`? | [I] MIX; LSB [?] |
| Master volume | `B0 03 vv` | 7-bit | [I] MIX |
| EQ LOW | `B1 02 msb` + `B1 22 lsb` | **14-bit** (MSB at CC n, LSB at CC n+0x20) | **[S] captured** (a): `B1 02 3F`, `B1 22 66` |
| EQ MID | `B1 03 msb` + `B1 23 lsb` | 14-bit | **[S] captured** (a): `B1 03 34`, `B1 23 4E` |
| EQ HIGH | `B1 04 msb` + `B1 24 lsb` | 14-bit | **[S] captured** (a): `B1 04 3F`, `B1 24 3C`. The poster labelled CC 0x24 "gain", but it fits the Inpulse 500 LSB pattern exactly. |
| GAIN | `B1 05` (+`B1 25`) or `B4 04` (shift layer) | 14-bit? | [I] Inpulse / [?] |
| FILTER | `B1 01 vv` | 7-bit, centre 0x40 | [I] MIX/Inpulse |
| Channel volume | `B1 00 vv` (+`B1 20` LSB?) | 7- or 14-bit | [I] MIX (7-bit), Inpulse 500 (14-bit) |
| Tempo fader | `B1 08 msb` + `B1 28 lsb` | **14-bit** | [I] MIX, Inpulse (all) |
| Jog touch (top) | `91 08 7F/00`; SHIFT `94 08` | Button | [I] MIX/Inpulse; "two signals" [S] |
| Jog rotate, touched (scratch) | `B1 0A vv`; SHIFT `B4 0A` | Relative 7-bit signed: 0x01..0x3F = clockwise, 0x41..0x7F = counter-clockwise. On the MIX it is always ±1 per tick. | [I] MIX script `_convertWheelRotation` |
| Jog rotate, ring (bend) | `B1 09 vv`; SHIFT `B4 09` | As above | [I] |
| SYNC | `91 05`; SHIFT `94 05` | Button | **[S] captured** `91 05 7F` (a) |
| CUE | `91 06`; SHIFT `94 06` | Button | [I] |
| PLAY | `91 07`; SHIFT `94 07` | Button | [I] |
| PFL/headphone | `91 0C` | Button | [I] |
| LOAD | `91 0D` (deck B `92 0D`) | Button | **[S] captured** `91 0D 7F`, `92 0D 7F` (a) |
| Mode HOT CUE | `91 0F` | Button | [I] Inpulse 300/500 (0x0F = hot cue) |
| Mode LOOP | `91 10` | Button | **[S] captured** (a) |
| Mode FX | `91 11` | Button | **[S] captured** (a) |
| Mode NEURAL MIX | `91 12` | Button | **[S] captured** (a). The (a) post also says "Neural Mix on channel 3"; check this. |
| SHIFT + mode (Pitch Play, Bounce, Slicer, Sampler) | `94 0F..12`, or `91 13..16`? | Button | [?] On the Inpulse 300, secondary modes have their own notes 0x13..0x16. |
| SHIFT | `90 03`? (MIX) / `91 04` (Inpulse 500, per deck) | Button | [I] / [?] |
| VINYL (if present) | `91 03` | Button | [I] MIX |

### 3.3 Pads: the note number encodes the mode [I]

Hercules firmware switches pad modes **inside the controller**. The same physical pad sends a
different note in each mode, and the SHIFT layer usually adds +0x08. There are two known layouts:

| Note base on 0x96/0x97 | DJControl MIX (4 pads) | Inpulse 300 (8 pads) | Inpulse 500 (8 pads) |
|---|---|---|---|
| 0x00 / +0x08 shift | Hot cue / clear | Hot cue / clear | Hot cue / clear |
| 0x10 | **Sampler** | Roll | Loop (0x18 shift) |
| 0x20 | **FX** (0x28 shift = select) | Slicer | Slicer |
| 0x30 | **Loop** | Sampler (0x38 shift = stop) | Sampler |
| 0x40 | – | Toneplay (16 notes) | Pitch/tone |
| 0x50 | – | FX | Roll |
| 0x60 | – | Slicer loop | FX |
| 0x70 | – | Beatjump | Beatjump |

For the Mix Ultra's 8 modes, the bases are **[?]**: they must be captured. A reasonable guess
treats it as an 8-pad Inpulse-style unit with these assignments (base / mode):

| Base | Mode |
|---|---|
| 0x00 | Hot Cue |
| 0x10 | Loop or Roll |
| 0x20 | Slicer |
| 0x30 | Sampler |
| 0x40 | Pitch Play |
| 0x50 or 0x60 | FX |
| one free base | Neural Mix |
| one free base | Bounce Loop |

Do not ship this guess.

### 3.4 LED output [I]

- Simple LEDs (play, cue, sync, PFL, mode buttons, load): send a note-on to the same note as the
  input; `0x7F` = on, `0x00` = off. On the MIX and Inpulse 300, the SHIFT-layer LEDs (for
  example the sync-leader LED) are addressed on 0x93/0x94.
- Hot-cue pad LEDs on the MIX/Inpulse 300 use velocity `0x7E` for on. On those single-colour
  units this probably selects a dimmer or alternate state; check it.
- **RGB pads:** if the Mix Ultra pads are RGB (reviews only say they "glow"; [?]), the Hercules
  RGB encoding from the Inpulse 500 script is a **7-bit 0bRRGGGBB** value:
  - 2 bits red: 0x60 = red;
  - 3 bits green: 0x1C = green;
  - 2 bits blue: 0x03 = blue;
  - examples: 0x7F white, 0x7C yellow, 0x1F cyan, 0x42 magenta, 0x74 orange, 0x12 dim cyan,
    0x40 dim red.

  See `DJCi500.PadColorMapper` in `res/controllers/Hercules-DJControl-Inpulse-500-script.js`
  (lines 85–105). It would plug straight into Mixxx's `ColorMapper` for `hotcue_N_color`.
- VU meters (if any): Hercules sends `B1 40 vv` / `B2 40 vv` per deck and `B0 40`/`B0 41` for
  master, with vv = level x 125 (Inpulse 300 script). Probably not present on the Ultra.

### 3.5 Init and shutdown messages [I]

- **`B0 7F 7F`** asks the controller to resend every knob and fader position. It is sent in
  `init()` by the MIX, Inpulse 200/300/500 and Starlight scripts. Use it after connecting over BLE.
- `B0 7F 00` turns all LEDs off; the MIX uses it in `shutdown()`.
- No SysEx handshake, LED dump or demo-mode message appears in any Hercules mapping in this repo.
  Hercules non-Ultra controllers need no SysEx. For the Ultra: [?]. Try the universal identity
  request `F0 7E 7F 06 01 F7` and record the reply; it is useful for firmware and version checks.

### 3.6 What the MIX (non-Ultra) mapping in this repo contains

Files: `res/controllers/Hercules DJControl MIX.midi.xml` and `Hercules-DJControl-MIX-scripts.js`.

| Status | Notes / CCs |
|---|---|
| 0x90 | 0x03 SHIFT |
| 0x91 / 0x92 | 0x03 VINYL (LED lit at init), 0x05 SYNC, 0x06 CUE, 0x07 PLAY, 0x08 jog touch, 0x0C PFL |
| 0x94 / 0x95 | Shifted: 0x05 sync_leader, 0x06 start, 0x07 play_stutter, 0x08 jog-touch-shift, 0x0C CUE MASTER / SPLIT |
| 0x96 / 0x97 | Pads: 0x00–03 hot cue, 0x08–0B clear, 0x10–13 sampler, 0x20–23 FX, 0x28–2A FX select, 0x30–33 loop |
| 0xB0 | 0x00 crossfader, 0x03 master, 0x04 headphone |
| 0xB1 / 0xB2 | 0x00 volume, 0x01 filter, 0x02 EQ low, 0x08/0x28 tempo (14-bit, soft-takeover), 0x09 jog ring, 0x0A jog top; SHIFT 0xB4/0xB5 0x0A |

The MIX script's scratch settings are a good starting point: `engine.scratchEnable(deck, 248,
33+1/3, 1/8, 1/256)`, ticks of ±1, and seeking with SHIFT at 4x. The Mixxx 2.4+ manual page
(source in `mixxxdj/manual`, `source/hardware/controllers/hercules_djcontrol_MIX.rst`) says the
MIX "is a class compliant USB MIDI" device and that the mapping needs USB, not Bluetooth.

## 4. Bluetooth LE MIDI and USB

- **Transport:** standard BLE MIDI 1.0. The phone app scans and connects.
  - Service `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`.
  - Characteristic `7772E5DB-3868-4112-A1A9-F2669D106BF3` (write-without-response and notify).
  - Every packet starts with a header and a 13-bit millisecond timestamp, and can carry several
    messages with running status.

  The phone side is already planned in `ANDROID_PORT.md` §2: `MidiManager.openBluetoothDevice()`,
  `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT`. [P] djay pairing steps in the manual; the BLE MIDI
  spec details are standard.
- **Pairing:** do not pair in Android Bluetooth settings. The app scans for the device and opens
  it. [P] manual: "Every time you open the djay app you must carry out the Bluetooth pairing
  procedure", and "if the display turns off or goes into locked mode, the Bluetooth pairing is
  lost". For our app this means:
  - keep the screen on, or run a foreground service, while a controller is connected;
  - reconnect automatically to the last device address;
  - send `B0 7F 7F` and refresh all LEDs after every reconnect.
- **Troubleshooting** [P] KB: if the app cannot see the controller, remove it from Android's
  Bluetooth device list. Android requirement: the manual says Android 10+, the support site says
  8.1+.
- **Latency:** Hercules states the BLE MIDI link adds **< 7 ms**, similar to USB [P] KB 1806, as
  quoted in a search snippet (open the page to confirm). Reviews report it as not noticeable,
  except slightly on fast cuts and scratches [S].

  On Android, the system BLE-MIDI service asks for a high-priority connection interval of about
  7.5–15 ms (from AOSP `BluetoothMidiDevice`, from memory; check it). Jog wheels produce bursts
  of CCs, so the script should add up ticks per callback rather than assume one message per
  event.
- **USB-C:**
  - Officially it is for charging. The FAQ says the controller "is not meant to be used for DJing
    in USB mode on a computer", **but** "on a computer it's easy to connect the controller via
    USB". [P] KB 1806.
  - VirtualDJ says "no driver required" (the HDJC driver is recommended for firmware updates)
    [S] https://virtualdj.com/manuals/hardware/hercules/mixultra/setup.html.
  - djay Pro on Windows/macOS works over USB [S].
  - A forum user says the USB-C port works as a USB MIDI interface [S].
  - Conclusion: it is very likely **USB MIDI class-compliant**, like the MIX. That means desktop
    Linux capture with `amidi`/`aseqdump` should work, and an Android USB-OTG connection should
    work through the same `MidiManager` path. [?] Check `lsusb -v` (Hercules USB vendor ID is
    0x06F8) and the interface class (Audio, subclass 3 = MIDIStreaming).

## 5. What must be captured on the real hardware

Use any of these:
- the app's MIDI logcat once M5 lands;
- an Android MIDI monitor app over BLE;
- on desktop Linux over USB: `amidi -l`, `aseqdump -p <port>` and `amidi -p hw:X -S '...'` for
  LED tests.

Checklist:
1. **Device identity:** the BLE advertised name and the Android `MidiDeviceInfo`
   (`PROPERTY_NAME` and `PRODUCT`, port names). Both are needed for auto-detection and the
   mapping's `<controller id>`. USB VID/PID and `lsusb -v` descriptors.
2. **Every button**, plain and with SHIFT: status and note, press/release values. Especially
   SHIFT itself (one shared note, or one per deck?), the STEMS/Neural button, all 4 mode buttons
   on both decks, SHIFT + mode buttons, LOAD, PFL, and whether VINYL exists.
3. **Pads:** for each of the 8 modes (4 plus 4 shifted) and each of the 8 pads, plain and with
   SHIFT: the note number. Does the controller switch mode internally (pad notes change) or does
   it only send the mode button and leave state to software? Does it remember the mode per deck?
4. **Knobs and faders:** CC numbers, whether each is 14-bit (watch for the n+0x20 LSB), which end
   is 0, and the centre value of the filter, EQs and tempo. Also: what does STEMS mode change
   (different CCs, or only the button LED)? Is GAIN a SHIFT layer (different CC or channel)?
5. **Jog wheels:**
   - the touch note;
   - the CCs for top and ring;
   - value range at slow and fast speed (only ±1, or bigger steps?);
   - ticks per revolution (needed for `scratchEnable` intervalsPerRev; MIX uses 248);
   - whether SHIFT changes the channel.
6. **LEDs:**
   - for every input note, send 0x7F/0x00 and record which LED lights;
   - test pads with 0x01, 0x7E, 0x7F, 0x60, 0x1C, 0x03, 0x12 to find out whether they are RGB
     and confirm the 0bRRGGGBB format;
   - check whether SHIFT-layer LEDs exist (0x93–0x95), and whether there is blink support or any
     VU / beat LEDs.
7. **Init behaviour:**
   - Does `B0 7F 7F` trigger a full value dump?
   - Does `B0 7F 00` blank the LEDs?
   - What does the identity request `F0 7E 7F 06 01 F7` return?
   - What does the controller send on its own when it connects (some Hercules units send all
     values at power-on)?
8. **BLE behaviour:**
   - round-trip latency (press to logcat timestamp);
   - drops when the screen is locked or the app is in the background;
   - auto-reconnect after a power cycle;
   - MIDI throughput while spinning both jogs and moving faders.
9. **Battery/USB:** does it work over USB-C OTG to the phone (powered from the phone)? Does it
   send MIDI over USB while charging from a PC?

## 6. Recommended next steps for writing the mapping

1. **Fetch the community mapping by hand** (the forum is blocked from this sandbox):
   https://mixxx.discourse.group/t/hercules-djcontrol-mix-ultra/32257, files
   `Hercules_DJControl-Mix-Ultra.midi.xml` and `Hercules-DJControl-Mix-Ultra-script.js`. It
   already contains real note/CC numbers for Mixxx 2.5.2, which is close to 2.5.6. Check its
   licence and author before reusing it (Mixxx mappings are normally GPL-2.0-or-later), and
   credit the author.
2. Start the new mapping from the **Inpulse 300** structure: 8 pads, 8 modes, mode-button LEDs,
   toneplay and slicer code. Take the deck basics, `B0 7F 7F` init and scratch constants from the
   **MIX** mapping. Use 14-bit `fourteen-bit-msb/lsb` for the EQs and tempo. Add the Inpulse 500
   `ColorMapper` if the pads are RGB.
3. Mixxx 2.5.6 has no stems. Use STEMS mode or the Neural Mix pads for samplers, quick effects or
   EQ kills (for example: top row = EQ band solo, bottom row = kill). Document this in the mapping
   description.
4. Confirm every [I] and [?] item against the capture checklist in §5 before shipping.

## 7. Sources

Primary (Hercules):
- Manual (EN): https://ts.hercules.com/download/sound/manuals/DJC_Mix_Ultra/DJControl_Mix_Ultra_user_manual_EN.pdf
- Manual (21 languages): https://ts.hercules.com/download/sound/manuals/DJC_Mix_Ultra/DJControl_Mix_Ultra_User_Manual_21_Languages.pdf
- Technical FAQ: https://support.hercules.com/en/kb/1806-en/
- Software compatibility FAQ: https://support.hercules.com/en/kb/1836-en/
- Support/downloads: https://support.hercules.com/en/product/djcontrolmixultra-en/
- Product page: https://www.hercules.com/en/product/djcontrol-mix-ultra/

Secondary:
- Mixxx forum mapping (2.5.2): https://mixxx.discourse.group/t/hercules-djcontrol-mix-ultra/32257
- Mixxx forum raw codes: https://mixxx.discourse.group/t/i-need-midi-mapping-for-the-hercules-mix-ultra/32253
- Hercules community (same codes): https://community.hercules.com/community/djcontrol-mix-ultra/mixxx-midi-mapping/
- VirtualDJ controls: https://virtualdj.com/manuals/hardware/hercules/mixultra/controls.html
- VirtualDJ setup: https://virtualdj.com/manuals/hardware/hercules/mixultra/setup.html
- Algoriddim forum: https://community.algoriddim.com/t/hercules-djcontrol-mix-ultra/33012
- Algoriddim forum (Android connection): https://community.algoriddim.com/t/hercules-djcontrol-mix-ultra-not-connecting-on-android/35363
- Reviews:
  - https://www.digitaldjtips.com/hercules-djcontrol-mix-ultra-award/
  - https://wearecrossfader.co.uk/blog/djcontrol-mix-ultra-review/
  - https://magneticmag.com/2025/03/hercules-djcontrol-mix-ultra-review/
  - https://djlifemag.com/2025/11/get-started-hercules-dj-control-mix-ultra/

Sibling mappings (in this repo and in Mixxx main):
- `res/controllers/Hercules DJControl MIX.midi.xml`, `Hercules-DJControl-MIX-scripts.js`
- `res/controllers/Hercules_DJControl_Inpulse_{200,300,500}.midi.xml` and their scripts
- `res/controllers/Hercules DJControl Starlight.midi.xml`, `Hercules_DJControl_Jogvision.midi.xml`
- Mixxx manual source: `mixxxdj/manual` → `source/hardware/controllers/hercules_djcontrol_MIX.rst`
