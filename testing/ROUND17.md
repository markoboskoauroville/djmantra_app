# Round 17: the redesign (Marko, 12.10.2026)

Build: the first green Android run of the commit "New interface: the redesign ..." on ccr-eb7d1019-d3kubd.
New interface (GPU), portrait first. Smoke check (10 s) first: the interface draws, `Quick: interface ready`.

Marko: "we are repeating this phrasing LED lights multiple times. We need to have only one, and that's the one
with the matrix display ... The rest of the user interface needs to be saved for playlists and for overview
waveform." Top to bottom:

## 1. Layout (screenshot portrait and landscape)
- Top row: menu icon left, Camelot badge right, the camera hole clear.
- Deck headers: cover, artist + key, title, then TOTAL and REMAINING time only (`5:40   -4:16`). No phrase squares,
  no elapsed time, no bar.beat in the headers.
- Player controls: deck 1 `SYNC CUE PLAY JUMP`, deck 2 mirrored `JUMP PLAY CUE SYNC`. The JUMP icon is a drawn ▸|
  (round 16b: tofu).
- Mixing modes: `AUTOMIX  EQ  TEMPO BLEND  8 BEATS`.
- The matrix display: per deck (deck 1 above deck 2) elapsed time left (grey), bar.beat in the middle (deck colour),
  the countdown right (yellow), then the running lights. Nothing runs together (round 16b: "1"+"36.2" read "136.2").
- Two overview waveforms (deck 1, deck 2), the played part dimmed, the play position yellow.
- The rest: the playlist (tab Songs), the crossfader, the tabs Songs / Mixer / Keys.
- Gone: the 8 big beat pads, the phrase squares beside the covers, the phrase bar, the timed AUTO MIX.
- Landscape: the decks on the left, the playlist / mixer / keys, crossfader and tabs on the right.

## 2. Matrix and beats
- 8 BEATS → 16 BEATS → 32 BEATS → 8: the matrix shows that many cells per deck, bars (4 beats) set apart; the running
  light runs through all of them, beat 1 yellow. Log `Quick: phrase beats 16`.
- Beat launch now on the matrix: deck 1 playing, deck 2 stopped; tap deck 2's cell 1 (lower row, first cell): the cell
  blinks yellow, deck 2 starts on deck 1's next beat 1, both yellow cells together (log `beat launch [Channel2] on beat
  1`). A second tap on the blinking cell cancels. A tap on a playing deck's cell N jumps to beat N in time.
- Synced, aligned decks: both rows' lit cells one above the other, in 8, 16 and 32.

## 3. Player controls
- SYNC, CUE (hold), PLAY / pause per deck as before.
- JUMP: to the next section, in time. Log now in seconds plus the cues that count as section changes:
  `jump to the next section at 160.2 s; cues: intro_start_position 0.1 s, hotcue_1_position ...`.
  Round 16b: after JUMP the countdown said -9.0 where the detection's next change was ~14 bars away. Read the
  `cues:` list: is there a cue ~9 bars after the landing point? Report the line and the countdown.

## 4. AUTOMIX (Marko: "it means that we'll do EQ and tempo blend as automix. So I'm manually moving the fader")
- AUTOMIX on, EQ on, TEMPO BLEND on; deck 1 playing, crossfader full left; deck 2 loaded, playing (sync).
  Log `automix on`. Deck 2's EQs (Mixer tab) go to the bottom (killed) while the fader rests left.
- Move the crossfader slowly to the right: the button shows the percent; deck 2's LOW rises while deck 1's LOW falls
  first (bass swap), then HIGH, then MID; deck 1's BPM glides to deck 2's own BPM. Moving back undoes it.
- At the right end: `automix: [Channel2] is the outgoing deck`; the next blend runs right to left in the same order.
- AUTOMIX off: all EQs back to the top (0 dB). EQ off / TEMPO BLEND off: only the other part follows the fader.
- Listen: smooth? Report anything abrupt.

## 5. Playlist
- Songs tab: the folders (Music, Download); a tap on a folder opens it, ← goes up.
- A tap on a song loads it onto the free (stopped) deck (nothing playing: 1, 2, 1 …; one playing: the other; both:
  asks). The bar says `tap a song: free deck`.
- A tap on a deck's cover: Songs tab, the bar says `tap a song: deck N` (yellow); the next tapped song goes there.

## 6. Keys tab
- The Camelot wheel, then per deck: the code, KEY MATCH, − / +.
- KEY MATCH (round 16b: 12B → 7B against 8A, a diagonal): deck 2 in a B key against 8A now goes to 8B (relative);
  an A key goes to 8A. Log `key match [Channel2] 12B > 8A: -4 semitones`. The badge then says compatible / same.
- KEY MATCH off: back to the song's own key. − / +: one semitone (as round 16b, PASS).

## 7. Mixer tab: LED rings (OXI E16) on GAIN / HIGH / MID / LOW / FILTER, cut-only EQ (full ring = 0 dB, double tap
back to the top), faders, VU, PFL.

## 8. CPU (`top`, both decks playing, Songs tab) vs round 13 (32-55 %). Loudness as round 16 part 6 if time.
