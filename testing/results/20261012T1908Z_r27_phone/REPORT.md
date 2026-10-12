# Round 27 on the phone (Nothing Phone (2a), A142, Android 16) - V1

Build: run 38291801689, commit a972af4. Started on Marko's "go" (12.10.2026 ~21:08 local). Portrait (ROTATION_0, as the phone lies).

## Part 0: smoke - PASS
- `adb install` of djmantra-arm64-v8a.apk: versionName=V1, versionCode=6. (My first try picked the x86_64 APK from the run's artifacts: not installed, the old 0.5.0 stayed and started `Interface: classic (widgets)` from its old setting; V1 then installed fine.)
- `Interface: Qt Quick (GPU)`, `interface ready 413x919`, no qml error, `Sound output open ... (clock)`, `frame rate 90.0 Hz -> 60.0 Hz` (then 120 -> 60). The clock shows 21:09 at start (`p0_start.png`).

## Part 1: transport - PASS
- 3 Sense (tap) / Addis (NEXT). `play 1`, `auto cue ... [Channel2]`, `align [Channel2] at beat 6`, `start ... from the cued beat 6 ...`, `beat launch ... on beat 6`. 30 s both playing: xruns 0, late 0, slowest 2.3-4.2 ms.
