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
