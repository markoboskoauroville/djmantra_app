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
