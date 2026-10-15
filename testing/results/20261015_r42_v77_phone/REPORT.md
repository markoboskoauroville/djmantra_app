# Round 42 on V77 (794c399), Pixel 7, 15.10.2026 morning (wireless adb 192.168.1.103:5555; Mix Ultra on Bluetooth)

Tester: Mac session claude-code-32. APK: android-latest `DJMantra-V77-794c399-phone-arm64.apk`, installed over V71
(`adb install -r`, settings kept), versionName V77.

| part | check | result | notes |
|---|---|---|---|
| basics | starts, `Quick: interface ready 411x914`, both decks load and PLAY | PASS | Tap a song = first free deck (`free deck [Channel1]`, then `[Channel2]`). PLAY 2 with SYNC: `play armed (aligned start)`, started on the beat. |
| 68 | Bluetooth connect does not open the browser | PASS | V71 the same morning: `in 90 00 7F` → `controller: browse press` → opened Music/djmantra_app/Keys (00). V77: `Connected (bluetooth)`, toast, `controller link: bluetooth`, no browse press; the list stayed at the djmantra_app root (01). BT pill on. |
| 65 | compact list | PASS (look) | One line per row, whole folder names, no "Folder" text, TITLE / ARTIST / LENGTH / BPM / KEY columns, small covers (02). |
| 73 | waveform as sharp as the icon | PASS, two notes | See below. |

## 73, the waveform

Songs: deck 1 "Aum Ganesha Ganesha short english" (10A), deck 2 "cura prox cura, cura, cura" (7B), both playing.

- **Scrolling waveform (Phrasing tab), 04–06:** separate solid bars with black gaps, no haze, no translucent fringe,
  no soft edges, mirrored round the middle. Kicks red, the vocal hits green / cyan, some blue / violet and magenta bars.
  Default zoom (04), wide (06, ~10 bars on screen): solid bars. Deep zoom (05): the thin real-wave line, as before.
- **Shimmer:** a 4 s screen recording at the widest zoom (60 fps, kept on the Mac, not pushed to save SIM data):
  four consecutive frames stacked in 07. Every bar keeps its width, height and colour from frame to frame; nothing
  flickers. Whether it reads as smooth to the eye is for Marko to judge.
- **Overview lanes (02):** solid 2 px bars with gaps, pure colours, the white beat/cue marks over them.
- **Note 1, in-between colours:** deck 2 (and some of deck 1) has many ORANGE, AMBER and YELLOW-GREEN bars (06, 07),
  not only the pure red / green / blue / magenta of the spec. Probably the mix of bass + mids blended. If the icon
  look means four pure colours only, this needs quantising.
- **Note 2, height:** the bars use only about a quarter to a third of each lane's height (04, 06); deck 2 is quieter
  still. djay fills about two thirds. Maybe scale up.

## Next (needs the Mix Ultra in Marko's hands)

72 (ghosts), 69 (FX pads), 71 (SHIFT + FILTER depth), 70 (SHIFT + pad 5), then 63, 64, 66, 67, 56, the controller
halves of 51/53, 45–49.
