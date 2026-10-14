# Round 34 on the Pixel 7 - V12 (6ddbe77)

Tester "mantra dj app tester"; Pixel 7 over adb Wi-Fi, portrait. 13.10.2026 14:15 local.

## Part 1: the lanes and the scrub window show the wave - PASS
- **Phrasing lanes** (`phrasing_lanes_wave.png`), Nanana (deck 1, top) and 4B (deck 2, bottom), both playing:
  the wave is there: deck 1's hits are right-pointing arrows (sharp attack, decay) in red / orange / pink / violet,
  deck 2 mirrored below in teal / violet; orange bar lines numbered 8, 9, 10; grey beat lines; centre line. Next to
  djay's zoomed lane (`djay_nanana_zoomed_for_reference.png`): the same arrow idea and colours; the app's wave is
  much taller (half a lane per deck), djay's a thin band.
- **Scrub window** (`scrub_window_wave.png`): the thin mirrored wave in colour, bar 10 line, beat lines. PASS
- **A file never analysed on the phone** (a copy "Brick House r34.mp3" in a new folder, removed afterwards): the
  overview and the lane show the wave **1 s after loading** (`new_file_1s_and_4s_after_load.png`, left 1 s, right 4 s);
  `Sections: [Channel1] 4 changes, at 19.2, 181.4, 188.1, 194.8 s`. PASS
- Overview colours: Brick House teal / blue / green as djay (`cmp_brick_house.png`); the app's body is fuller
  (less thin spikes) than djay's.
- Log: `waveform shader [Channel2]: compiled`, `[Channel1] made, status 1, level 5.3125`.

## Part 2: CPU on the Phrasing page, both decks playing
`top` 5 samples: 74, 75, 56, 83, 48 % of a core (round 32: 48-72 %): a little higher with the wave drawn.
xruns 0, late 0, slowest callback 3.3-5.2 ms. Meter: C 8 %, G 5 %.

## Part 3: CQ parts - waiting: the Pixel's USB cable is on the Mac, not on the CQ.

## Part 4 (V13 dad7107): thinner as djay, CPU per thread
- **Lanes** (`v13_phrasing_lanes.png`): about half the height of V12: deck 1's arrows above the centre, deck 2's
  below, both thin bands now; nearer djay's thin lane (`djay_nanana_zoomed_for_reference.png`), still taller than djay's.
- **Scrub window** (`v13_scrub_window.png`) and **overview** (`v13_cmp_nanana.png`): the bright body thinner, the
  darker outline as before; breaks and colours as djay.
- **CPU**, Phrasing page, both decks playing: process 62, 76, 62 % (V12: 48-83 %). Per thread (`top -H`, 3 samples,
  names from /proc/<pid>/task/<tid>/comm):
  | thread | % of a core |
  |---|---|
  | qtMainLoopThread (Qt's main / GUI thread on Android) | 39, 46, 39 |
  | QtThread (render thread) | 14, 14, 11 |
  | AAudio_1 (the sound callback) | 11, 11, 11 |
  | CachingReaderWorker x2 | 3.5 each, now and then |
  | everything else (57 threads) | 0 |
  So the main thread (QML bindings / timers / JS) costs most, not the GPU render thread or the audio.

## Part 5 (V14 fc2e0fa): the matrix paints only its lit dots - PASS
- **The matrix looks the same** (`v14_matrix_v13_top_v14_bottom.png`: V13 above, V14 below, both decks playing):
  same digits, dim grid, the yellow / dim-yellow / blue phrase cells.
- CPU, both decks playing, 3 samples each (`top`, `top -H`; % of one core):
  | page | process | qtMainLoopThread | QtThread (render) | AAudio_1 |
  |---|---|---|---|---|
  | Phrasing (V13) | 62-76 | **39-46** | 11-14 | 11 |
  | **Phrasing (V14)** | **48-52** | **3.5-11** | 18-21 | 7-11 |
  | Songs (V14) | 21-32 | 3.5-7 | 0-7 | 11-14 |
  | Mixer (V14) | 28-32 | 0-7 | 11-18 | 7-15 |
  Binder and CachingReaderWorker threads 3.5 % now and then. The interface thread dropped by ~35 points; the
  render thread is now the biggest on Phrasing (the lanes). xruns 0.
