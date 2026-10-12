# Round 29 on the phone - V7 (run 38319486655, commit 208a91a, versionCode 12)

Started by the tester's build watcher, 13.10.2026 01:12 local (23:12 UTC), Marko asleep. Portrait. The phone was awake with the notification shade down: closed (`cmd statusbar collapse`), screen kept awake for the round.

## Part 0: smoke - PASS
- Menu shows "V7" (yellow, underlined). `Interface: Qt Quick (GPU)`, `interface ready 413x919`, no qml error, `Sound output open ... (clock)`, `frame rate 120.00001 Hz -> 60.0 Hz`.

## Part 1: transport - PASS
- 3 Sense (deck 1) / Addis (NEXT, deck 2). `play 1`, `align [Channel2] at beat 5`, `start ... from the cued beat 5 ...`, `beat launch ... on beat 5`. 30 s both playing: xruns 0, late 0, slowest 3.2-3.7 ms. Both stopped afterwards.
