# Round 25 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 25 finished")

Build: the green Android run of the newest commit with this file (includes b72fa77, 7c24ec5 which round 24 did not
have). Grep once per part: `adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Sound:"`.

## 0. Smoke (10 s): `Quick: interface ready`, no `qml` error, `Sound output open ... (clock)`, frame rate -> 60 Hz.
A second tap map ~6 s after the start: does it match the screen now (crossfader y)?

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick): crossfader, one EQ knob; Mixer tab VU meters move smoothly.

## 3. AUTO PHASE and CUE (fix: the on-air deck never moves; a CUE preview is ignored)
- Deck 1 on air (playing, volume up, crossfader middle or left). Deck 2 paused on its cue, a different phrase beat.
  HOLD deck 2's CUE 3-4 s (preview), release: deck 1 does NOT jump (no `auto phase: [Channel1]` line); after the
  release deck 2 is back on its cue.
- Both playing, deck 2's volume DOWN, phrases misaligned: AUTO PHASE moves deck 2 (`auto phase: [Channel2] ...`),
  never deck 1. Volume up on both, crossfader middle: the old rule (the deck heard less / not the one you moved).
- CUE on a PLAYING deck 3 times: each press logs `cue press [ChannelN]: deck at X s, cue at Y s, playing`; it
  stops on Y each time. Report every line; if it stops elsewhere, the line and where it stopped.

## 4. CPU (fix: the time left, the overview, the CUE button and the VU meters update 4-30 times a second, no longer
on every audio buffer)
- Both decks playing, 30 s per tab: the meter and `top -b -n 1 -p <pid>`; `top -H` top 8 threads on Songs and on
  Phrasing. Check the time left still counts each second, the overview playhead moves, VU meters still look live.

## 5. Genre: the Croatian leftover ("Klasična glazba" on Addis) is looked up again and turns English. Report it.
