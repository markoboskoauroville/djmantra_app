# rekordbox 7.2.19 vs the app's analysis (Mixxx 2.5 QM), 144 of Marko's songs (12.10.2026)

rekordbox = the reference (Marko). rekordbox data: Marko's XML export -> `rekordbox_bpm_key.json`
(`rekordbox_xml.py`). The app's analysis run offline with the same code (`tools/analysis/mixxx_analyze.cpp`:
qm-dsp DetectionFunction + TempoTrackV2, BeatUtils fixed tempo + adjustPhase, GetKeyMode + global key) on
ffmpeg-decoded audio. Per song: `rekordbox_vs_mixxx.json`. rekordbox's XML has no phrases (sections need
its analysis files, `rekordbox_phrases.py`).

## BPM: 114 identical, 9 within 0.5, 4 double / half, 17 different
- Nearly all "different" ones are live-played disco / funk / soul (Brick House, Best of My Love, Upside Down,
  Get Down On It, Street Player ...): rekordbox follows the drummer with a DYNAMIC grid (300-1100 tempo
  markers, average 107.14 etc.); the app forces ONE constant tempo (Mixxx "assume constant tempo"), so its
  grid drifts against the music in those songs. **Catch 1: constant grid on live drums.**
- 4 double / half (e.g. Lotus Heart 90 vs 181.8): metrical level.

## Key: 79 identical (55 %), 27 relative major / minor, 19 a fifth away (Camelot +-1), 19 other
- The app uses Mixxx's default QM key detector. **Catch 2: the weaker key detector.** Measured against
  rekordbox (same key): QM 55 % (58 % with full overlap), Essentia bgate / shaath / edma 71-73 %,
  **KeyFinder 78 %** (117 of 150; 20 a fifth away, 5 relative, 8 other; Mac run, keyfinder_keys.json).
  Fix: KeyFinder is the default key detector from V6 (it is already in the app).

## Beat grid (78 songs with the same BPM and a constant rekordbox grid)
- rekordbox's beat line sits on the kick's onset (median -6 ms, 10 songs with a clear kick).
- The app's line is **59 ms early** (median), on every song: **Catch 3: a constant ~60 ms phase bias**
  (offline; the phone's decoder may shift it, to be measured on the phone).
- With +59 ms: 55 on the beat (+-25 ms), **21 on the OFF-beat (half a beat away)**, 2 other.
  **Catch 4: QM locks onto the off-beat (hi-hats) in ~27 %**: its raw beats split into an on-beat and an
  off-beat group, and the fixed-tempo step anchors the grid on the longest steady stretch, which can be the
  off-beat one (e.g. That Girl).
- Sections and the bar.beat counter are counted on this grid, so catches 3 and 4 move them too.

## Fix plan
1. Grid phase: after the QM grid, test the grid and the grid shifted by half a beat against the low band
   (kick) onsets; keep the one on the kick; then snap to the kick onset (removes the 60 ms bias).
2. Drifting tempo: when the raw beats do not fit one tempo (live drums), keep Mixxx's beat map (follows the
   drummer) instead of the constant grid.
3. Key: measure KeyFinder vs QM against rekordbox; make the better one the default.
4. Measure the app itself on the phone (its stored grids and keys for these songs), not only offline.
