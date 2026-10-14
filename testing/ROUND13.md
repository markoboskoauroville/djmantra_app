# Round 13: the new interface (Qt Quick, drawn by the GPU), 12.10.2026

Build: the green Android run of the commit "New interface in Qt Quick ..." or later on `ccr-eb7d1019-d3kubd`.
Output: `testing/results/<UTC stamp>_r13_phone/REPORT.md` (table: what | expected | what happened | pass/fail),
screenshots, `logcat-quick.txt`; push after each step; message the cloud session "DJ mantra app testing".
Watcher: `tools/android/watch_host.sh ~/djmantra-watch/r13 $SERIAL` during the round.

0. Classic interface first (default): quickly check fix build 2 is fine (bars hidden, logo opens the menu,
   Settings opens, waveforms full width, counter readable). Settings now has "New interface (GPU)".
1. Start the new interface: either Settings → "New interface (GPU)" on, then close and open the app, or
   `adb shell am start -n com.djmantra.app/org.qtproject.qt.android.bindings.QtActivity --es applicationArguments "'--qml --play /sdcard/Music/Addis.mp3 /sdcard/Music/Dune.mp3'"`.
   Log: `adb logcat -d | grep -E "Interface:|Quick:|qml|QML|Deck . playing|FATAL" > $OUT/logcat-quick.txt`.
   Expect "Interface: Qt Quick (GPU)", "Quick: interface ready", "Quick: map <name> x y w h" lines (device
   pixels: tap at x+w/2, y+h/2). Any "module ... is not installed" / "Failed to load QML file" = report at once.
2. Screenshots portrait: mixer tab, waveforms tab, pads tab, menu (logo), library (LIBRARY in the menu or a
   tap on a deck cover), Camelot wheel (tap the badge). Check: nothing under the camera hole, no text cut off,
   no bars over the app.
3. Play two songs: waveforms scroll smoothly (both lanes), VU meters move, counter TIME and a tap → BARS,
   key and badge colour, overview. Taps answered at once? Every button: SYNC, CUE (hold), PLAY, crossfader,
   volume faders, knobs (drag up/down, double tap = middle), PFL, hot cue pads, zoom −/+.
4. CPU, both decks playing, waveforms tab: `top -p PID` 3 samples; and `adb shell dumpsys gfxinfo com.djmantra.app`
   (frame times). Compare with the classic interface numbers (126/117/123 %CPU).
5. Library: open Music, load a song with LOAD 1, another with LOAD 2; back button and Android back.
6. Landscape: one screenshot of each tab (portrait-first layout: report what breaks).
7. Menu → "BACK TO THE CLASSIC SCREEN", close and open the app: the classic interface again.
