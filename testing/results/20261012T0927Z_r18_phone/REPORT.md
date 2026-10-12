# Round 18 on the phone (Nothing Phone (2a), A142, Android 16)

Build: Android run 38227433624, commit 302b570. adb over 127.0.0.1:5555 (phonelink). Rules: testing/PROTOCOL.md.

| Part | Check | Expected | Result | |
|---|---|---|---|---|
| 0 | smoke | installs, starts, `Quick: interface ready`, not black, no QML error | `Interface: Qt Quick (GPU)`, `Quick: interface ready 413x919`, no qml lines; screen drawn: 4 tabs, CPU/GPU meter top right (00_start.png) | PASS |
| 1 | load | Songs → Music → tap 2 songs: deck 1, deck 2 | `Quick: load [Channel1] …/3 Sense.mp3`, `Quick: load [Channel2] …/Addis.mp3` | PASS |
| 1 | PLAY / pause | at the press, log `button …play 1/0`, sound | `Quick: button [Channel1],play 1` → `play 0`, same deck 2; `Output level: peak 0.318` / `0.354` while playing | PASS |
| 1 | CUE | hold = preview, release = back | deck 1 paused, CUE held 6 s: `Output level: peak 0.319`; after release audio_flinger `Standby: yes` | PASS |
| 1 | SYNC | deck 2 tempo matches deck 1 | `Quick: button [Channel2],sync_enabled 1`; the BPM itself is not in the log, not measured | PASS (press only) |
| 1 | Marko's own PLAY presses | every press in the log | not done: Marko is out (4G); ask when he is back | — |
| 2 | crossfader | L: deck 1 only, R: deck 2 only, M: both | deck 1 playing: left `peak 0.292`, right `peak 0`, middle `peak 0.336` | PASS |
| 2 | volume faders | down = silent | deck 1 down `peak 0`, up `0.343`; deck 2 down `peak 0`, up `0.347` | PASS |
| 2 | GAIN | changes the sound, double tap resets | drag down `0.343 → 0.028`; double tap (two taps 120 ms apart) `→ 0.296`. Two taps from separate adb calls (~400 ms) do not count as a double tap: expected | PASS |
| 2 | HIGH / MID / LOW / FILTER | sound changes, LED ring follows, double tap resets | rings follow each knob (02_mixer_knobs_set.png: HIGH, MID, LOW cut = fewer dots, FILTER right half); FILTER `0.317 → 0.126`; all four reset by double tap `→ 0.289`. A LOW cut hardly moves the peak (`0.30 → 0.32`, deck 2 `0.35 → 0.30`): peak level is not a bass measure, not a fault | PASS |
| 2 | VU | moves | the meter between the faders shows the level (screenshot) | PASS |
| 3 | matrix time left | counts DOWN | `-2:25` → `-0:54` (deck 1, 02_mixer_knobs_set.png), `-2:24` → `-1:29` later; deck 2 `-5:39` → `-4:41` | PASS |
| 3 | 8 / 16 / 32 | cells change | `Quick: phrase beats 16`, `32`, `8`, `16`, `32`; matrix and Phrasing tab follow (03_phrasing.png 16, 03_phr_32.png 32) | PASS |
| 3 | Phrasing tab LEDs | 8 per row, numbers, current beat lit, beat 1 yellow | 16: two rows of 8 per deck, numbered, current beat in the deck colour, beats 1 and 9 dim yellow (03_phrasing.png); 32: four rows | PASS |
| 3 | beat launch (big LED) | stopped deck 2: press LED 1, it blinks, starts on deck 1's beat | LED 1 yellow while armed, `Quick: beat launch [Channel2] on beat 1`; afterwards both decks on cell 11 (03_phr_launched.png) | PASS |
| 3 | jump (big LED) | playing deck: jumps to that beat in time | deck 1 LED 9 pressed: before both on 11, after deck 1 on 8 / deck 2 on 12 (03_phr_after_jump.png): it moved. No log line for a jump: suggest `Quick: jump [ChannelN] to beat M` | PASS |
| 3 | easy to hit at 32 | Marko: "big so I can jump easily" at 32 | 32: cells ~115 × 50 px = **~44 × 19 dp**, below a 48 dp finger target; the scrolling waveform above takes ~480 px (03_phr_32.png). Suggest: at 32 give the LEDs the waveform's room (or two decks × 4 rows at ≥ 40 dp) | FAIL |
| 4 | full screen | no bars after start and after home → back | `dumpsys window`: `navigationBars visible=false`, `statusBars visible=false`, both times | PASS |
| 4 | tap on empty space | the navigation bar only | tap on deck 1's title: `Quick: show the navigation bar`, `WindowBridge: showing the navigation bar`; `navigationBars visible=true`, `statusBars visible=false` | PASS |
| 4 | the bar hides again | 2nd tap on empty space hides; any press hides; unused ~4 s hides | **never hides**: a 2nd tap logs `Quick: show the navigation bar` again (no hide line ever); 6 s and 16 s later still `navigationBars visible=true`; a press on the menu button works (`Quick: menu`) but the bar stays | FAIL |
| 4 | room for the bar | nothing under the bar (system-bars.md) | landscape (04_landscape.png): the 3 buttons sit over the app's right edge: + / − zoom, the Keys tab label, the CPU meter, the last LED column under them. The app does not make room because it never sees the bar as visible | FAIL |
| 4 | tabs | Songs / Phrasing / Mixer / Keys | four tabs, Phrasing second, all switch (00_start.png, 02_*, 03_*) | PASS |
| 4 | landscape | layout | decks left, Phrasing right (04_landscape.png). Phrasing LEDs in landscape: 4 rows × 8 per deck about 20 px tall, numbers unreadable. The scrolling waveform shows diagonal lines across its top right (zig-zag, right after the rotation) | FAIL |
| 4 | swipe up from the bottom | bars come, next touch hides | not tested: the bar was already stuck on screen | — |

Tester's guess at the cause (for the builder, not verified in code): the app's "bar visible" state never turns true. If
WindowBridge still decides with `insets.isVisible(WindowInsets.Type.systemBars())`, that is true only when status AND
navigation bars are both visible; showing the navigation bar alone leaves it false. `isVisible(navigationBars()) ||
isVisible(statusBars())` would see it, and with it the 4 s hide, the touch-to-hide and the room-making would run.
| 5 | folders | Music and Download | `Folders`: Music, Download; Music opens with ← (00_start.png, 05_cover_pick.png) | PASS |
| 5 | tap a song | loads onto the free deck | part 1: `Quick: load [Channel1] …3 Sense`, then `[Channel2] …Addis` | PASS |
| 5 | cover tap, then a song | that deck | deck 2's cover: Songs tab, `tap a song: deck 2` in yellow (05_cover_pick.png); Aktysia → `Quick: load [Channel2] /storage/emulated/0/Music/Aktysia.mp3`. The tap map's `deck2` is the whole header: a tap in its middle hits the text (= empty space → navigation bar), the cover is at the outer edge | PASS |
