# Round 24 on the phone (Nothing Phone (2a), A142, Android 16)

Build: run 38269733985, commit f6eae00 (9a43d0b + ROUND24.md). Started by the tester's build watcher, 12.10.2026.
The phone had gone to sleep (dozing, no lock): woken with KEYCODE_WAKEUP; kept awake with `svc power stayon true` for the round (switched off at the end). The phone kept turning (0 / 180 / 270, it was being carried), so rotation was locked to portrait for the round with `cmd window user-rotation lock 0` (freed at the end).

## Part 0: smoke - PASS
- No qml error, `Sound output open: ... (clock)`, `interface ready 413x919`.
- NEW: `WindowBridge: frame rate 90.0 Hz -> 60.0 Hz`, then `frame rate 120.00001 Hz -> 60.0 Hz` (twice, after display mode changes). X = 90, then 120. PASS

## Part 1: transport - PASS
- 3 Sense (deck 2, tap) and Addis (deck 1, NEXT). PLAY: `play 1`; deck 2: `start ... from the cued beat 8 ...`, `beat launch ... on beat 8`. 30 s and later: xruns 0, late 0, slowest 2.7-3.9 ms.

## Part 2: sliders and knobs - PASS
- Crossfader L and back; HIGH deck 1 dragged down: counter-clockwise, ring dims; double tap: back.
- **Map note (FAIL-worthy for the tester's tools, not for Marko):** after the restart in portrait, the logged map put the crossfader at y 2024 and the tabs at 2145, but on screen they were at 2124 / 2244 (100 px lower); a later map (after the Mixer tab was shown) had it right. A swipe on the stale crossfader position hit a song row ("Deck 1 is playing. Load the new song anyway?", cancelled). The map seems logged before the final layout (before the insets settle); please log it again once the layout is final.
- The list's sort is remembered (TITLE ▲): the root shows Download before Music now.

## Part 3: CUE return - FAIL (the fix works; a new problem found)
- 32 BEATS, deck 2 stopped, LED 17: `cue set [Channel2] at phrase beat 17` (45.1). PLAY: aligned start, `beat launch ... on beat 17`; played 17 s; pause; CUE: `button [Channel2],cue: back to the cue point` → **45.1, stopped** (`p3_back17b.png`). PASS
  (The first CUE tap after the pause logged nothing and the screen looked shifted, `p3_back17.png`; the second tap worked. Possibly one tap lost to the bars / insets; not repeatable later.)
- On the cue, CUE held 1.5 s: `cue_default pressed`, previews, back on release.
- **FAIL: during that preview AUTO PHASE moved the deck ON AIR:** `auto phase: [Channel1] (beat 12) to the other deck's beat`, `align [Channel1] to beat 19`, `jump [Channel1] to beat 19 on the next beat`. Deck 1 was playing to the floor; deck 2 was only being previewed in the headphones for 1.5 s. The audience hears deck 1 jump. AUTO PHASE must ignore a deck that only plays because CUE is held (and, generally, never move the deck that is on air while the other is in the headphones).
- LED 3: `cue set ... at phrase beat 3` (41.3); play 13 s, pause, CUE: `cue: back to the cue point` → **41.3** (`p3_cued3.png`, `p3_back3.png`). PASS
- Playing deck + CUE (a CDJ returns to the cue and stops): first try, PLAY then CUE 8 s later: `cue_default pressed`, the deck stopped at 48.2, not at the cue 41.3 (`p3_playing_cue.png`). Second try (from 48.2, played 6 s to 51.1, CUE): back to 48.4 and stopped (`p3_playing_cue2.png`). So on a PLAYING deck CUE seems to set a new cue where it was pressed the first time (or the aligned-start wait was still running): please log the cue position on every CUE press to make this checkable.

## Part 4: CPU - FAIL (evened out, not lower)
Both decks playing (Addis + Aktysia, `beat launch ... on beat 7`; output peak 0.63-0.70 at every tab, xruns 0), 8 s per tab then `top` over 3 s:

| tab | round 24 | round 23 |
|---|---|---|
| Songs | 62.3 % | 51.6 % |
| Phrasing | 64.3 % | 76.3 % |
| Mixer | 64.6 % | 74.6 % |
| Keys | 66.3 % | 49.0 % |

Threads (`top -H`): Phrasing: qtMainLoopThread 24.3 %, QtThread (render) 20.6 %, AudioTrack 7.0 %, mali-event-handler 4.0 %, binder 2.0 %, CachingReaderWorker 1.6 + 1.3 %, ged-swd 1.3 %. Mixer: qtMainLoopThread 24.6 %, QtThread 21.3 %, AudioTrack 7.3 %, mali 4.0 %.
- The `Controller` thread is GONE from the top 8 (was 8 % on every tab). PASS for that fix.
- The render thread is unchanged on Phrasing (20.6 % → 20.6 %) and now ~21 % on Mixer: the 60 Hz cap did not lower it (the panel was at 90 / 120 Hz before; `frame rate ... -> 60.0 Hz` logged). Songs and Keys went UP by ~10-17 points: something now runs on every tab (the main loop at ~24 % everywhere).
- A first measurement with both decks silently stopped (peak 0) gave Keys 10 %: so ~55 points of the cost come from playing decks (waveforms, meters, matrix animating), not from the tab itself.
- Smoothness: waveforms scroll, no visible stutter in screenshots (not judged by eye on the phone).
- Tester slip: one CUE / PLAY pair landed on SYNC / CUE of deck 1 (`sync_enabled 1` logged from a CUE tap); deck 1's SYNC is now on. Positions in the map matched the screen afterwards, so maybe a moment with the bars shown.
