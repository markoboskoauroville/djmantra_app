# Round 18 on the phone (Nothing Phone (2a), A142, Android 16)

Build: Android run 38227433624, commit 302b570. adb over 127.0.0.1:5555 (phonelink). Rules: testing/PROTOCOL.md.

| Part | Check | Expected | Result | |
|---|---|---|---|---|
| 0 | smoke | installs, starts, `Quick: interface ready`, not black, no QML error | `Interface: Qt Quick (GPU)`, `Quick: interface ready 413x919`, no qml lines; screen drawn: 4 tabs, CPU/GPU meter top right (00_start.png) | PASS |
