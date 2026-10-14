# Round 40, V41 (0fa0d39) on the Pixel 7 (41241FDH2S10SJ, Android 16), 14.10.2026

Tester: Claude Code on the Mac (hub session), adb over USB. APK DJMantra-V41-0fa0d39-phone-arm64.apk, versionName V41.

| part | check | result | notes |
|---|---|---|---|
| 0 | install, start | PASS | `Success`; app up in ~1 s |
| 0 | `Quick: interface ready` | PASS | `Quick: interface ready 411x914` |
| 0 | DJM filter loaded | PASS | `DJM filter: loaded on [Channel1]`, `... [Channel2]` |
| 0 | not black, no QML error | PASS | Songs tab drawn (Music, Download folders); no QML warning/error in logcat |
| 0 | load from Songs | PASS | tap -> `Quick: load [Channel1] ...Aum Ganesha...`, then `[Channel2] ...cura prox...` (Music folder, 8 songs 36:02, 100 %) |
| 0 | PLAY / pause each deck | PASS | `button [Channel1],play 1`; deck 2 `play armed (aligned start)` -> `beat launch [Channel2] on beat 8`; `play 0` both |
| 0 | audio engine | PASS | `Output rate` 48171-48212 of 48000, xruns 0, late 0, slowest callback 11-14 ms |
| 0 | CUE, SYNC | PASS | `cue press [Channel1] ... back to the cue point`; `sync_enabled 1` / `0` |
| 0 | knobs move | PASS | GAIN drag up/down both decks: pointer + dots follow |
| 0 | **EQ knobs at start** | FAIL? | right after start HIGH / MID / LOW on BOTH decks were drawn fully right (pointer at max, all dots lit) — the old "EQ at maximum" look. A double tap brought each to the middle. Either the stored/initial EQ value is max, or the drawing is wrong; not checked by ear |
| 0 | double tap resets | PASS | GAIN, HIGH, MID, LOW -> middle; volume fader -> top; crossfader -> middle |
| 0 | volume fader cuts sound? | ? | deck 1 alone: fader near bottom `Output level: peak 0.036`, fader top `0.041` (quiet part of the song). As in r39: the logged peak barely follows the fader — to check by ear |
| 0 | LEDs matrix | PASS | time left, bar.beat (4.4, 5.1, 7.3), countdown and the running lights move while playing (screenshots) |
| 0 | **BPM / key on load** | NOTE | library row says 114.6 BPM 10A and 118.0 BPM 2B; after loading, `sync [Channel1]: deck 1 90.07 bpm, deck 2 90.07 bpm` and deck 2 shows key **1B** (header `10A > 1B clash`). The deck's tempo differs from the library's |
