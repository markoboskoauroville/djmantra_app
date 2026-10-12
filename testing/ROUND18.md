# Round 18 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 18 finished")

Build: the green Android run of the newest commit with this file. Marko's 8 requests: r17 REPORT.md.
Grep the log once per part: `adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Sections:|jump to"`.

## 0. Smoke (10 s)
- Install, start: `Quick: interface ready`, not black, no `qml` error. Black / crash: stop, report, wait.

## 1. Transport (round 17: Marko "PLAY doesn't work")
- Load a song on each deck (Songs tab, tap a song: goes to the free deck). PLAY deck 1: plays at the press (before the
  finger lifts), log `Quick: button [Channel1],play 1`; pause: `... play 0`. Same deck 2. Sound heard (observe.py).
- Ask Marko to press PLAY / pause himself a few times: every press shows in the log?
- CUE (hold = preview, release = back), SYNC on deck 2: tempo matches deck 1.

## 2. Sliders and knobs
- Crossfader left / middle / right: deck 1 only, both, deck 2 only (listen or VU on the Mixer tab).
- Mixer tab: both volume faders; GAIN, HIGH, MID, LOW, FILTER per deck: the sound changes and the knob's 15-dot LED
  ring follows (EQ: full ring = 0 dB at the top, cut only; FILTER half ring in the middle); double tap resets.
  One screenshot with the knobs at different values.

## 3. LEDs
- Matrix, per deck: left the TIME LEFT `-4:16` counting DOWN (new), middle bar.beat, right the countdown to the next
  section (yellow), under them the running lights; 8 / 16 / 32 BEATS changes the cells.
- Phrasing tab (2nd tab, new): deck 1's and deck 2's big beat LEDs, 8 per row (16: 2 rows, 32: 4), beat numbers, the
  current beat lit, beat 1 yellow. Deck 2 stopped: press an LED: it blinks (yellow frame), deck 2 starts on deck 1's
  same beat (log `beat launch`); a playing deck: press jumps to that beat in time. Easy to hit at 32?

## 4. Screen
- Full screen after start and after home → DJ Mantra again: no status bar, no navigation bar.
- Tap on empty space (a header's text, the gap above the overviews): the NAVIGATION bar only comes (log `Quick: show
  the navigation bar`); the next press anywhere (a button works as usual) hides it; a tap on empty space while it is
  there only hides it; unused it hides after ~4 s. Swipe up from the bottom: bars come, next touch hides them.
- Tabs Songs / Phrasing / Mixer / Keys; landscape once (screenshot).
- Top row: menu, key badge next to it (left of the camera hole), and top right the small matrix meter (Marko,
  12.10.2026): `CPU nn%` (the app's share of all cores, once a second, green < 50, yellow < 80, red) and `GPU nn%`
  or `--`. Log once: `Quick: GPU load from <file>` or `from nowhere`. Compare CPU with `top` (top shows % of ONE
  core: divide by the core count, `nproc`). Note the values on the Songs and the Phrasing tab.

## 5. Library
- Songs tab: Music and Download folders; a tap on a song loads onto the free deck; a cover tap then a song: that deck.

## 6. The new specific features
- Headers: artist, title, key + genre; no times; deck 2's key on its own (right) side.
- Genre online: a song with no genre in its file: `Quick: looking up the genre ...`, `GenreLookup: ...`,
  `Quick: genre <title> : <Genre>`, shown in the header. 3 songs: found? sensible?
- Overviews (same size as before): the waveform fills the strip from the bottom (bass red on top, mids green, highs
  blue); sections as blue / black bands, a white line at each change, the NEXT change a thick yellow line, which turns
  white when passed. Lines where the music changes (listen at 2)? Screenshot, both decks playing.
- Phrasing tab top half: both decks' big scrolling waveforms (deck 1 up, deck 2 down from the middle line), + / −
  zoom. Smooth while playing?
- Keys tab: the wheel as wide as the page; KEY MATCH (deck 2 B key vs 8A → 8B, log `key match ... semitones`); off;
  − / +.
- JUMP (▸|): to the next section in time; log `jump to the next section at X s; cues: ...`; lands on the yellow line?
- AUTOMIX on, fader rests left, deck 2 playing synced: moving the fader right swaps bass, then highs, then mids
  (Mixer knobs move), deck 1's BPM glides to deck 2's; back undoes; off resets the EQs.

## 7. CPU and memory
- `top -n 1 | grep -i djmantra` with both decks playing: Songs tab and Phrasing tab (round 13: 32-55 %).
