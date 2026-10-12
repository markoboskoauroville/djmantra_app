# Round 23 on the phone (Nothing Phone (2a), A142, Android 16)

Build: run 38265760924, commit 7478fb6. Started by the tester's build watcher, 12.10.2026. Landscape again (the phone is sideways, auto-rotate on).

## Part 0: smoke - PASS
- Ready in 4 s (`interface ready 919x413`), no qml error, `Sound output open: "Phone audio output ..." (clock) 48000 Hz, 2 channels`, `AudioRouter: 9 output(s) added` (`p0_start.png`).

## Part 1: transport - PASS
- 3 Sense (tap) and Addis (NEXT). PLAY deck 1: `play 1`, `auto cue: headphones to [Channel2]`; PLAY deck 2: `start [Channel2] from the cued beat 5 with the other deck's beat 5`, `beat launch ... on beat 5`.
- 30 s both playing: xruns 0, late 0, slowest 2.3-3.0 ms.

## Part 2: sliders and knobs - PASS
- Crossfader left and back; HIGH deck 1 dragged down: counter-clockwise, ring dims; double tap: back.
- Tester note: the start log had no `Quick: map` lines (my `logcat -c` ran before the layout logged them); a restart gave the map.

## Part 3: CUE after play-from-cue - FAIL
- 32 BEATS, deck 2 stopped, LED 17: `cue [Channel2] at beat 17`, `cue set [Channel2] at phrase beat 17` (46 ms later); matrix 37.1 (`p3_cued17.png`). PLAY (deck 1 playing): `start ... from the cued beat 17 ...`, `beat launch ... on beat 17`; played 2.3 s, pause (`play 0`), CUE (`cue_default pressed`): **the deck stood at 38.2 = beat 22, LED 22, not 17** (`p3_back17.png`).
- Again with LED 3: `cue set ... at phrase beat 3`, matrix 33.3; start waited 10 s for deck 1's beat 3, played 10 s, pause, CUE: **38.4, not 33.3** (`p3_back3.png`).
- The CUE press after a pause does not move the deck at all: the position after CUE is where the pause left it. This looks like Mixxx's default cue mode: CUE on a paused deck that is not on the cue point SETS a new cue there (in CDJ / Pioneer mode it returns to the cue point). Suggest `[ChannelN],cue_mode` = CDJ (or a return-to-cue of our own: `cue_gotoandstop`), and log the cue position on CUE.
