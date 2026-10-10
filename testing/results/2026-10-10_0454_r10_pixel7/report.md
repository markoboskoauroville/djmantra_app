# Round 10, Pixel 7 41241FDH2S10SJ, build 0.5.0-f7d3886, 10.10.2026 (Djapp local)

Ears: BlackHole (observe.py device 1) through the scrcpy mirror. Screens in `screens/`.

## Fixed and confirmed
- **Speed**: a 44.1 kHz 440 Hz tone now gives 442.3 Hz, a 48 kHz one 441.7 Hz (was 479.7). `ears-tone-44100`,
  `ears-tone-48000`. (The device_test tone is 44.1 kHz too: 441.9 Hz.) A +0.4% rest remains; maybe a pitch offset.
- **MP3** opens (8c83bcb already; see `../2026-10-10_0450_mp3_diag/`).
- **Picker**: Music is the real /storage/emulated/0/Music now (`picker-music.png`).
- **Mixer layout** as specified: HIGH/GAIN, MID, LOW, FILTER; 1·2, MASTER, headphones 1·2; CUE; the dj logo
  (`bars-portrait-6s.png`). The mixer's "1" opens the picker for deck 1 (`tap-mixer1.png`).
- **LOOP pads**: LOOP mode shows ¼ ½ 1 2 / 4 8 16 32; pad "4" → `[Channel1],loop_enabled = 1`, size 4 (the log
  line is loop_enabled, not beatloop_4_enabled).
- **Start screen**: the double-wave icon in the middle (`start-b.png`).
- ONE DECK in **portrait**: layout right; a tapped song plays at once (-29 dBFS) and its row turns orange;
  folder name in the header (`od-song1.png`).

## UI problems (first in the fixes)
1. **ONE DECK: ⏭ froze the app.** After ⏭ (05:00:10) the main thread logged nothing more; the screen stayed at
   the new title with -3:19.74; the engine played on until 05:00:38, then peak 0 and no more engine lines;
   Android logged `Qt A11Y: Could not run accessibility call in object context, accessing main thread could lead
   to deadlock` repeatedly. Play did nothing. Force-stop needed. `od-next.png`, `hang-logcat-all.txt` (no root
   for a thread dump).
2. **ONE DECK: the clock and the waveform never move.** TIME stays at the full length (-11:39.60) and "Ready to
   play, analyzing..." stays while BlackHole hears the song; BPM shows the previous track's value (140.1) and KEY
   is empty. The left of the waveform still shows the previous track (the tone's red zigzag). `od2-song.png`.
3. **ONE DECK: a swipe or tap on the overview pauses** (`[Channel1],play = 0`) instead of seeking; after that Play
   only logged play = 0 and stayed silent. So CONTINUOUS / STOP AFTER at the end of a song and the persistence
   could not be tested (`od2-seek.png`, `od3-after-end.png`).
4. **ONE DECK landscape is broken**: BPM/KEY/TIME cut off, waveform empty, folder list squeezed to one clipped row
   (`onedeck-landscape.png`).
5. **System bars**: on the swipe, `Window: the system bars take QMargins(0, 52, 0, 48)` and 0.3 s later
   `QMargins(0, 0, 0, 0)` while the bar is still shown: the app is full size under the bar at 0.4 s (portrait: the
   bar over the play/crossfader row; landscape: over deck 2's SYNC/pitch). 6 s later all is fine.
   `bars-portrait-0s.png`, `bars-landscape-0s.png`.
6. **Pads repaint**: after LOOP and pad 4, HOT CUE kept a white edge and pad 4 was only partly blue; just after
   the tap the old labels showed through (¼ over 1). `pads-loop4.png`, `pads-loop4-later.png`.
7. The pads mode is remembered: the LOOP I chose stayed for the UI test (see 8).
8. **ui_test** (`ui/report.md`): 140 controls, 45 FAIL. The 32 hotcue pad FAILs are from 7 (pads were in LOOP).
   Real: portrait both volume faders; waveform double-taps (both orientations); landscape deck 2's EQ/filter,
   bend, SYNC, pitch (under the bar, 5). The first run gave 57/62 FAIL because load_deck now opens the picker
   full screen and every later tap went into it: ui_test.py now presses Back after a load_deck tap (pushed).
   That run is kept as `ui-harness-picker-open/`.
9. The picker inside ONE DECK says "Folders" in the header while showing Music (`od-music.png`); small.

## Not done
- STOP AFTER and its persistence, the automatic mix (G) and the LED check: blocked by 1-3. The Mix Ultra is
  connected to the phone over Bluetooth (app log), the Mac has nothing driving it.
- Note: device_test turned auto-rotate back on; with the phone lying on its side the app came up in landscape.
  The phone is locked to portrait again.
