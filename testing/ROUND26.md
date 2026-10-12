# Round 26 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 26 finished")

Build: the green Android run of the newest commit with this file. Grep once per part:
`adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Sound:"`.

## 0. Smoke (10 s): `Quick: interface ready`, no `qml` error, `Sound output open ... (clock)`, frame rate -> 60 Hz.

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick): crossfader, one EQ knob; VU meters (now 20 updates a second) still look live.

## 3. CUE stays put (fix: only a beat pressed by hand sets the CUE; an aligned PLAY no longer moves it)
- Deck 2: press a beat by hand (cue set, log `cue [Channel2] at beat N`), PLAY aligned, pause, CUE: back on it.
  Then PLAY aligned 3 times in a row with pause + CUE between: the `cue at Y s` in `cue press` stays the SAME Y.
- PLAY with no beat pressed (aligned start): log `align [Channel2] at beat N`, and the CUE point does not move.

## 4. CPU (fix: the scrolling waveforms take at most 60 position updates a second; VU 20 a second)
- Both decks playing, 30 s per tab: meter and `top -b -n 1 -p <pid>`; `top -H` top 8 threads on Phrasing and Mixer.
  Waveforms on Phrasing still scroll smoothly (say if not).

## 5. Genre match (fix: only a Deezer track by a matching artist; the match is logged)
- Clear "Addis"'s genre if you can (or load 3 songs without one): `GenreLookup: Deezer match: <artist> - <title>
  (album ...)` per song. Report each match and the genre; a wrong match = FAIL with the line.

## 6. Clock (new, Marko 12.10.2026: "real-time clock in the matrix display style" at the top)
- Needs the build with ClockDisplay.qml (re-install if parts 4-5 ran on the older one). Top bar, left of the C/G
  meter: HH:MM in yellow matrix dots, the colon blinking each second; it matches the phone's time and turns over at
  the minute. Portrait and landscape: not under the camera hole, not overlapping the key badge or the meter.
  Screenshot of the top bar.
