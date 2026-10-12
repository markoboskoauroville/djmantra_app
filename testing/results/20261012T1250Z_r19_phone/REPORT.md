# Round 19 on the phone (Nothing Phone (2a), A142, Android 16, 1084x2412)

Build: run 38233904582, commit 1a724b3. Started on Marko's word ("please start testing"), 12.10.2026 12:50 UTC.

## Part 0: smoke - PASS
- Installed, started in 6 s: `Quick: interface ready 413x919`, not black, no qml error in the start log.
- `Quick: insets 0 0 0 0 corner 50`. `Sound buffer: 2048 of 2048 frames` (ROUND19 expected about 3072: noted for part 1 DROPS).
- Screenshot `p0_start.png`: full screen, no navigation bar; the meter top right shows `C` / `G` with `1%` and `--`.

## Part 1: transport - PASS (by log and screen; not by ear)
- Loaded 3 Sense (deck 1) and Addis (deck 2) from the library. PLAY on deck 1: `button [Channel1],play 1`, sound (peak 0.04 → 0.6).
- Aligned start, 5 presses of deck 2's PLAY at random moments: each `align [Channel2] to beat N with the other deck` then `beat launch [Channel2] on beat N` (N = 7, 5, 1, 7, 6). Screenshot `p1_aligned.png`: both decks' lit matrix cells in the same column.
  Note: the start press logs `button [Channel2],play 0` (not `play 1`), the stop press also `play 0`. Cosmetic in the log; the deck does start.
- AUTO PHASE: off → `auto phase off`, LED 5 on deck 2 moved it out of phase and nothing moved back. On → `auto phase: [Channel2] (beat 5) to the other deck's beat`, `align ... beat 4`, within ~2 s. Marko-style jump on deck 1 (LED 3): deck 2 followed (`auto phase: [Channel2] (beat 2) ...`, `align ... beat 4`). `p1_follow.png`: both rows lit on 4.
- ALIGN ▸| tap (AUTO PHASE off, deck 2 off by 2 beats): `align [Channel2] to beat 8 ...` + `jump ... on the next beat`. Held 0.7 s: `next section [Channel2]`.
- SYNC deck 2: `sync [Channel2]: deck 1 122.00 bpm, deck 2 122.00 bpm` - equal. (The press logged `sync_enabled 0` though the button turned blue/on.)
- CUE: a tap on CUE (either deck) logs no `button ... cue` line at all; after CUE + PLAY the deck plays (aligned start). Check whether the tap reaches the engine.
- DROPS: `Sound buffer: 2048 of 2048 frames` (ROUND19 said ~3072). 60 s on Phrasing, both playing, SYNC on: xruns stayed 1 (the one from start), late 0, slowest callback 2.4-3.4 ms. KEY MATCH on deck 2 (`key match [Channel2] 8A > 10A: 2 semitones`), 30 s: xruns 1, late 0, slowest 4.0-4.3 ms.
  Not done: a sound recording to count silences (no audio capture over adb on Android 16); Marko's ear needed.
- The key badge (top) opens the Keys tab.

## Part 2: sliders and knobs - PASS (quick check)
- Crossfader L / R / M: the thumb follows the finger to both ends and back (`p2_xf_left.png`, `p2_xf_right.png`). No log line for it, so the sound side is not checked (not by ear).
- Volume fader deck 1 dragged to half (`p2_moved.png`), double tap: back to full (`p2_reset.png`).
- HIGH knob deck 1: a drag DOWN turned it clockwise (up). Check the direction: on DJ apps a drag down turns a knob down. Double tap: back to centre; its LED ring follows.
- The meters between the faders do not change with the fader or the crossfader (pre-fader). Fine if intended.
- The Mixer tab emits no `Quick: map` lines (tapped by screen coordinates).
