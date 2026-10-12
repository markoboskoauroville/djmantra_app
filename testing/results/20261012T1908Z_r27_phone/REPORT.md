# Round 27 on the phone (Nothing Phone (2a), A142, Android 16) - V1

Build: run 38291801689, commit a972af4. Started on Marko's "go" (12.10.2026 ~21:08 local). Portrait (ROTATION_0, as the phone lies).

## Part 0: smoke - PASS
- `adb install` of djmantra-arm64-v8a.apk: versionName=V1, versionCode=6. (My first try picked the x86_64 APK from the run's artifacts: not installed, the old 0.5.0 stayed and started `Interface: classic (widgets)` from its old setting; V1 then installed fine.)
- `Interface: Qt Quick (GPU)`, `interface ready 413x919`, no qml error, `Sound output open ... (clock)`, `frame rate 90.0 Hz -> 60.0 Hz` (then 120 -> 60). The clock shows 21:09 at start (`p0_start.png`).

## Part 1: transport - PASS
- 3 Sense (tap) / Addis (NEXT). `play 1`, `auto cue ... [Channel2]`, `align [Channel2] at beat 6`, `start ... from the cued beat 6 ...`, `beat launch ... on beat 6`. 30 s both playing: xruns 0, late 0, slowest 2.3-4.2 ms.

## Part 2: Mixer tab - PASS
- Both decks playing, Mixer, portrait (`p2_mixer.png`): the two VU meters between the faders rise and fall in 4 frames 0.25 s apart, green into yellow at the top; no black wedge or spike anywhere.
- Crossfader full left with deck 2 in the headphones (auto cue had put it there, Marko's case): 2 more frames, the same, no spike (`p2_mixer_xf_left.png`). Crossfader back to the middle.

## Part 3: overview scrubbing - FAIL
- TAP on deck 1's overview at 30 %: `jump [Channel1] to 48.94 s` at once, no zoom window (`p3_tap.png`). PASS
- HOLD still (motionevent DOWN, 0.6-1 s): `scrub [Channel1] zoomed from 52.79 s` / `scrub [Channel2] zoomed from 174.57 s` is logged, **but no zoomed waveform / yellow frame is visible anywhere on the screen** (`p3_hold.png`, `p3_hold_full.png`: full screenshot while the finger is down; the matrix, overviews and list look as without a hold).
- Drag while held: deck 1, 100 px right → `ended at 53.48 s` (+0.69 s, later, slow); deck 2, 150 px left → remaining time 3:00 → 3:02 (earlier). Release: the deck stays there. The fine movement works; only the window is missing.
- Drag at once (no hold): `scrub [Channel2] zoomed from 194.79 s`, 80 px right, `ended at 194.77 s`: it hardly moved (my MOVE events came fast, in one shell call; may be the input, not the app).
- **During every scrub AUTO PHASE jumped the OTHER deck**: scrubbing deck 2 → `jump [Channel1] to beat 7` twice and `beat 8` within ~2 s; the tap on deck 1 → `jump [Channel1] to beat 2`. With both on air that follows the "the deck you didn't touch" rule, but the audience hears the deck on the floor jump several times while the DJ only places the other deck. Suggest: AUTO PHASE waits until the scrub has ended, then moves the scrubbed deck.
- Landscape scrubbing: not done here (see part 4; rotation is Marko's).

## Part 4: landscape - PASS (the scrub window is missing here too, see part 3)
- Rotated for this part only (`user-rotation lock 1`, then `free`): the deck part fills the screen: headers with covers, the button row, the modes, the matrix and two tall overviews across the full width; no song list, crossfader or tabs (`p4_land.png`).
- Deck 1's cover: `browse open for [Channel1]`, the list full screen in front, ← top left, "tap a song: deck 1" (`p4_browse.png`). Tap Aktysia: `load [Channel1] Aktysia`, `browse closed` (`p4_loaded.png`).
- Open again: in the Music folder ← goes UP to Folders (not closed); at the top ← closes: `browse closed`, nothing loaded. Fine (one ← per folder level).
- Landscape hold on deck 2's overview: `scrub [Channel2] zoomed from 323.19 s` ... `ended at 323.66 s`, again no zoom window visible (`p4_land_hold.png`).
- Portrait afterwards unchanged: Songs / Phrasing / Mixer / Keys, crossfader (`p4_portrait.png`). Auto-rotate given back (`user-rotation free`).

## Stopped by Marko after part 4 (12.10.2026 ~21:18 local)
- Part 5 (CPU) had started: both decks playing (Aktysia + Alba, peak 0.62-0.74, xruns 0); Songs tab 9.0 %, then the app's CPU read 0 % on Phrasing / Mixer / Keys (the app was no longer running in front: someone was using the phone, the volume panel had opened). Void, not a result.
- Marko: "stop testing". Parts 5-9 NOT TESTED. Screen stay-awake switched off; rotation is free.
