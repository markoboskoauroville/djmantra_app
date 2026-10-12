# Round 24 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 24 finished")

Build: the green Android run of 9a43d0b or newer. Retests of the round 23 fails plus the genre language.
Grep once per part: `adb logcat -d | grep -E "Quick:|WindowBridge|AndroidMidi|GenreLookup|Sound:"`.

## 0. Smoke (10 s): `Quick: interface ready`, not black, no `qml` error; `Sound output open: ... (clock)`; NEW:
`WindowBridge: frame rate <X> Hz -> 60.0 Hz` (report X; `frame rate not limited` = FAIL with the line).

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick): crossfader, one EQ knob.

## 3. CUE return (fix: a paused deck away from its cue now goes BACK to it, no new cue)
- Deck 2 stopped, press beat 17: `cue set ... at phrase beat 17`. PLAY (deck 1 playing), 2 bars, pause, CUE: log
  `button [Channel2],cue: back to the cue point`, the deck is on beat 17 and stays stopped. CUE again (now on the cue):
  `cue_default pressed`, it previews while held, back to 17 on release. Same with beat 3.
- Playing deck: CUE goes back to the cue and stops (as a CDJ).

## 4. CPU (fixes: 60 frames a second; the MIDI controller is no longer polled every 1 ms)
- Both decks playing, 30 s per tab (Songs, Phrasing, Mixer, Keys): the matrix meter and `top -b -n 1 -p <pid>`.
  On Phrasing and Mixer: `top -H -b -n 1 -p <pid> | head -25`, top 8 threads. Report the Controller thread (was 8 %)
  and the render thread (was 20.6 % on Phrasing). Waveforms still scroll smoothly (say if they stutter).
- Mix Ultra not plugged: the controller still connects later when plugged (only if Marko plugs it, else skip).

## 5. Genre in English (fix: Accept-Language en)
- Load 3 songs: the headers show English genres (e.g. "Electro", "Dance"), not "Elektronička glazba". Report them.
