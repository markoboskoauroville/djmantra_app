# Round 30 (V8; rules: testing/PROTOCOL.md; tester: "mantra dj app mac" on Marko's Mac, Pixel 7 over USB)

Build: V8 (the newest green Android run on branch ccr-eb7d1019-d3kubd; releases/tag/android-latest, tap the version
in the app's menu to reach it). New in V8: **a section only at a big change of energy** (Marko 13.10.2026: "not on
every one, only on the big change of energy"): ~8 sections per song instead of ~20; the matrix's bar.beat counter
restarts at 1.1 only there. The `Beats:` log lines now name the song's file.
V8 also has **Settings -> SECTIONS** (4 algorithms: ENERGY CHANGES default, PHRASES, ENERGY ONLY, CLASSIC 8-BAR; sensitivity
1-5) and **Settings -> BEATS** (grid AUTO / ALWAYS STEADY, GRID ON THE KICK on / off, ANALYSE THE STOPPED DECKS AGAIN).
Every choice is logged: `Settings:`, `Sections options:` (algorithm, level, each change's strength), `Beats options:`.
Grep: `adb logcat -d | grep -E "Quick:|Analysis:|Sections|Beats|Settings:|Sound:|MidiBridge"`.

**Talking to Marko (his rule, 13.10.2026): only by voice**, through the Mac's own voice engine on the Adam Audio D3V
speakers (e.g. `say "Marko, on the DJ controller, press ..."`), and only for what the camera and adb cannot do
(press a controller button, turn on the controller). One action at a time; wait, then check by camera / screenshot /
log that it happened. The Pixel faces the DJ controller: use its camera (screenshots) to see the controller's lights.
Songs: if /sdcard/Music/Drive150 is not on the Pixel, clone the private repo markoboskoauroville/djmantra-test-songs
and `adb push` its mp3 files to /sdcard/Music/Drive150.

## 0. Smoke: "V8" in the menu, `Interface: Qt Quick (GPU)`, no `qml` error, sound output open.

## 1. Basics: PLAY / pause on both decks, a fader, a knob, the crossfader, aligned start; 30 s both playing:
xruns flat, late 0.

## 2. Sections of all 150 songs
- Songs -> Drive150, wait for `Quick: analysis in advance finished`. Then NEXT on deck 1 through the whole list (as
  in round 29) and `adb logcat -d | grep "Sections"` -> testing/sections/app_sections_v8.txt (one line per song).
  Also `grep "Analysis:"` -> app_analysis_v8.txt and `grep "Beats:"` -> app_beats_v8.txt. Push all three.

## 3. Eyes and ears (the main part): 8 songs: That Girl, Brick House, Get Down On It, Lovely Day, Anchor Point,
Sauce, Mi Casa, Oh Honey
- Open each song in djay Pro on the Mac and screenshot its whole waveform (the big energy changes are clearly
  visible there: Marko's reference). Write next to it the app's V8 section times for that song.
- For each app section: is it on a visible big change in djay Pro's waveform (yes / no / off by N bars)? Any big
  change in djay Pro that the app does NOT mark? Listen where unsure.
- On 2 songs play across 2 section changes in the app: the matrix shows 1.1 on the first beat of each (screenshot).
- Push testing/sections/djay_vs_v8.md (+ screenshots in testing/sections/djay_v8/).

## 4. Settings: sections and beats
- Settings -> SECTIONS: on one loaded song try each algorithm, and for ENERGY CHANGES levels 1, 3, 5. Each tap:
  `Sections options:` in the log with the new algorithm / level and the overview's section marks change at once.
  Push the log lines (testing/sections/app_options_v8.txt). Which algorithm / level matches djay Pro's visible
  changes best on 3 of the songs of part 3?
- Settings -> BEATS: ALWAYS A STEADY GRID + GRID ON THE KICK off, ANALYSE THE STOPPED DECKS AGAIN with a song on a
  stopped deck: `Beats options: ... grid constant ... on the kick off`, the grid redrawn, then the sections again.
  Put both back to the defaults afterwards.

## 5. The DJ controller over Bluetooth only (Hercules DJControl Mix Ultra; no USB cable)
- Ask Marko by voice to switch the controller on (Bluetooth). The app looks for it by itself: `MidiBridge: Found
  ... over Bluetooth`, `Connected (bluetooth)`. Time from switch-on to connected.
- Then by voice, one at a time, while you watch the app (screenshots) and the controller (Pixel camera): PLAY on
  deck 1 and 2, a volume fader, the crossfader, an EQ knob, the jog wheel, BROWSE (song list opens) + LOAD, the 4
  pad mode buttons (pages), the pads. Per control: works / wrong / nothing, and the latency you see.
- The controller's LEDs: PLAY lit while playing, the beat lights.
- Report every control that fails with its `MidiBridge: in ...` bytes.

## 6. Report: testing/results/<date>_r30_pixel/REPORT.md, then "round 30 finished".
