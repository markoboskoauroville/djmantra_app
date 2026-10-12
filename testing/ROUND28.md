# Round 28 (V2; rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 28 finished")

Build: V2 (releases/tag/android-latest, DJMantra-V2-ebd0270-...). Marko: "test back and forth until the app is
stable". Grep once per part: `adb logcat -d | grep -E "Quick:|Sections|WindowBridge|GenreLookup|Sound:"`.

## 0. Smoke: V2 in the menu, `Interface: Qt Quick (GPU)`, no `qml` error, sound output, frame rate -> 60 Hz.

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Scrub (round 27 part 3 fixes): HOLD an overview 0.3 s: the zoomed waveform (yellow frame, ~4 s) is VISIBLE
just above it (screenshot, portrait and landscape); the drag moves the position finely; release closes it. During
the scrub (both decks playing, phrases misaligned): NO `auto phase:` line until 1 s after release.

## 3. Counter reset (new): the matrix's middle number (bar.beat) reads 1.1 on the first beat of every section and
counts up from there; before the first section it counts from the song's "1". Watch 3 section changes on 2 songs
(Phrasing tab or the overview's yellow next-change line): screenshot / logcat the number right after each change.

## 4. Section bench v2 (new, offline, `testing/sections/sections.py` updated): green dashed = v2, the app's next
detector built from the app's own data only (low / mid / high + beat grid): downbeat vote, kick track with the fill
rule, phrase "1" vote, bar self-similarity novelty, 8-bar grid. Run it on your 8 songs AND every Drive song you
have downloaded; push the pictures + JSON to testing/sections/ (bench2/ subfolder) and message the TOTAL line
(app vs v2 against the reference's major sections) and the 5 worst songs for v2.

## 5-9: round 27 parts 5-9 (CPU per tab with `top -H`, genre match, clock, Mix Ultra when Marko plugs it in, version
link: tap V2 opens releases/tag/android-latest).
