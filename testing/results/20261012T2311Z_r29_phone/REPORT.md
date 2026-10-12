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

## Part 3: sections in the app - PASS
- Instead of 8 songs, deck 1's NEXT walked the whole Drive150 list (decks stopped): **150 loads, 150 `Sections:` lines**, one per song, paired load → sections. `testing/sections/app_sections_v7.txt`: `<file>  N changes, at ... s` per song; the 8 asked (That Girl, Brick House, Get Down On It, Lovely Day, Anchor Point, Sauce, Mi Casa, Oh Honey) are in it.
- V7 finds **many** sections: 9-25 per song, mean 19.9 (e.g. Nanana 18, About Damn Time 21: every ~8 bars). If the counter should reset only on the big changes (Marko: "reset on every section"), check whether this is the major set or every phrase.
- Counter reset, 2 songs, played from the start, screenshot just after a change:
  - (It Goes Like) Nanana: changes at 7.5 s and 20.4 s → the middle number reads **1.2** at 7.9 s and at 20.8 s (one beat after 1.1) (`p3_counter_after_7.5s.png`, `..._20.4s.png`).
  - 4B: changes at 15.5 s and 32.6 s → **1.3** at 15.9 s and 33.0 s (`p3_4B_after_*.png`).
  The counter restarts at every section. PASS

## Round 29 finished: 4 pass (0-3), 0 fail
Nothing needed Marko; nothing skipped. Handover: docs/HANDOVER_PHONE.md.
