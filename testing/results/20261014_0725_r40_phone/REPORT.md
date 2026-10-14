# Round 40, V41 (0fa0d39) on the Pixel 7 (41241FDH2S10SJ, Android 16), 14.10.2026

Tester: Claude Code on the Mac (hub session), adb over USB. APK DJMantra-V41-0fa0d39-phone-arm64.apk, versionName V41.

| part | check | result | notes |
|---|---|---|---|
| 0 | install, start | PASS | `Success`; app up in ~1 s |
| 0 | `Quick: interface ready` | PASS | `Quick: interface ready 411x914` |
| 0 | DJM filter loaded | PASS | `DJM filter: loaded on [Channel1]`, `... [Channel2]` |
| 0 | not black, no QML error | PASS | Songs tab drawn (Music, Download folders); no QML warning/error in logcat |
