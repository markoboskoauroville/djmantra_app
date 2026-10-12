# Round 29 on the phone - V7 (run 38319486655, commit 208a91a, versionCode 12)

Started by the tester's build watcher, 13.10.2026 01:12 local (23:12 UTC), Marko asleep. Portrait. The phone was awake with the notification shade down: closed (`cmd statusbar collapse`), screen kept awake for the round.

## Part 0: smoke - PASS
- Menu shows "V7" (yellow, underlined). `Interface: Qt Quick (GPU)`, `interface ready 413x919`, no qml error, `Sound output open ... (clock)`, `frame rate 120.00001 Hz -> 60.0 Hz`.

## Part 1: transport - PASS
- 3 Sense (deck 1) / Addis (NEXT, deck 2). `play 1`, `align [Channel2] at beat 5`, `start ... from the cued beat 5 ...`, `beat launch ... on beat 5`. 30 s both playing: xruns 0, late 0, slowest 3.2-3.7 ms. Both stopped afterwards.

## Part 2: the analysis of the 150 songs - PASS
- Songs → Music → Drive150: `Quick: analysing 150 songs in advance (BPM, key)` at 01:14:12, the header counted down from "150 left", `Quick: analysis in advance finished` at 01:20:05: **150 songs in 5 min 53 s (~2.4 s each)**, the app in front, screen on, no crash.
- `testing/sections/app_analysis_v7.txt`: 150 lines `Analysis: <file>  bpm  key  first_beat_s  const`. 18 songs `const 0` (beat map). 93 BPMs are whole numbers, 57 are not (e.g. 122.5, 129.2, 104.3333, 115.0833).
- `testing/sections/app_beats_v7.txt`: 55 `Beats:` lines: the grid moved half a beat onto the kick, or "the tempo is not constant, a beat map instead of a grid". **The `Beats:` lines carry no file name**, so they cannot be matched to songs except by order; please add the file to that line.
- The 6 renamed files on the phone (" -> ', ? removed) appear under their phone names.
