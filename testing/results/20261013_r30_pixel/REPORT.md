# Round 30 on the Pixel 7 - V8 (commit e8a02cb, release android-latest "DJ Mantra V8 (test build e8a02cb)")

Tester: Claude Code on Marko's Mac ("mantra dj app tester"), Pixel 7 over USB (41241FDH2S10SJ, Android 16).
Started 13.10.2026 10:21 local (08:21 UTC). Landscape, interface 914x411.

## Part 0: smoke - PASS
- Menu shows "V8" (`p0_menu.png`). `Interface: Qt Quick (GPU)`, `Quick: interface ready 914x411`, no qml error in logcat.
- `Sound output open: "Phone audio output (speaker, USB or Bluetooth)" (clock) 48000 Hz, 2 channels, 1024 frames, AAudio`.

## Part 1: basics - partly done, then STOPPED by Marko
- Drive150 analysis (opening the folder): `analysing 150 songs` 10:24:11 → `analysis in advance finished` 10:29:23: **150 songs in 5 min 11 s** (V7: 5:53).
- Nanana on deck 1 (tap), 4B on deck 2 (NEXT). PLAY 1 → `play 1`; PLAY 2 → `align [Channel2] at beat 2`, `start ... from the cued beat 2 with the other deck's beat 2`, `beat launch ... on beat 2` (aligned start PASS).
- 30 s both playing: `xruns 0`, `late 0`, slowest callback 2.1-4.1 ms (`p1_both.png`). PAUSE both: `play 0` each.
- Crossfader (portrait) dragged to the right: it moved (`p1_mixer.png`); no log line for it. Volume faders / knobs not done.
- Seen on the way (to check): the song list shows 4B as **6A**, the deck header shows **House 1A** for the same song. `Sections:` lines are logged **three times** per load.
- V8 sections on load: Nanana `8 changes, at 7.5, 118.3, 125.7, 140.4, 184.7, 197.7, 214.3, 221.7 s` (energy changes, level 3); 4B `6 changes, at 2.2, 32.6, 78.4, 84.1, 89.8, 173.6 s`.

## STOP (13.10.2026 10:31 local)
Marko: *"stop testing in the phone debugging. Please stop testing until the new version comes with waveform shape from DJay Pro. Please make this paramount and do it, and then continue testing. I want to see on screen the new waveform shape"*.
Decks paused, auto-rotate back on, screen stay-on off. Parts 2-6 wait for the build with djay Pro's waveform shape (testing/djay/README.md); then round 30 resumes from part 1 on that build.
