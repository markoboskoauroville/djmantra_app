# Round 18: Marko's 8 requests from round 17

Build: the first green Android run of the newest commit on ccr-eb7d1019-d3kubd that has this file.
New interface (default since 7171a0a). Smoke check (10 s) first: `Quick: interface ready`, not black.
Requests: testing/results/20261012T0737Z_r17_phone/REPORT.md.

## 1. Matrix: time left, counting down
- The matrix's left field per deck is the time LEFT in the song, `-4:16`, counting down while it plays (white).
  Middle: bar.beat; right: the countdown to the next section (yellow); under them the running lights (as round 17).

## 2. Deck headers: artist / title / key + genre, no times
- Line 1 artist (grey), line 2 title (bold), line 3 the key in the deck colour and the genre (yellow). No times.
- Deck 2 mirrored: its key sits at its outer (right) edge, next to its own cover (round 17: it read as deck 1's).
- Genre online: load a song whose file has no genre (check with `adb logcat | grep -i genre`):
  `Quick: looking up the genre of [ChannelN] <artist> - <title>`, then `GenreLookup: ...: <Genre>` and
  `Quick: genre <title> : <Genre>`; the header shows it. A song MusicBrainz does not know: `GenreLookup: no recording`
  / `none`, nothing shown. The phone needs internet. (MusicBrainz' official web service, one request a second.)
  Report a few songs: genre found? sensible?

## 3. Overview waveforms: contrast and sections
- Both overviews: the whole song in frequency colours (bass red, mids green, highs blue) on near-black, the loudest
  part filling the height; quiet parts (breaks) visibly lower.
- Sections: alternating lighter bands with a yellow line at each section start, as soon as `Sections: [ChannelN] ...`
  is in the log. Do the yellow lines sit where the music changes (listen at 2-3 of them; JUMP lands on the next one)?
- The played part dimmed, the play position a white line, the deck colour at the left edge. Tap / drag seeks.

## 4. Big scrolling waveform (Phrasing tab)
- Tab 2 "Phrasing": top half of the page: both decks' scrolling waveforms, big, half each: deck 1 grows up and deck 2
  down from the line between them; frequency colours; the play line in the middle. + / − zoom both decks together.
- Scrolls smoothly while playing? CPU in `top` with this tab open vs the Songs tab.

## 5. Full screen
- After start and after coming back from another app (home, then DJ Mantra again): no status bar, no navigation bar.
- A swipe up from the bottom edge brings the 3-button bar; it hides again by itself (~4 s) or on the next touch.
- A tap on an empty spot (no button: e.g. a header's text, the space between the matrix and the overviews) brings the
  bar (log `Quick: show the system bars`, `WindowBridge: showing the system bars`); the next touch anywhere hides it.
- The app makes room while the bar is there and takes the whole screen back after.

## 6. Keys tab: the Camelot wheel big
- The wheel fills the page's width (no help text under it); KEY MATCH and − / + for both decks below it.
- KEY MATCH (round 17 not tested): deck 2 in a B key against 8A goes to 8B; an A key to 8A. Log `key match ... semitones`.

## 7. PLAY / pause and the other toggles act on the press
- PLAY, SYNC, PFL, the modes: a press switches at once (before lifting the finger); a sliding finger still switches.
- Each press logs `Quick: button [ChannelN],play 1` (or 0): Marko's own presses show in the log. Ask Marko to try PLAY.

## 8. Phrasing tab: big beat LEDs
- Under the big waveforms: deck 1's LEDs, then deck 2's: 8 per row (8 BEATS: 1 row, 16: 2 rows, 32: 4 rows), the beat
  number in each, beat 1 of each row yellow-tinted, the current beat lit (deck colour, beat 1 yellow).
- A press launches / jumps exactly as a tap on the matrix's cell (round 17 part 2): a stopped deck arms (the LED blinks
  with a yellow frame) and starts on the other deck's same beat; a playing deck jumps to that beat in time.
- Easy to hit at 32 beats? Both rows step together when the decks are aligned.

## 9. Then the rest of round 17 (parts 3-8: JUMP and its `cues:` log line, AUTOMIX on the crossfader, playlist load to
the free deck, Mixer LED rings and cut-only EQ, CPU), as far as time allows.
