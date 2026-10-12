# Round 20 on the phone (Nothing Phone (2a), A142, Android 16, 1084x2412)

Build: run 38253413748, commit bd59c40. Started by the tester's own build watcher (Marko: "automatically detect when build is finished, and then you need to test"), 12.10.2026 13:24 UTC.

## Part 0: smoke - PASS
- Ready in 6 s: `Quick: interface ready 413x919`, no qml error, `Quick: insets 0 0 0 0 corner 50`.
- `Sound buffer: 3072 of 4096 frames` (asked: about 3072).
- `p0_start.png`: full screen; the matrix and overview edges are already yellow for both decks.

## Part 1: transport - PASS
- Loaded Let Me In (deck 1) and Lions (deck 2) (meant 3 Sense and Addis: the list had scrolled under the finger, see part 6).
- PLAY deck 1: `button [Channel1],play 1`. Deck 2 PLAY while deck 1 plays: `align [Channel2] to beat 8 ...`, `button [Channel2],play armed (aligned start)`, `beat launch [Channel2] on beat 8`. Pause: `play 0`. All as asked.
- SYNC logs the new value: deck 2's SYNC was on from the last session, the first press turned it off (`sync_enabled 0`), the second on (`sync_enabled 1`), each with `sync [Channel2]: deck 1 122.00 bpm, deck 2 122.00 bpm`.
- CUE: `button [Channel1],cue_default pressed`.
- AUTO PHASE: off, deck 2 moved to beat 3 → one beat behind (`p1_off.png`: deck 1 on a white 1, deck 2 on 2); on → `auto phase: [Channel2] (beat 4) to the other deck's beat`, both on 4 (`p1_on.png`). A first try (LED 5) logged no correction within 5 s: probably landed in phase already.
- Drops: 30 s, both playing, SYNC on, while the analysis in advance was running: xruns 0, late 0, slowest 2.4-3.4 ms.

## Part 2: sliders and knobs - PASS
- Mixer map lines are there: `knob_[Channel1],pregain`, the three EQ `parameter3/2/1`, `super1`, `fader_[ChannelN],volume`, `crossfader`.
- Knob direction: HIGH deck 1, slow `input swipe` from its centre 150 px DOWN (1.5 s): the pointer turned COUNTER-clockwise (value lower). Two drags up: back clockwise. Right now (`p2_knob_down.png`, `p2_knob_up.png`).
- Double tap resets the knob (`p2_knob_reset.png`) and the volume fader (dragged to ~half, `p2_moved.png` → `p2_reset.png`). Crossfader to L and back to the middle.
- Question for the builder: at rest (and after a double tap) HIGH / MID / LOW point to ~4-5 o'clock, while GAIN and FILTER point to 12. If the EQ's neutral is meant to be the middle, the drawing maps it wrong (Mixxx EQ 0..4, neutral 1).

## Part 3: LEDs - PASS
- 32 BEATS portrait (`p3_32beats.png`): the scrolling waveforms step aside, the LEDs take the page: each cell 129 device px high (`map led1_1 66 966 106 129`), ~49 dp. Asked: at least ~46 dp / ~120 px. Round 19's FAIL fixed.
- 8 and 16 BEATS: waveforms back above the LEDs (`p3_8beats.png`, `p3_16beats.png`).
- The LED map is logged again after each 8 / 16 / 32 change (`map bigLeds1 ... 560`, `led1_32`, `led2_32` at 32).
- Quantized jumps, AUTO PHASE off: LED 1 three times in a row → 3 x `jump [Channel2] to beat 1 on the next beat`; then 32, 8, 4, 1 → each logged. xruns 0, late 0, slowest 4.1-5.0 ms. Groove continuity: not by ear.
- AUTO PHASE back on moved deck 1 to deck 2's beat (`auto phase: [Channel1] (beat 10) ...`): the deck it moves is the one heard less; with the crossfader in the middle either is right.
