# Round 27 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 27 finished")

Build: V1 (the green Android run of the newest commit with this file). Start only when Marko says so (he is using the
phone). Grep once per part: `adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Sound:"`.

## 0. Smoke (10 s): `Quick: interface ready`, no `qml` error, `Sound output open ... (clock)`, frame rate -> 60 Hz.

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Mixer tab (fix: the black spike between the volume faders, Marko's screenshot)
- Both decks playing, Mixer tab, portrait: the two VU meters (between the faders) rise and fall green / yellow,
  NO black wedge or spike anywhere. Screenshot. Crossfader full left, deck 2 in the headphones (Marko's case): same.

## 3. Overview scrubbing (new, Marko: tap = jump without zoom; hold / drag = zoom in to place the playhead)
- TAP an overview: the deck jumps there at once, no zoom window (`Quick: jump [ChannelN] to X s`).
- HOLD still 0.3 s: a zoomed scrolling waveform (yellow frame, ~4 s wide) opens just above that overview
  (`scrub [ChannelN] zoomed from X s`). Drag right: the position moves LATER slowly, the waveform follows the finger
  1:1; left: earlier. Release: the window closes (`scrub ... ended at Y s`), the deck stays at Y. Playing and stopped.
- Drag at once (no hold): the zoom opens after ~12 px and the same fine movement. Both decks, portrait + landscape.

## 4. Landscape (new: only the decks, full width; the song list in front while browsing)
- Rotate: the deck part fills the whole screen (headers, buttons, modes, a taller matrix, taller overviews); no song
  list, crossfader or tabs at the side. Screenshot.
- Tap a deck's cover: the song list comes in front, full screen, with ← at the top (`browse open for [ChannelN]`).
  Tap a song: it loads onto that deck and the list closes (`browse closed`). Open again, ← closes without loading.
- Portrait: unchanged (Songs / Phrasing / Mixer / Keys tabs, crossfader).

## 5. CPU (round 26 part 4 again, void then): both decks playing, 30 s per tab, `top` + `top -H` top 8 threads on
Phrasing and Mixer. Do not touch the decks during the measurement.

## 6. Genre match (round 26 part 5): `GenreLookup: Deezer match: ...` per song for 3 songs without a genre.

## 7. Clock (round 26 part 6): HH:MM, colon STATIC, matches the phone's time, clear of the camera hole, both
orientations.

## 8. Mix Ultra (new; needs Marko to plug the controller in: USB or Bluetooth)
- BROWSE turn: the song list comes in front (landscape full screen; portrait the Songs tab), log `browse open`;
  further turns move a yellow-framed row (`controller` lines); SHIFT + turn: 10 rows. Press on a folder: it opens;
  SHIFT + press: up a folder (at the top: closes). Press on a song: loads onto the free deck, the list closes and
  the tab from before comes back. A deck's LOAD: the highlighted song onto THAT deck (a playing deck: LOAD again
  confirms).
- Pad mode buttons: HOT CUE = Songs, LOOP = Phrasing, FX = Mixer, NEURAL MIX = Keys (`controller: page N`), on
  either deck; the pads still work in the controller's mode.

## 9. Version and the classic screen gone (new)
- Menu: the version reads "V1". The APK is named DJMantra-V1-...; `adb shell dumpsys package com.djmantra.app |
  grep version`: versionName=V1, versionCode=6.
- No "BACK TO THE CLASSIC SCREEN" in the menu; the app always starts the new interface (`Interface: Qt Quick (GPU)`),
  even if an old setting chose the classic one. The APK has no assets/res/skins.
