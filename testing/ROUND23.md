# Round 23 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 23 finished")

Build: the green Android run of the newest commit with this file. Fixes of the round 22 fails and notes.
Grep once per part: `adb logcat -d | grep -E "Quick:|SoundDeviceOboe|AudioRouter|GenreLookup|Sound:"`.

## 0. Smoke (10 s): `Quick: interface ready`, not black, no `qml` error; `Sound output open: ... (clock)`.

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick): crossfader, one EQ knob.

## 3. CUE after play-from-cue (fix: the CUE landed 3 beats early)
- Deck 2 stopped, a beat pressed (e.g. 17): log `Quick: cue [Channel2] at beat 17`, then `Quick: cue set [Channel2] at
  phrase beat 17` (the CUE is now set once the deck IS on that beat, not on a timer). PLAY (deck 1 playing), let it
  play 2 bars, pause, press CUE: the deck is back exactly on beat 17 (matrix and Phrasing LED). Repeat with 3 beats.

## 4. CPU (fix: hidden tabs are no longer made: Phrasing, Mixer, Keys only exist while shown)
- Both decks playing, 30 s on each tab (Songs, Phrasing, Mixer, Keys): the app's CPU from the matrix meter AND
  `adb shell top -b -n 1 -p <pid>` per tab. Then `adb shell top -H -b -n 1 -p <pid> | head -25` on the Phrasing tab
  and on the Songs tab: report the top 8 THREADS by name and %CPU (this finds the real culprit).
- Each tab still works after switching back and forth (Phrasing LEDs light, Mixer knobs move, the wheel shows keys).

## 5. Audio router (fix: the list showed only "Phone default": the app now reads the outputs itself)
- Settings: the output list shows the phone's outputs; log one line per output `Quick: audio output <key>|<label>|<ch>`
  (speaker at least). Report all lines. The ← button stays at the top while the page scrolls (fix).
- Choose MASTER = speaker, HEADPHONES = phone default, APPLY: sound plays, xruns flat.
- Needs Marko (only here): plug the Mix Ultra or a Bluetooth / USB-C output in; the list grows within ~1 s (log
  lines again), MASTER on it, HEADPHONES on the speaker, APPLY: both play. Multichannel device: stereo pair 3-4 offered.
  Unplug: back to the phone default, no crash.

## 6. Genre (fix: Deezer found nothing 4 times in 4)
- Load 5 songs. Each logs `GenreLookup: Deezer https://api.deezer.com/search?...q=... -> N tracks` (and a second,
  plain "artist title" search if the first found 0), or `GenreLookup: HTTP <status> for <url>`. Report: genres found
  of 5, and every Deezer / HTTP line for the songs without a genre.

## 7. AUTOMIX EQ (new log; the EQ curves of round 22)
- AUTOMIX + EQ on, both playing, move the crossfader slowly from one end to the other (~10 s): logs `Quick: automix eq
  N%: out hi .. mid .. low .. | in hi .. mid .. low ..` every 10 %. Report the 11 lines. Check: the outgoing low at
  1.00 until 45 %, both lows swap over 45-55 %, the incoming hi up before its mid; no value stuck.
