# Round 22 on the phone (Nothing Phone (2a), A142, Android 16, 1084x2412)

Build: run 38260844240, commit 94fc08d (the builder's head after 98e3fae, 5a23541, 2df39cf). Started by the tester's build watcher, 12.10.2026.
Round 21 was skipped at the builder's request (its fixes are in this build).

## Part 0: smoke - PASS
- Ready in 6 s, no qml error. `AudioRouter: 9 output(s) added`; `Sound output open: "Phone audio output (speaker, USB or Bluetooth)" (clock) 48000 Hz, 2 channels, 1024 frames per callback, burst 256, API AAudio, device 3`; `Sound buffer: 3072 of 4096 frames`.
- It started in landscape (`interface ready 919x413`): the phone was lying sideways with auto-rotate on. Locked to portrait for the test with `cmd window user-rotation lock 0` (`settings put system user_rotation 0` alone did not turn it); auto-rotate is given back at the end.

## Part 1: transport - PASS (in landscape)
- Marko turned auto-rotate back on with the phone sideways, so the rest of the round runs in LANDSCAPE as he holds it (my first taps used a stale map and loaded a ganesha song; redone after a restart).
- Loaded 3 Sense (tap) and Addis (deck 2's NEXT). PLAY deck 1: `button [Channel1],play 1`, `played in this set: 3 Sense`. PLAY deck 2: `align [Channel2] to beat 6 ...`, `start [Channel2] from the cued beat 6 with the other deck's beat 6`, `play armed (aligned start)`, `beat launch [Channel2] on beat 6`.
- 30 s both playing: xruns 0, late 0, slowest 2.1-3.5 ms.

## Part 2: sliders and knobs - PASS
- Crossfader full left and back (deck 2 cut: headphones stayed on deck 2, right by the AUTO CUE rule).
- HIGH deck 1, slow drag 150 px down: pointer counter-clockwise, ring dims (lower); double tap: back. The neutral position is still drawn at ~4-5 o'clock (round 20 question).

## Part 3: LEDs and cueing - PASS (one note)
- Deck 2 stopped (deck 1 playing), Phrasing LED 3: `cue [Channel2] at beat 3`; the deck went there, the LED turned solid white (asked: a white frame), nothing started (`p3_cued.png`; matrix 49.3).
- PLAY: `start [Channel2] from the cued beat 3 with the other deck's beat 3`, `play armed (aligned start)`, `beat launch [Channel2] on beat 3`; both rows lit on the same cell afterwards (`p3_started.png`).
- Deck 2 stopped, LED 5: `cue [Channel2] at beat 5` → 60.1 (bar 60 beat 1 = phrase beat 5); CUE: `cue_default pressed`, still 60.1 (`p3_c1.png`, `p3_c2.png`). PASS
- Both stopped, LED 5, PLAY: `button [Channel2],play 1`: started at once (no aligned wait), as asked.
- **Note:** after that start, a pause and CUE, the deck stood at 59.2 (`p3_state.png`), 3 beats BEFORE the cued 60.1. Either the cue point moved with the start or CUE went somewhere else; please check which.
- Playing deck jumps: unchanged from round 20 (not repeated).

## Part 3b: NEXT and played songs - PASS
- Deck 2 stopped, its ▸|: `next [Channel2]: load Aktysia` (after Addis, skipping 3 Sense on deck 1), again: `load Alba`. Deck 2 playing: `next [Channel2]: the deck is playing, nothing loaded`. Held 0.7 s: `next section [Channel2]`.
- `played in this set: <path>` for every song that started (3 Sense, Addis, Alba). In the list: ✓ before the title, title and artist dimmed (`p3b_list.png`, `p3b_list2.png`); Aktysia (only loaded) is not marked.
- After a restart the marks are gone (`p3b_restart.png`).
- Landscape note: the song list shows only ~3 rows beside the decks (`p3b_list.png`); usable, but very short.
