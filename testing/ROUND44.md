# Round 44: V92 on the Pixel 7 (Pioneer USB export)

Tester: the local Claude Code session on the Pixel 7 (adb over wireless debugging, see docs/HANDOVER.md).
Screenshots to Download/djm_v92/. Report each part when done (PASS / FAIL, what you saw, logcat lines).

0. Install V92 (release "android-latest", phone-arm64); versionName V92; starts; crash grep empty.
1. Basics: PLAY / pause on both decks, a fader and a knob move, the LEDs follow (as round 43).
2. Settings -> scroll to PIONEER USB (CDJ / XDJ): it lists "Documents/DJ Mantra/Pioneer USB" (and a USB entry if a
   stick is in the phone); the playlists are listed as buttons; ALL PLAYLISTS is lit. Screenshot.
3. Tap EXPORT TO PIONEER USB (folder target, ALL PLAYLISTS): the title shows "n/N <song>" while it runs, then a toast
   "PIONEER USB READY: N songs, M playlists, X MB -> ...". logcat `Pioneer USB: done,`. Note the time it took.
4. adb pull "/sdcard/Documents/DJ Mantra/Pioneer USB" to testing/results/<date>_pioneer/ (the PIONEER and Contents
   folders) and commit it if under 50 MB (else only PIONEER/). The cloud reads export.pdb and the ANLZ files back.
5. One playlist only: tap one playlist button (it lights, ALL goes dark), EXPORT again; the toast names 1 playlist.
6. Optional, if the Mac has rekordbox: copy PIONEER and Contents to the top of a FAT32 USB stick and plug it into the
   Mac; rekordbox shows it under Devices with the playlists and each song's BPM, key and waveform. Screenshot.
7. 2 minutes of play, then the crash grep. Send "round 44 finished".
