# Round 41 (V42; Pixel 7; rules: testing/PROTOCOL.md)

Marko 14.10.2026: "write the length of the song overlaid over the album art ... similar as YouTube ... the library
needs to have two views: list view and thumbnail view."

## 0. Basics: installs, starts, `Quick: interface ready`, PLAY / pause both decks.
## 1. Songs tab, LIST view: every song's cover has its length bottom right, white on a dark box (3:07, 1:02:45);
songs without a cover too (on the note). Sort by TITLE / BPM: the lengths stay with their songs (they were lost by a
sort before). Compare 3 lengths with the deck's time display after loading.
## 2. The view button in the top bar (left of the stats): THUMBNAILS: 3 covers a row (portrait; more in landscape),
title and artist under each, the length bottom right on the cover, BPM and key top left; played songs dimmed with a
tick; a tap loads onto the free deck; folders open. Tap again: LIST. Leave the app and come back: the view is kept
(log `library view: ...`). The controller's BROWSE knob / LOAD work in both views.
## 3. Screenshots of both views (downscaled) into the results folder.
## 4. r40 fixes: right after start HIGH / MID / LOW on both decks are in the middle (log: no EQ at max); the
volume fader: its label must say VOLUME while testing it (a still hold of 0.5 s flips it to PITCH, which would explain
r40's "fader barely changes the peak" and "90.07 BPM": grep `fader [Channel` in logcat).
## 5. r40 part 6: the PITCH scale's middle = the deck's BPM where the song is (local_bpm), the same number as the
handle at 0 % (was the library's 114.6 while the deck played at 91).
