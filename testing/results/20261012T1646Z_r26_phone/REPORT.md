# Round 26 on the phone (Nothing Phone (2a), A142, Android 16) - landscape (ROTATION_270, as Marko left it)

Build: run 38277822232, commit a5fd87c. Started by the tester's build watcher, 12.10.2026 18:47 local.
The phone was awake but the notification shade was pulled down over the app (focus `NotificationShade`): closed with `cmd statusbar collapse`. Kept awake with `svc power stayon true` for the round.

## Part 0: smoke - PASS
- `interface ready 919x413`, no qml error, `Sound output open ... (clock)`, `frame rate 120.00001 Hz -> 60.0 Hz`.

## Part 1: transport - PASS
- 3 Sense / Addis (NEXT). `play 1`, `auto cue ... [Channel2]`; deck 2 PLAY with no beat pressed: `align [Channel2] to beat 7 with the other deck`, NEW `align [Channel2] at beat 7`, `start ... from the cued beat 7 ...`, `beat launch ... on beat 7`. 30 s: xruns 0, late 0, slowest 3.7-4.1 ms.

## Part 2: sliders and knobs - PASS
- Crossfader L and back; HIGH deck 1 down and double tap back; Mixer VU meters differ in frames 0.3 s apart (live at 20 updates a second).
