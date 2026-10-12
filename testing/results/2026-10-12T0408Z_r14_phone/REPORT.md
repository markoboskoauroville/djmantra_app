# Round 14 on the phone (Nothing A142, Android 16), 12.10.2026

Build: Android run 38195388871 of 43e3538, sha256 `164bbbf07f27bd339d644fa334d24711053e2bac9e5a69f5725573c5704b4622`,
installed 06:08 CEST. Watcher `~/djmantra-watch/r14`. `-hole` copies: camera hole red, 126 px line yellow.

## Marko, 06:09, while the round ran

> "please continue testing and take a screenshot, and you're going to see that text from application is under my wi-fi and clock"

Confirmed in A1b below: as soon as the bars come back (any swipe from the top edge), the clock and the notification
icons sit on the dj logo, and Wi-Fi, LTE, signal and battery sit on the Camelot badge "8A > 5B clash". **The bars then never hide again.**

| # | what | expected | what happened | pass/fail |
|---|---|---|---|---|
| A1a | Classic: bars hidden after the start | hidden | `06:09:00.639 WindowBridge: hiding the system bars` → `requestedVisibleTypes -528`. `dumpsys`: statusBars and navigationBars `visible=false` (`a1-classic-start-hole.png`) | PASS |
| A1b | Swipe from the top: bars show, the app makes room, they hide again after ~4 s | room made, hidden again | `06:09:23.007 requestedVisibleTypes to -525 (was -528)`: bars visible. **The app does not make room**: the clock sits on the logo, Wi-Fi/LTE/battery on the badge (`a1-swipe-0s-hole.png`). **They never hide again**: still `visible=true` at 5 s and 15 s (`a1-swipe-5s`, `a1-swipe-15s`). **No "WindowBridge: bars … insets …" line** was logged after the swipe: the bars watch did not run | **FAIL** (Marko's report) |
| A1c | Logo while the bars show | menu | No menu, no log line (`a2-logo-bars-shown-hole.png`) | FAIL (A1b) |
| A2a | Logo → menu (bars hidden, after a restart) | menu | Menu opens (LIBRARY, CONTROLLER, ONE DECK, REC, Settings). **The ✕ close button sits right under the camera hole** (`a2-menu-hole.png`). No "Menu" log line for opening it | PASS / **hole FAIL** |
| A2b | Menu → Settings | opens, logged | `06:10:44.591 Menu: Settings tapped`, `06:10:44.608 Settings screen open QRect(0,0 413x919) visible true`. Settings shows Main Volume, Split Output, New interface (GPU), Draw on the GPU (test), Sound, Library, Camelot wheel (`a2-settings-hole.png`). Title and ← clear of the hole | PASS |
| A3a | Counter readable | readable size | Not cut (`0:43/5:56/-5:13`), white, but **still ~8 px high** on a 1084 px screen | FAIL (size) |
| A3b | Waveform width | (report) | **~30 % of the lane** on both decks (round 13: ~10 %) (`a3-waveforms-hole.png`) | better, not full |
