# Round 9 (part 1), Pixel 7 41241FDH2S10SJ, build 0.5.0-7837ffd, 10.10.2026 (Djapp local)

8c83bcb was not released yet; this ran on 7837ffd. Steps C, D, A done; B and F, G follow.

## UI problems on the real phone (first in the fixes)

1. **The picker's Music is the app's private folder, not the phone's Music.** Deck 1 note icon → Folders shows
   Music and Download; Music contains only "Mixxx" (`screens/c1-picker.png`, `screens/c2-music.png`). The real
   /storage/emulated/0/Music has 13 folders and 8 loose files, 228 media files in all (`music-files.txt`). The
   folder shown is /storage/emulated/0/Android/data/com.djmantra.app/files/Music. The app HAS all-files access
   (appops MANAGE_EXTERNAL_STORAGE: allow); `--play /sdcard/Music/...` loads fine. A path bug in the picker.
2. **The navigation bar covers controls.** Portrait (ui_test `ui/portrait-mixer.png`): Android's bar sits over the
   bottom row: both Play buttons and the crossfader (ui_test FAILs for exactly those). Landscape
   (`after-pause.png`): the bar on the right covers deck 2's SYNC, BPM, pitch fader and Play (ui_test FAILs:
   [Channel2] sync_enabled, rate, rate_temp_up/down). After a fresh start in portrait the bars were hidden
   (`screens/c0-portrait.png`), so it comes and goes: the app should keep content inside the system-bar insets
   (or stay immersive).
3. ui_test: 104 controls, 24 failed (`ui/report.md`). Besides 2: portrait waveforms and pads tabs (page did not
   change), [Channel1] volume and pregain, the [Channel2] EQ knobs and filter, both volume faders; landscape both
   waveform double-taps.
4. Menu (round header button) and Settings look clean and Android-like (`screens/c3-menu.png`, `c4-settings.png`).

## Sound: the ears are BlackHole now

Marko's change done: the scrcpy mirror already running on the Mac forwards the phone's audio to the Mac output;
the Mac output was switched to "BlackHole 2ch" (observe.py audio device **1**) and BlackHole is recorded.

| | level dBFS | peak dBFS | strongest Hz | result |
|---|---|---|---|---|
| 440 Hz tone playing, Mac mic (device 2), before the change | -16.4 | -12.9 | 480.7 | heard: false |
| 440 Hz tone playing, BlackHole | -20.9 | -10.4 | 479.7 | heard: false |
| deck 1 paused, BlackHole | -120 | -120 | | silent: true |

5. **The tone plays 8.8% fast.** 440 × 48000/44100 = 479: a 44.1 kHz file played at the 48 kHz output without
   resampling (or rate off by that ratio). The app's own key detection says "Am" (right for 440 Hz), so the file
   is read correctly; it is playback. Heard both ways (mic and BlackHole), so not the capture.
6. The engine log's "Output level: peak 0.29914" kept repeating unchanged after the pause, while BlackHole was
   silent: the meter in the log looks stale.
7. tools/android/observe.py crashed on exact digital silence (log10 of 0 in tone_over_noise_db); fixed here.

## Controller and eyes

The Mix Ultra is on neither the phone (`dumpsys midi`: no devices) nor the Mac's USB, so the lights test (B) has
nothing to light. The Mac's cameras: built-in hung ffmpeg, Iriun gave an I/O error. Marko says the Pixel's own
camera is the eye (NDI Camera v136 on its home screen, scrcpy window on the Mac), which is the same phone as the
app under test: a photo means switching away from DJ Mantra.

## Also

The Pixel 7 lock is removed (Marko, 10.10.2026: one session). Phone left in portrait (rotation locked; before:
`screens/rotation-before.txt`).

## Step F: the phone's Music files (`music-files.md`, `music-files.json`, script `music_test.py`)

226 files (180 m4a, 39 opus, 6 mp3, 1 mp4), each copied to /sdcard/Music/.djtest/test.<ext> and started with --play.
**208 played with sound on BlackHole.**

8. **MP3 does not open at all: 6 of 6.** `SoundSourceProxy - Giving up to open file ".../test.mp3" after 2
   unsuccessful attempts`, no engine level, BlackHole -120 dBFS. The app still logs "Deck 1 playing" for it, so
   that line alone does not mean sound.
9. m4a 171/180, opus 36/39, mp4 1/1 above -60 dBFS. Of the rest, all but two have an engine peak (quiet intros,
   -60 to -86 dBFS in the 3 s window): not failures. #153 (Nahko - Budding Trees.m4a) and #208 (Shaman's Dream -
   Moonlight Saptah.m4a): no engine level line and no error; worth a look.
