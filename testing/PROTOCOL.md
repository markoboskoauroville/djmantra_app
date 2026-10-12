# The test loop, standing rules (Marko, 12.10.2026)

> "Always send after the build to local Claude session a list of tests it needs to do. Coming from the most
> important things, first play/pause, testing all these sliders, LEDs, and then go to the more specific features ...
> send you each report immediately ... even if the connection drops, you can have all these bugs collected. And after
> the whole test is finished, then it needs to send you message test finish, and then you do fixing. And this
> workflow will need to continue forever until we come to the bug-free application ... Efficiency always, and speed
> always, and saving the credits as much as possible always."

Builder = the cloud session (codes). Tester = Claude Code on the phone (Termux, adb; never codes).

## Builder, every round
1. Fix / build, push ONE commit with `testing/ROUND<N>.md` (the ordered list below, the round's new items in place).
2. When that commit's Android run is green: `send_message` to the tester, one line:
   `round N ready, run <android run id> (commit <sha>), list testing/ROUND<N>.md`.
3. Collect the tester's part reports as they come (the repo is the record). Do not fix mid-round unless the tester is
   blocked (black screen, crash at start): then fix at once and say "round N restart".
4. On `round N finished`: read REPORT.md, fix everything that failed, write ROUND<N+1>.md, push, back to 1.
   Forever, until a round passes completely.

## Tester, every round
- Install, then test strictly in the list's order: the basics first. A FAIL in a basic part does not stop the round;
  write it and go on (only a black screen / crash at start stops it).
- After EACH part: append its rows to `testing/results/<UTC>_r<N>_phone/REPORT.md`, commit, push (the record survives
  a dropped connection), and `send_message` the builder ONE line: `rN part K: PASS` or
  `rN part K: FAIL - <what> (<log line>)`.
- At the end: push, then send `round N finished: X pass, Y fail`.
- Cheap and fast: one adb shell batch per part, `logcat -d | grep` for the lines named in the list, a screenshot only
  where the list asks (downscaled), no long transcripts in messages; the details live in REPORT.md.

## The order of every round's list (most basic first)
0. Smoke (10 s): installs, starts, `Quick: interface ready`, not black, no QML error.
1. Transport: PLAY / pause each deck (press, log `button ...play`), sound out (observe.py listen if available), CUE,
   SYNC.
2. Sliders and knobs: crossfader, both volume faders, pitch / tempo, every knob on the Mixer (GAIN, HIGH, MID, LOW,
   FILTER): each moves the sound and its LED ring; double tap resets.
3. LEDs: the matrix (time left, bar.beat, countdown, running lights), the Phrasing tab's big LEDs, the 8 / 16 / 32
   switch, beat launch on a stopped deck, jump on a playing one.
4. Screen: full screen, the navigation bar on a tap on empty space, hidden on any touch; tabs; portrait / landscape.
5. Library: songs listed, a tap loads onto the free deck.
6. The specific features, the round's new ones first (waveforms, sections, JUMP, keys, AUTOMIX, genre ...).
7. CPU and memory (`top`, both decks playing), loudness.
