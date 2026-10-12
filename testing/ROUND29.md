# Round 29 (V6; rules: testing/PROTOCOL.md; Marko is asleep: skip anything needing a person and note it)

Build: V6 (13f9526 or newer, green Android run; releases/tag/android-latest). New analysis: KeyFinder key,
beat map for live drummers, sections from the model learned from rekordbox's phrases.
Grep: `adb logcat -d | grep -E "Quick:|Analysis:|Sections|Beats:|Sound:"`.

## 0. Smoke: "V6" in the menu, `Interface: Qt Quick (GPU)`, no `qml` error, sound output open, 60 Hz.

## 1. Transport (quick): PLAY / pause on both decks, aligned start; 30 s both playing: xruns flat, late 0.

## 2. The analysis of the 150 songs (the main part)
- Songs tab -> /sdcard/Music/Drive150 (opening the folder starts "analysing BPM / key: N left"). Leave the app in
  front until it reaches 0 and `Quick: analysis in advance finished` (screen on). It may take a while.
- `adb logcat -d | grep "Analysis:" > testing/sections/app_analysis_v6.txt` (one line per song: bpm, key,
  first_beat_s, const) and `grep "Beats:"` (songs that got a beat map). Push both files.

## 3. Sections in the app (8 songs: That Girl, Brick House, Get Down On It, Lovely Day, Anchor Point,
Sauce, Mi Casa, Oh Honey)
- Load each onto deck 1 (both decks may be stopped). Wait ~5 s, then `adb logcat -d | grep "Sections"`
  for that song (section times). Push them to testing/sections/app_sections_v6.txt.
- On 2 of them play across 2 section changes: the matrix middle number shows 1.1 on the first beat of each
  section (screenshot right after a change).

## 4. Report: testing/results/<date>_r29_phone/REPORT.md, "round 29 finished", then docs/HANDOVER_PHONE.md as
Marko asked, "phone done: <SHA>".
