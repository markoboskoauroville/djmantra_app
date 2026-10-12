# Round 16 (12.10.2026): the one complete test of the redesign (Marko: test once, efficiently)

Build: the green Android run of "Redesign: mixing by phrase lights and sections ..." or later on
`ccr-eb7d1019-d3kubd`. Test in the NEW interface (`--qml` or Settings → "New interface (GPU)").
Output: `testing/results/<UTC stamp>_r16_phone/REPORT.md`: table what | expected | result | pass/fail, one
screenshot per screen (+ `-hole`), logcat (`grep -E "Quick:|Sections:|jump to the next|Interface:|FATAL"`).
Push once at the end (and once halfway if the session might stop). Two of Marko's songs on the decks.

## 1. Screen and style (one pass, portrait then landscape)
- Horoscope style: navy background and cards, one yellow accent, grey second lines; bottom navigation
  Mix | Mixer | Keys (drawn icons, selected yellow); nothing under the camera hole, no text cut, no bars over it.
- No waveforms anywhere (no waveform page, no overview). No hot cues, no loops in the interface.
- Deck headers: cover, 8 phrase LEDs (4 over 4) beside it, artist + Camelot key, title, counter (tap: TIME ↔ BARS).
- Phrase bar under the headers on every page: deck 1's 8 steps over deck 2's, each deck's section countdown.

## 2. Mix page (the main tool)
- Each deck: 8 big beat pads (the running lights: the current beat lit, beat 1 yellow), the NEXT SECTION box
  (countdown, JUMP), the key box (code, KEY MATCH, − / +); AUTO MIX bar between the decks.
- Beat launch: deck 1 plays, deck 2 stopped. Tap deck 2's pad 1: it blinks, deck 2 moves to its beat 1 and starts
  when deck 1 reaches its beat 1, in sync (log "beat launch [Channel2] on beat 1"; both phrase rows light "1"
  together; listen: in time?). Try pad 5 too. A second tap on the blinking pad cancels.
- Beat jump while playing: tap pad 3 on a playing deck: it jumps to beat 3 of its phrase, staying in time.
- JUMP: the deck jumps to the next section start (log "jump to the next section at"), in time if playing;
  the countdown restarts from the following section.
- Sections: log "Sections: [ChannelN] N changes, at ... s" per song; compare a few with what you hear (bass in/out,
  breakdown, drop). Countdown never "|" when changes were found before the end.
- KEY MATCH on the incoming deck: its key code changes to match (or neighbour of) the other deck; off: back to its
  own key. − / +: one semitone down / up (key code changes; tempo unchanged).
- AUTO MIX (deck 1 playing, deck 2 loaded and stopped, EQ and TEMPO BLEND on, 16 BARS): the button shows WAIT 1,
  deck 2 starts on deck 1's next beat 1; percent runs; EQ knobs (Mixer page) move: bass swapped first, then highs,
  then mids, then deck 1 fades out and stops; deck 1's BPM glides to deck 2's own BPM. Repeat with EQ off and
  8 BARS. Listen: smooth? Report anything abrupt.
## 3. Mixer page: knobs, faders, VU, PFL as before. 4. Keys page: the Camelot wheel (horoscope style), deck keys
ringed, compatible lit, match in the middle; the badge (top right) opens it too.
## 5. Library: one LOAD per song → the free deck (nothing playing: 1, 2, 1 …; one playing: the other; both: asks);
a cover tap → LOAD 1 / LOAD 2 onto that deck; Music listed once.
## 6. Loudness: one song alone, volume up, 20 s: `adb logcat -d | grep "Output level"`.
## 7. Controller (only if connected): pads light the beat in the phrase (BEAT_LIGHTS).
