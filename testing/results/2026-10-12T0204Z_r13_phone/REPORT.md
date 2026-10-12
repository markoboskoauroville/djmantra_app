# Round 13 on the phone (Nothing A142, Android 16), 12.10.2026

Build: Android run 38193283916 of d2dbddb (contains c8e59f6 = fix build 2), `djmantra-arm64-v8a.apk`,
sha256 `f4b604ff1e3aafba9514a896001201ea87cca14311cea36bac84c4844ec524c3`, `adb install -r` at 04:03 CEST.
Watcher: `~/djmantra-watch/r13`. Every screenshot has a `-hole` copy: camera hole red, 126 px cutout line yellow.
Media volume 0 (night).

| # | what | expected | what happened | pass/fail |
|---|---|---|---|---|
| 0a | Classic: bars hidden | bars gone ~1 s after start | **Still visible.** `dumpsys window`: statusBars and navigationBars `visible=true`. The log says why: `04:04:25.516 InsetsController: Setting requestedVisibleTypes to -528` (bars hidden: Qt full screen), then `04:04:27.050 WindowBridge: hiding the system bars` → `04:04:27.051 Setting requestedVisibleTypes to -9 (was -528)`. -9 = only the IME hidden, so **WindowBridge's "hide" makes the bars visible again.** No "bars shown/hidden, insets" line and no "insets update failed" line ever came. `logcat-step0-bars.txt`, `s0-mixer-hole.png` | **FAIL** |
| 0b | Classic: logo opens the menu | menu, log "Menu" | Logo under the clock (x 92 y 67): no menu, no log line (`s0-logo-tap-hole.png`) | **FAIL** (bars) |
| 0c | Classic: Settings opens | "Menu: Settings tapped" | Cannot be reached (0b) | BLOCKED |
| 0d | Classic: waveforms full width | full width | **Narrower than before: ~10 % of the lane on both decks** (`s0-waveforms-hole.png`) | **FAIL** |
| 0e | Classic: counter readable | own line, white, readable, not cut | Own line under the title, white, not cut (`0:44/5:56/-5:12`). **Still tiny**: ~8 px high on a 1084 px screen. The key moved to the artist line (`Orkidz 8A 5B Maxim Sheremetev, Cafe De A…`), and that line runs deck 1's artist into deck 2's | not cut / too small |
| 0f | Camera hole | nothing under the hole | Under the status bar anyway (0a); the badge sits right of the hole, the logo left | blocked by 0a |
