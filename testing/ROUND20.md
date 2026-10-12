# Round 20 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 20 finished")

Build: the green Android run of the newest commit with this file. Marko's requests from round 19 (r19 REPORT.md):
both decks one colour (yellow), BPM and key analysed in advance; plus round 19's open items.
Grep once per part: `adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Sections:|jump|Output level|analys"`.

## 0. Smoke (10 s)
- `Quick: interface ready`, not black, no `qml` error. `Sound buffer: N of M frames`: N about 3072 now (round 19: 2048).

## 1. Transport (quick: round 19 passed)
- PLAY / pause both decks; logs `button [ChannelN],play 1` / `play 0` / `play armed (aligned start)` (round 19: an
  aligned start logged "play 0"); SYNC logs the NEW value (`sync_enabled 1` when it turns on); CUE logs `pressed`.
- Aligned start once, AUTO PHASE once (as round 19). Drops: 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick)
- Crossfader, faders, knobs as round 19. The Mixer tab now has map lines (`map knob_[ChannelN],pregain ...`,
  `map fader_...`). Knob direction (round 19 note): a drag DOWN must turn a knob DOWN (counter-clockwise, value lower);
  report which way it really goes with a slow `input swipe` from the knob's centre 150 px down.

## 3. LEDs (round 19 FAIL: 32-beat LEDs 37 dp)
- Phrasing tab at 32 BEATS, portrait: the scrolling waveforms step aside (hidden) and the LEDs take the page: each row
  at least ~46 dp (~120 device px on the A142). At 8 and 16 the waveforms come back above. Screenshot at 32.
- The LED map is logged again after each 8 / 16 / 32 change.

## 4. Colours (Marko: "both players same colour ... yellow: their times, beat LEDs and buttons")
- Both decks yellow everywhere: SYNC / PLAY when on, the matrix's time left (left) and bar.beat (middle) and running
  lights, the Phrasing LEDs, the key labels under the titles, the overviews' edge, the Keys tab rows. No green, no
  blue for a deck.
- The marked states are WHITE so they stay readable: a lit beat 1 (matrix cell and LED), a beat waiting to launch /
  jump (white frame, blinking white). The matrix's countdown (right) is white. Screenshot, both decks playing.

## 5. Screen (round 19 part 4 untested)
- Full screen after start and home → back; corners: nothing touches the rounded corners; the meter `C nn%` / `G --`.
- Tap on empty space: the navigation bar comes, the app makes room; a press anywhere hides it; unused ~4 s hides.
  Swipe up: bars come, next touch hides. Landscape: the bar covers nothing.

## 6. Library: BPM and key IN ADVANCE (Marko: "BPM scanning of the songs shown in a playlist ... automatically in
advance ... also the key")
- Open Music: songs without BPM / key (round 19: One for the Road, Open Up Your Eyes, One Last Call, Pieces) start
  analysing by themselves: log `Quick: analysing N songs in advance (BPM, key)`; the bar says `analysing BPM / key:
  N left`, counting down; the BPM and KEY columns fill in one song after another; at the end `Quick: analysis in
  advance finished`. Time it (per song).
- While it runs, both decks playing: xruns flat, late 0 (one analysis thread at low priority, no waveform).
- Close and reopen the app, open Music again: the values are there at once (saved in the library), no new analysis.
- Sorting by BPM / KEY with the complete columns. Cover, title, artist per row (round 19 part 5 untested).
- A tap loads onto the free deck; cover tap then a song: that deck.

## 7. The rest of round 19 (untested parts 6-7): JUMP / sections, genre, Phrasing zoom and waves, KEY MATCH,
AUTOMIX with the BPM glide, CPU and memory (`top`, both playing, Songs / Phrasing / Mixer tabs), as round 19 lists.
