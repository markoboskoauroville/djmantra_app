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

## 3. Overview waveforms: big, sections framed (Marko, 12.10.2026, during the build)
Marko: "the waveform needs to be much bigger and the section colors more contrasty ... make vertical line between each
section so I can clearly see how the timeline is approaching that section ... clearly see the changes in arrangements".
- Same strip size as round 17. The waveform grows from the bottom and fills the strip: the loud parts reach the top
  (the 97th percentile of the song does), breaks visibly lower.
- Layers: highs light blue, mids green, bass red on top: a bass going out / coming back shows as the red appearing or
  vanishing.
- Sections: contrasting bands, blue / black alternating, framed by a white vertical line at each change (as soon as
  `Sections: [ChannelN] ...` is in the log). The NEXT change after the play position is a thick yellow line; when the
  play line passes it, that line turns white and the following one turns yellow.
- Do the frames sit where the arrangement changes (listen at 2-3; JUMP lands on the yellow one)? Is the white play
  line easy to follow towards the yellow one? Screenshot both decks playing.
- The played part dimmed, the play position a white line. Tap / drag seeks.

## 4. Big scrolling waveform (Phrasing tab)
- Tab 2 "Phrasing": top half of the page: both decks' scrolling waveforms, big, half each: deck 1 grows up and deck 2
  down from the line between them; frequency colours; the play line in the middle. + / − zoom both decks together.
- Scrolls smoothly while playing? CPU in `top` with this tab open vs the Songs tab.

## 5. Full screen (Marko: "running on top of everything, even the status line ... tap on the empty space ... invites
the navigation bar ... interact with any element ... it disappears")
- After start and after coming back from another app (home, then DJ Mantra again): no status bar, no navigation bar;
  the app fills the whole screen, around the camera hole. Screenshot.
- A tap on an empty spot (no button: e.g. a header's text, the gap between the matrix and the overviews) brings the
  NAVIGATION bar only (the status bar stays hidden). Log `Quick: show the navigation bar`, `WindowBridge: showing the
  navigation bar`, then `WindowBridge: bars shown`.
- With the bar there: a press on any button (it works as usual) hides the bar at once; a tap on the empty space hides
  it too (and does not bring it back). Unused, it hides by itself after ~4 s.
- A swipe up from the bottom edge still brings the bars (Android's own gesture); the next touch hides them.
- While the bar is there the app makes room for it (nothing under it), then takes the whole screen back.

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
