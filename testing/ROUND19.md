# Round 19 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 19 finished")

Build: the green Android run of the newest commit with this file. Fixes for round 18's fails
(testing/results/20261012T0927Z_r18_phone/REPORT.md). Grep once per part:
`adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Sections:|jump"`.

## 0. Smoke (10 s)
- Install, start: `Quick: interface ready`, not black, no `qml` error.

## 1. Transport
- PLAY / pause both decks on the press (log `Quick: button [ChannelN],play 1/0`), sound.
- SYNC on deck 2 (new log, 0.4 s after the press): `Quick: sync [Channel2]: deck 1 X bpm, deck 2 X bpm`: equal?
- CUE. Marko's own PLAY presses if he is there.
- DROPS (Marko's screen recording 11:52: ~4 drops a second, 10-60 ms each): both decks playing, SYNC on, 60 s on the
  Phrasing tab. Log at start: `Sound buffer: N of M frames` (N about 3072); every 5 s `Output level: peak ..., xruns N,
  slowest callback X ms, late K`: xruns must not grow, late 0, slowest well under the callback (21 ms at 1024 frames).
  Record 15 s of the phone's sound (observe.py listen, or a screen recording with sound) and count the silences.
  Repeat with KEY MATCH on (keylock: now SoundTouch on the phone, lighter than Rubber Band).

## 2. Sliders and knobs
- Crossfader L / M / R, volume faders, Mixer knobs and their LED rings, double tap (as round 18: quick check).

## 3. LEDs (round 18 FAIL: 32-beat LEDs 19 dp)
- Phrasing tab at 8, 16, 32 BEATS: each LED row at least ~46 px (logical; ~120 device px on the A142) at 32; the
  waveforms above shrink to make room (at 32 a thin strip). Screenshot at 32, portrait.
- Jumps are now QUANTIZED TO THE BEAT (Marko: "as Ableton with snap 1 beat ... seamless"): a press on a playing
  deck's LED marks it (yellow frame, log `Quick: jump [ChannelN] to beat M on the next beat`); on the next beat the
  deck plays beat M instead, in time, no gap, no stumble. Press the SAME LED several times in a row: it jumps every
  time (round 19 bug: the 2nd press of the same beat did nothing). Try 32 → 1, 32 → 8, 4 → 1 with both decks
  synced: does the music stay one continuous groove? Listen / record.
- Launch on a stopped deck as round 18.
- Matrix: time left counts down, 8 / 16 / 32 cells.

## 4. Screen (round 18 FAIL: the navigation bar never hid)
- Full screen after start and after home → back.
- Tap on empty space: the navigation bar comes (`WindowBridge: bars shown`), the app MAKES ROOM (nothing under the
  bar, log `Quick: insets ...` with the bar's size); a press on any button hides it (`WindowBridge: bars hidden`);
  a tap on empty space while shown hides it; unused it hides after ~4 s. Landscape: the bar covers nothing (+ / −,
  the Keys tab, the CPU meter stay clear). Swipe up from the bottom: bars come, next touch hides.
- Landscape Phrasing: the two decks' LEDs side by side, readable numbers; the scrolling waveform without diagonal
  lines (screenshot).
- Edges (Marko's photo, round 19: the app ran into the rounded corners, the meter's numbers cut): log `Quick: insets
  ... corner N` (the screen's corner radius); nothing touches the screen's edges or corners, the menu button and the
  meter sit inside the rounded corners; photo / screenshot of the top corners and the bottom tabs.
- The meter top right, half its old size: `C nn%` / `G nn%` (or `--`), both numbers fully visible; values on the
  Songs and the Phrasing tab.

## 5. Library
- A tap loads onto the free deck; cover tap then a song: that deck (quick check).

## 6. Specific features
- JUMP (round 18 FAIL: went to the song's end): with a section ahead it lands on the yellow line
  (`jump to the next section at X s; cues: ...`, the cues no longer list intro_start / outro_end); with none ahead it
  does nothing: `jump: no section change ahead`.
- Sections (round 18 FAIL: none in American Dream, Amulet): load both: within ~1 min a `Sections: [ChannelN] N changes`
  line for each; the overview shows the bands and lines.
- Genre (round 18: 1 of 5): the same 5 songs (Eran Hersh 3 Sense, Orkidz Addis, Jakatta American Dream, Amonita Amulet,
  Fulltone Alba): `GenreLookup: ...` lines; now the first artist only and the title without its mix name, the artist's
  genre when the song is unknown, one retry on 503. How many found?
- Phrasing waveforms (round 18 FAIL): at 8 BEATS the waves fill most of their half lane (3x louder); + / − zoom
  visibly (log `Quick: waveform zoom N`): + shows less time, − more.
- KEY MATCH on / off, − / +: each logs (`key match ...`, `Quick: key [ChannelN] reset_key / pitch_up / pitch_down`).
- AUTOMIX: with the new SYNC log, the BPM glide: deck 1's bpm moves to deck 2's as the fader goes right.

## 7. CPU and memory (round 18 FAIL: ~100 % of a core)
- `top` with both decks playing on the Songs, Phrasing and Mixer tabs, and the app's own meter. The matrix now paints
  only when its content changes (it painted many times a second). PSS.
