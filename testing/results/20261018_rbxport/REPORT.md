# The rbxport port against rekordbox 7 (144 of Marko's songs, 83 with a constant rekordbox grid)

One offset for every song and both grids: -50.6 ms (rekordbox's clock against ffmpeg's).

| measure (constant rekordbox grids) | rbxport port | app now (QM) |
|---|---|---|
| BPM within 0.05 | 82 / 83 (99 %) | 78 / 83 (94 %) |
| BPM within 0.5 | 82 / 83 (99 %) | 79 / 83 (95 %) |
| 90 % of beats within 25 ms | 75 / 83 (90 %) | 56 / 83 (67 %) |
| 90 % of downbeats within 25 ms | 57 / 83 (69 %) | (no downbeat) |
| phrase starts: precision / recall within half a bar | 37 % / 23 % | |
| names, time agreeing (intro / low / full / outro) | 66 % | |



## How it was measured (18.10.2026)

- The port: `src/analyzer/rbxanalysis.cpp` (rbxport's onset, attack, tempo and downbeat stages in C++), run offline with
  `tools/analysis/rbx_build.sh` + `tools/analysis/rbx_run.py` on 150 songs of `djmantra-test-songs`, scored by
  `tools/analysis/rbx_compare.py` against rekordbox 7's export (`djmantra-test-songs/rekordbox`). The QM column is the
  app's grid from `testing/sections/rekordbox_vs_mixxx.json` (12.10.2026).
- **Where the port's beats sit:** the 900-9000 Hz band (the kick's click) averaged around every beat of "4B" peaks
  3 ms after the beat line, the full band reaches half its peak 2 ms after: on the attack. rekordbox's times run
  ~50 ms later on these MP3s (its clock keeps the decoder delay), so one offset of -50.7 ms is applied to every song
  and both grids; a grid half a beat off still counts as off.
- **Beat 1:** 59 of 82 songs on rekordbox's "1"; the misses are whole beats (1, 2 or 3) more than half beats. Four other
  weightings of rbxport's downbeat evidence (longer phrases more, the bass on 1) were worse (48-57 of 82).
- **Sections on the same grid** (82 songs, rekordbox's phrase starts, within one bar): the app's own finder (v3,
  "ENERGY CHANGES") F1 0.45 (precision 65 %), rbxport's phrase starts F1 0.39 (precision 49 %). So the app's finder stays
  the default and rbxport's is a choice in Settings.
- **Names** (rekordbox's phrase classes intro / low (Down, Bridge) / full / outro, time agreeing): 66 % with the page's
  rules; rekordbox's "Down" and "Bridge" are not Marko's breakdown (the beat stopping), so this is a loose check.
- Unit tests: `src/test/rbxanalysis_test.cpp` (a 120 BPM loop with a breakdown: every beat within 3 ms of its kick, the
  breakdown named at bar 33; the naming rules on bars).
