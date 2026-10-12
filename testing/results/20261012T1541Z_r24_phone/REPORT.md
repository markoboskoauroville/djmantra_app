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
