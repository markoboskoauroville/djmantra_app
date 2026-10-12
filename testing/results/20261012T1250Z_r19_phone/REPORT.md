# Round 19 on the phone (Nothing Phone (2a), A142, Android 16, 1084x2412)

Build: run 38233904582, commit 1a724b3. Started on Marko's word ("please start testing"), 12.10.2026 12:50 UTC.

## Part 0: smoke - PASS
- Installed, started in 6 s: `Quick: interface ready 413x919`, not black, no qml error in the start log.
- `Quick: insets 0 0 0 0 corner 50`. `Sound buffer: 2048 of 2048 frames` (ROUND19 expected about 3072: noted for part 1 DROPS).
- Screenshot `p0_start.png`: full screen, no navigation bar; the meter top right shows `C` / `G` with `1%` and `--`.

## Part 1: transport - PASS (by log and screen; not by ear)
- Loaded 3 Sense (deck 1) and Addis (deck 2) from the library. PLAY on deck 1: `button [Channel1],play 1`, sound (peak 0.04 → 0.6).
- Aligned start, 5 presses of deck 2's PLAY at random moments: each `align [Channel2] to beat N with the other deck` then `beat launch [Channel2] on beat N` (N = 7, 5, 1, 7, 6). Screenshot `p1_aligned.png`: both decks' lit matrix cells in the same column.
  Note: the start press logs `button [Channel2],play 0` (not `play 1`), the stop press also `play 0`. Cosmetic in the log; the deck does start.
- AUTO PHASE: off → `auto phase off`, LED 5 on deck 2 moved it out of phase and nothing moved back. On → `auto phase: [Channel2] (beat 5) to the other deck's beat`, `align ... beat 4`, within ~2 s. Marko-style jump on deck 1 (LED 3): deck 2 followed (`auto phase: [Channel2] (beat 2) ...`, `align ... beat 4`). `p1_follow.png`: both rows lit on 4.
- ALIGN ▸| tap (AUTO PHASE off, deck 2 off by 2 beats): `align [Channel2] to beat 8 ...` + `jump ... on the next beat`. Held 0.7 s: `next section [Channel2]`.
- SYNC deck 2: `sync [Channel2]: deck 1 122.00 bpm, deck 2 122.00 bpm` - equal. (The press logged `sync_enabled 0` though the button turned blue/on.)
- CUE: a tap on CUE (either deck) logs no `button ... cue` line at all; after CUE + PLAY the deck plays (aligned start). Check whether the tap reaches the engine.
- DROPS: `Sound buffer: 2048 of 2048 frames` (ROUND19 said ~3072). 60 s on Phrasing, both playing, SYNC on: xruns stayed 1 (the one from start), late 0, slowest callback 2.4-3.4 ms. KEY MATCH on deck 2 (`key match [Channel2] 8A > 10A: 2 semitones`), 30 s: xruns 1, late 0, slowest 4.0-4.3 ms.
  Not done: a sound recording to count silences (no audio capture over adb on Android 16); Marko's ear needed.
- The key badge (top) opens the Keys tab.

## Part 2: sliders and knobs - PASS (quick check)
- Crossfader L / R / M: the thumb follows the finger to both ends and back (`p2_xf_left.png`, `p2_xf_right.png`). No log line for it, so the sound side is not checked (not by ear).
- Volume fader deck 1 dragged to half (`p2_moved.png`), double tap: back to full (`p2_reset.png`).
- HIGH knob deck 1: a drag DOWN turned it clockwise (up). Check the direction: on DJ apps a drag down turns a knob down. Double tap: back to centre; its LED ring follows.
- The meters between the faders do not change with the fader or the crossfader (pre-fader). Fine if intended.
- The Mixer tab emits no `Quick: map` lines (tapped by screen coordinates).

## Part 3: LEDs - FAIL (only the size at 32; everything else PASS)
- 32 BEATS (`p3_32beats.png`): each LED cell ~97 device px high (~37 logical; pitch ~115 px). Up from 19 dp in round 18, but under the asked ~46 logical / ~120 device px. The waveforms shrink to one thin strip as asked. 8 and 16 BEATS (`p3_8beats.png`, `p3_16beats.png`) fine.
  The tap map is not re-emitted when the beat count changes (only `led1_1..8` from the 8-beat layout); I tapped by coordinates.
- Quantized jumps, AUTO PHASE off, deck 2 playing: LED 1 pressed 3 times in a row → 3 x `jump [Channel2] to beat 1 on the next beat` (round 19's bug fixed). Then 32, 4, 1 → each logged. xruns stayed 1, late 0, slowest 4.3-7.1 ms. The yellow frame was not caught on a screenshot (screencap is slower than a beat). Continuity of the groove: not by ear.
- Switching 8 → 16 → 32 with AUTO PHASE on moved deck 2 by itself (`auto phase: [Channel2] (beat 10) ...`, `align ... beat 27`): aligned in 8 was not aligned in 32. Probably right; Marko to confirm he wants the beat count switch to move a deck.
- Launch on a stopped deck (32 BEATS): LED 1 turns yellow at once (`p3_launch.png`), the deck starts on deck 1's next phrase start: `beat launch [Channel2] on beat 1` (logged at the launch, ~16 s later, nothing at the press).
- Matrix: time left counts down (-1:51 → -1:09 ...), 8 / 16 / 32 cells shown.

## Part 4: screen - STOPPED (partial)
- After home → back: full screen. Tap on empty space: `WindowBridge: bars shown, insets [0, 0, 0, 126]`, `Quick: insets 0 0 0 48 corner 50`; the app makes room: the tabs sit above the bar (`p4_bars.png`). The bar hid by itself within ~6 s (`p4_after6s.png`), but no `WindowBridge: bars hidden` line was logged.
- Then Termux came to the front (Marko watching), so the button-press / second-tap / swipe-up checks did not reach the app. Not judged.
- Parts 4 (rest), 5, 6, 7 not tested: Marko asked for new features (below), testing stopped.

## Marko's new requests (12.10.2026, ~15:05, verbatim)

> "Please change the color scheme, both player side same color, not different color schemes. And automating BPM scanning of the current songs shown in a playlist, that must happen automatically in advance. And not only the BPM, also the key update, and then test."

### Shaped for the builder
1. **One color for both decks.** Deck 1 and deck 2 look the same everywhere: SYNC / PLAY buttons, the matrix numbers and cells, the overviews, the Phrasing LEDs, the key labels, the Keys tab rows. No green-left / blue-right.
2. **BPM analysed in advance, automatically.** Every song shown in the open folder / playlist gets its BPM analysed in the background without being loaded onto a deck, as soon as the folder is shown; the BPM column fills in by itself.
3. **Key analysed in advance too.** Same for the key: the KEY column (Camelot) fills in for every song shown, without loading it.
4. Then round 20: test it.

### Tester findings (suggestions, from round 19 on the phone)
- Today deck 1 is green and deck 2 blue (`p1_aligned.png`, `p3_32beats.png`): play/sync buttons, matrix time / position digits, overviews, LED highlight, key text under the title, Keys tab rows 1 / 2.
- In `Music` (`p1_aligned.png`, sorted by BPM): 3 Sense, Addis, Aktysia, Alba, American Dream, Amulet show BPM + key; One for the Road, Open Up Your Eyes, One Last Call, Pieces show neither: those come only from tags / earlier analysis. The analysis has to run on the songs in the list (one or two at a time, low priority, so playback does not drop: round 19 part 1 had xruns flat, late 0), and its results go into the library so they stay for next time.
- Sorting by BPM / KEY puts songs without values last; with the analysis in advance it becomes complete.
