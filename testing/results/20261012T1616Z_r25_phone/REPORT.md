# Round 25 on the phone (Nothing Phone (2a), A142, Android 16)

Build: run 38274065803, commit 763c684 (with b72fa77, 7c24ec5). Started by the tester's build watcher, 12.10.2026.
Phone woken (`KEYCODE_WAKEUP`, no lock), kept awake for the round. I locked portrait again; within a minute Marko freed rotation again (`user-rotation free`, auto-rotate 1) and the phone is sideways: so from now on the tester does NOT lock rotation and tests in the orientation Marko leaves (here landscape, ROTATION_270).

## Part 0: smoke - PASS
- No qml error, `Sound output open ... (clock)`, `frame rate 90.0 Hz -> 60.0 Hz` (then 120 -> 60), `interface ready 413x919`.
- A second map 4.5 s after the first; both say `crossfader 55 2124 974 105`, which matches the screen. PASS (round 24's offset is gone).

## Part 1: transport - PASS
- 3 Sense / Addis (NEXT). `play 1`, `auto cue: headphones to [Channel2]`, `start [Channel2] from the cued beat 2 ...`, `beat launch ... on beat 2`. 30 s: xruns 0, late 0, slowest 2.1-3.9 ms.

## Part 2: sliders and knobs - PASS
- Crossfader L and back, HIGH deck 1 down and double tap back. Mixer VU meters: three screenshots 0.3 s apart show different levels (`vu` frames): live.

## Part 3: AUTO PHASE and CUE - PASS (one note)
- Deck 1 on air, deck 2 paused on its cue (8.1; deck 1 at 2.3), deck 2's CUE HELD 3.5 s: `cue press [Channel2]: deck at 89.66 s, cue at 89.66 s, paused`, `cue_default pressed`; **no `auto phase: [Channel1]` line**; after the release deck 2 is back on 8.1, deck 1 played on (`p3_cued.png`, `p3_after_preview.png`). Round 24's fail fixed.
- Both playing, deck 2's volume DOWN (fader dragged to the bottom), deck 2 moved to beat 3 with AUTO PHASE off, then on: `auto phase: [Channel2] (beat 1) ...`, `jump [Channel2] to beat 3`. A jump on deck 1 (LED 6): deck 2 followed (`auto phase: [Channel2] (beat 8) ...`, `jump [Channel2] to beat 7`). Deck 1 never moved. PASS
- Both volumes up, crossfader middle: jump on deck 2 → deck 1 follows (`auto phase: [Channel1] ... jump [Channel1] to beat 3`); jump on deck 1 → deck 2 follows. The deck you did not touch moves: the old rule. PASS
  (Two earlier tries were void: deck 1's song had run out, so only one deck was on air.)
- CUE on a PLAYING deck, 3 times (PLAY again in between):
  - `cue press [Channel2]: deck at 253.44 s, cue at 88.16 s, playing` → stopped at -4:28 (the cue)
  - `deck at 98.22 s, cue at 90.16 s, playing` → stopped at -4:26 (the cue)
  - `deck at 99.30 s, cue at 89.66 s, playing` → stopped at -4:26 (the cue)
  Each time it stopped on the cue. PASS
- **Note:** the cue point itself moves between presses (88.16 → 90.16 → 89.66 s): every PLAY from the cue is an aligned start, which logs `cue [Channel2] at beat 5` / `cue set ... at phrase beat 5` (then 7) and so sets a NEW cue on the aligned beat. A DJ expects the cue to stay where it was set; please confirm with Marko whether the aligned start may move the cue.
