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
