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
| 1a | New interface starts (`--qml --play A B`) | "Interface: Qt Quick (GPU)", "Quick: interface ready", map lines | `04:05:57.866 Interface: Qt Quick (GPU)`, `04:05:59.256 Quick: interface ready 413x919`, both decks playing, 15 `Quick: map` lines (logo, keyBadge, deck1/2, tab0–2, page, sync1/2, cue1/2, play1/2, crossfader). No QML/module error, crash buffer empty. `logcat-quick.txt` | PASS |
| 1b | Bars hidden | no status/navigation bar | Hidden in the new interface (`q1-start-hole.png`) | PASS |
| 1c | Camera hole | nothing under the hole | Logo left (x 26–121), badge right (x 748–1058), the hole (x ~511–571) over empty black | PASS |
| 1d | Deck counter | moves while playing | **Stays `0:00/0:00/-0:00` on both decks** while both play and the VU meters move (`q1-start.png`, `q1-start-b.png` 4 s later) | **FAIL** |
| 1e | Play buttons | ▶ / ⏸ icon | **Two empty boxes (tofu) on both play buttons**: the glyph is missing from the font on Android | **FAIL** |
| 1f | Overviews | coloured waveform | Grey, flat, almost empty strips under the titles | FAIL? (look in step 3) |
| 2a | Mixer tab | clean, nothing cut, nothing under the hole | Knobs with labels under them, two VU meters moving, faders with deck colours (`q1-start-hole.png`). Hole clear. Counter stuck (1d), play icons tofu (1e) | PASS except 1d/1e |
| 2b | Waveforms tab | both lanes, full width | Both lanes scroll, beat lines, **but the waveform is only ~5 % of the lane width** (narrower than the classic one): `q2-waveforms-hole.png`. Same scaling problem as 0d | **FAIL** |
| 2c | Pads tab | 2×8 hot cue pads | DECK 1 / DECK 2 HOT CUES, pads 1–8, nothing cut (`q2-pads-hole.png`) | PASS |
| 2d | Menu (logo) | menu | LIBRARY, CAMELOT WHEEL, BACK TO THE CLASSIC SCREEN (`q2-menu-hole.png`). **Header says "DJ Mantra 2.5.6"**: that looks like Mixxx's version, the app is 0.5.0. The LIBRARY button starts at y≈96, 5 px under the hole: tight but clear. No Settings in this menu | PASS (version text wrong) |
| 2e | Library | folders | "Folders" with ← back: Music, Download, **Music again** (listed twice) (`q2-library-hole.png`) | FAIL (duplicate) |
| 2f | Camelot wheel (badge tap) | wheel | Wheel with 8A outlined green, 5B pink, three explanation lines in green/orange/red (`q2-camelot-hole.png`). **The title "Camelot wheel" runs under the camera hole** (the hole sits on "…eel") | **FAIL (hole)** |
