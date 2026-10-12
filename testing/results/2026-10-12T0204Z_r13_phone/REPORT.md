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
| 3a | Counter tap TIME → BARS | switches, bar.beat moves | Switches to `83.1  -95.1  4/4` and counts (90.1 → 119.4 over the round). **TIME mode never moves** (1d). Deck 2 stayed TIME `0:00/0:00/-0:00` all the time | PASS (BARS) / FAIL (TIME) |
| 3b | SYNC deck 2 | BPM 116 → 120, button lit | 120.0, SYNC lit pink. The key badge changed 5B → 12B (no keylock, the pitch moves: right) (`q3-sync2-xfader-left-hole.png`) | PASS |
| 3c | Crossfader left → right | moves | Handle follows the drag both ways | PASS |
| 3d | Volume fader deck 1 drag down | moves | Handle went down to the middle | PASS |
| 3e | Knob drag up / double tap | turns / back to the middle | Turned clockwise, double tap → back to 12 o'clock (`q3-controls`, `q3-knob-dbl`). **The knob labels sit halfway between two knobs** (GAIN between knob 1 and 2 … FILTER above the headphone button), so it is not clear which knob a label names | PASS / labels unclear |
| 3f | PFL (headphones) deck 1 | lights | Lit blue | PASS |
| 3g | CUE deck 2 (held 1.5 s, released) | cue play, back to cue and stop | Deck 2 stopped at the cue, orange CUE marker on the waveform (`q3-cue2-after`, `q3-zoom-before`) | PASS |
| 3h | PLAY deck 1 | pause / play | Pauses and plays. **▶ draws fine; the pause glyph is the tofu** (two boxes) | PASS / icon FAIL |
| 3i | Hot cue pad 1 deck 1 | set, pad lit | Pad turned green (`q3-pad1-hole.png`) | PASS |
| 3j | Waveform zoom + (deck 1, twice) | beat lines further apart | **No change**: beat lines ~87 px apart before and after (`q3-zoom-before`, `q3-zoom-plus`) | **FAIL** |
| 3k | Taps answered at once | yes | Every tap and drag answered within the 1–2 s screenshot delay; nothing ignored | PASS |
| 4 | CPU, both decks, waveforms tab | lower than classic (126/117/123 %) | **37.0 / 32.5 / 55.5 %CPU** (`cpu-quick-waveforms.txt`). `dumpsys gfxinfo` sees 0 frames: Qt Quick draws on its own surface that HWUI does not count, so no frame times from there (`gfxinfo-quick.txt`) | PASS (about a third of the classic CPU) |
| 5a | Library → Music | songs with LOAD 1 / LOAD 2 | Folders ganesha, Motiv Recordings, then the songs, each with LOAD 1 and LOAD 2. Long titles end in "…" inside their box (`q5-music-hole.png`) | PASS |
| 5b | LOAD 1 (Alba) | loads, back to the decks | `Quick: load [Channel1] /sdcard/Music/Alba.mp3`. Back to the deck screen by itself. Analysed within ~8 s: 122.0 BPM, key 5A, badge "12B > 5A clash" (`q5-load1`, `q5-load1-analysed`) | PASS |
| 5c | LOAD 2 (Amulet) while deck 2 **plays** | (a live deck should be protected) | `Quick: load [Channel2] /sdcard/Music/Amulet.mp3`: **it replaced Dune while Dune was playing, without a question**. In a set that stops the music on air (`q5-load2-hole.png`). Suggest: refuse or ask when the deck plays | **FAIL (risk)** |
| 5d | ← in the library | Music → Folders → decks | Music → Folders → deck screen (`q5-arrow-back`, `q5-arrow-back2`) | PASS |
| 5e | Android back in the library | back to the decks | Folders → deck screen (`q5-android-back`) | PASS |
| 5f | Library remembers the folder | (nice to have) | Opens at Folders every time, not in Music | note |
| 6a | Landscape: camera hole | app keeps out of the hole's side | Rotation 1: `DisplayCutout insets=Rect(126, 0 - 0, 0)`, hole on the left (bounding rect x 0–126, y 544–630). **The app starts right of x=126: hole clear** (`q6-land-*-hole.png`, yellow line = inset) | PASS |
| 6b | Landscape: header | readable | Logo, badge, both decks with cover/title/counter, overviews, tabs: fine | PASS |
| 6c | Landscape: waveforms tab | two lanes | **The page area is ~40 px high**: the lanes are a thin line with the zoom buttons under it (`q6-land-waveforms-hole.png`) | **FAIL** |
| 6d | Landscape: mixer tab | knobs, faders, VU | **Squashed into ~40 px**: GAIN/HIGH/MID/LOW/FILTER labels drawn on top of each other, no knobs visible, faders and VU a few px high (`q6-land-mixer-hole.png`) | **FAIL** |
| 6e | Landscape: pads tab | 2×8 pads | **Pad numbers overlap in a strip**, pads not drawn (`q6-land-pads-hole.png`) | **FAIL** |
| 6f | (restore) | — | `user_rotation` back to 0, auto-rotate left off as it was | — |
| 7 | Menu → BACK TO THE CLASSIC SCREEN, close and open | classic interface | Button turns into "Classic screen at the next start: close and open the app", log `Quick: interface at the next start: classic`. After force-stop + start: `Interface: classic (widgets)` (`q7-after-back-tap`, `q7-reopened`). The classic screen has the bars over it again (0a) | PASS |

## Summary for the next build

Classic interface: **the bars still cover it**: WindowBridge's "hiding the system bars" sets `requestedVisibleTypes -9` (only the IME hidden), right after Qt had hidden them (`-528`). That blocks the logo menu and Settings. Waveforms ~10 % wide. Counter not cut, but tiny.

New interface (Qt Quick): **loads on the first try, bars hidden, hole clear in portrait and landscape, every control answers at once, CPU about a third of the classic one (32–55 % against 117–126 %).** To fix:
1. TIME counter stays `0:00/0:00/-0:00` on both decks (BARS works).
2. The pause glyph is tofu (▶ is fine): use an icon or a font that has ⏸.
3. Waveforms ~5 % of the lane width (same scaling problem as the classic one).
4. Zoom −/+ does nothing.
5. The Camelot wheel title runs under the camera hole.
6. Library lists "Music" twice; LOAD on a playing deck replaces it without asking.
7. Menu header says "DJ Mantra 2.5.6" (Mixxx's version?), the app is 0.5.0.
8. Knob labels halfway between two knobs.
9. Landscape: the page area under the tabs collapses to ~40 px (waveforms, mixer, pads).
