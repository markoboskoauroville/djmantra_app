# Round 15 (12.10.2026): everything found so far + Marko's four requests, one build

Build: green Android run of "Round 15 build: ..." or later on `ccr-eb7d1019-d3kubd`.
Output: `testing/results/<UTC stamp>_r15_phone/REPORT.md`, screenshots (+ `-hole`), logcat; push after each part.

A. Classic interface
   1. Bars: hidden at start; swipe from the top: bars show, the app makes room (logo below the clock),
      they hide again within ~5 s. Log "WindowBridge: bars shown/hidden, insets [...]".
   2. Logo → menu: its ✕ clear of the camera hole; Settings opens.
   3. Counter readable (~14 px), waveforms about twice as wide as before.
   4. Library (menu → LIBRARY), tap a song: free deck (nothing playing: deck 1, next song deck 2, then 1 ...;
      one deck playing: the other). Both playing: asks.
   5. Waveforms tab: zoom one deck → the other follows (same zoom).
B. New interface (`--qml` or Settings → "New interface (GPU)")
   1. Library: one LOAD per song → the free deck (same rule; log "Quick: free deck"); a deck cover tap → LOAD 1/2
      onto that deck; Music listed once; a playing target asks.
   2. One ZOOM − / + under both waveforms: both decks change together.
   3. Waveforms fill most of the lane (log "Quick: waveform gain [ChannelN] <g> (95 % level ...)").
   4. Sections: log "Sections: [ChannelN] N changes, at ... s" for each loaded song. In BARS mode the second
      number counts down to the next change (no "|"); at a change it jumps to the next one. Listen/look: does
      the change match the music (bass in/out, breakdown, drop)? Report a few times vs what you hear.
   5. Camelot wheel (tap the badge): drawn like the horoscope wheel (dark disc, tinted ring sectors, thin lines,
      code + musical name in each band, deck keys ringed, compatible keys lit, match in the middle). Screenshot.
   6. Landscape: SYNC/BPM/CUE/PLAY at the sides, page and crossfader in the middle (not run in round 14).
C. Loudness (not run in round 14): one song alone, volume up, crossfader to its side, 20 s:
   `adb logcat -d | grep "Output level"`.

## Added before the round (Marko, 06:27 and 06:33): test these in the new interface

D. Phrase lights, Marko's mixing tool:
   1. Beside each deck's cover: 8 small squares, 4 over 4; the square of the current beat of the 8-beat phrase
      lights (beat 1 yellow).
   2. The phrase bar under the deck headers, on EVERY page (mixer, waveforms, pads, landscape): deck 1's 8 steps
      over deck 2's, numbered 1-8, the current one lit (beat 1 yellow), and at the right each deck's countdown to
      its next section change (yellow, "-6.2"). Play both decks with SYNC: do both "1"s light together?
   3. The controller (when connected): the pads light the same beat (BEAT_LIGHTS in the Mix Ultra script).
E. Horoscope style: deep navy background, navy cards, one yellow accent (selected tab, LOAD, logo tile), grey
   second lines, element colours; bottom navigation bar with icon + label (Mixer, Waveforms, Pads), the selected
   one yellow; library rows with a glyph, the name and a grey second line (Folder / MP3 ...). Screenshots of each
   page next to Marko's manual/08-planets.png style.
