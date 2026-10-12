# Round 14 (12.10.2026): round 13's fixes, both interfaces

Build: the green Android run of "Round 13 fixes ..." or later on `ccr-eb7d1019-d3kubd`.
Output: `testing/results/<UTC stamp>_r14_phone/REPORT.md` + screenshots (+ `-hole` copies), logcat; push after
each part; message "DJ mantra app testing". Watcher on as before.

A. Classic interface (default):
   1. The bars stay hidden after the start (Qt full screen is off now; WindowBridge alone hides them). Log lines
      "WindowBridge: hiding the system bars", "WindowBridge: bars ... insets ...". Swipe from the top: bars show,
      the app makes room, they hide again after ~4 s.
   2. Logo → menu → Settings opens ("Menu: Settings tapped", "Settings screen open").
   3. Counter readable (size), waveforms (width: report it).
B. New interface (`--qml`, or Settings → "New interface (GPU)"), two songs playing:
   1. TIME counter runs (elapsed/length/-remaining), BARS mode too.
   2. Play/pause icon: ▶ when stopped, two bars when playing (drawn, no tofu).
   3. Waveforms fill most of their lane (log "Quick: waveform gain [ChannelN] <gain> (loudest ...)").
   4. Zoom − / + change the waveform scale (beat lines farther apart / closer).
   5. Camelot wheel and library: title and back button below the camera hole.
   6. Library: Music listed once; LOAD on a playing deck asks "Deck N is playing. Load ... anyway?" (CANCEL, LOAD).
   7. Menu header "DJ Mantra 0.5.0".
   8. Knob labels right under their knobs.
   9. Landscape: SYNC / BPM / CUE / PLAY at each side, the tabs and the page in the middle, the crossfader under
      it; all three tabs usable (screenshots).
C. Loudness: with media volume at a normal level (or 0 if it must stay quiet) play one of Marko's songs alone,
   volume fader up, crossfader to its side, 20 s: `adb logcat -d | grep "Output level"` (engine peak) and
   `tools/android/observe.py listen` if the mic is available. Expected peak ~0.7-1.0 for a loud song; report it.
