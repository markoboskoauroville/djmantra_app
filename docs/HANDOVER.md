# DJ Mantra handover

Read this first in any new session (any account). It is kept current by every session before it
stops; the newest state wins over older notes. Last update: **2026-10-12 07:15 UTC** (see the newest section below); earlier: 2026-10-09 14:20 UTC, by the
cloud session "DJ APP cloud" (session_01DARPn46WRaCjZEMv9xJGE6), which stopped here because the
account's **7-day usage limit was at "warning"** (resets Thu 2026-10-15 01:00 UTC).

## 13.10.2026 night (latest): V24-V29

- FX DONE (V25-V29): all four within a few dB of djay's recordings on the phone (tester's band tables, audio in
  djmantra-test-songs fx/app_v2x_20261013). Echo Out runs while chosen; the cause of "ON does nothing" was
  QmlDJMantra::setControl using ControlObject::set() (its owner never hears it): now via ControlProxy (V27).
  Nit left: Echo Out's first 2 s (djay dips lows more, ours mids).
- CQ-12T on the phone's USB (Multitrack 16 ch): Android itself is slow on it (YouTube ~0.54x); our best is the
  PowerSaving (deep buffer) variant ~0.58x, kept automatically (V26-V28 variant search in sounddeviceoboe.cpp).
  Full fix needs Marko: CQ USB Stream Mode -> Stereo (MixPad CONFIG -> USB/SD), or our own USB audio driver (libusb).
  Asked Marko (yes/no), no answer yet. Do not switch the CQ without him.
- Open questions for Marko: lane height (djay 15 % vs ours 80 %), deep-zoom real wave, FX sliders.
- Also PASS on V29: STOP TIME (OFF / 1 s / 3 s, cancel), render thread 7-15 % (was 18-21), CQ over USB MIDI (CQ
  DELAY sets Ip1->FX1 0 dB and unmutes FX1, read back from the CQ). Blocked on Marko: the CQ Stereo decision, the djay
  waveform pairs (his Mac's screen is locked), a Tap Delay + soft key set up on the CQ for the tap tempo.

## 13.10.2026 evening: V17-V23 while Marko was out

- Lanes (V17, V21): mirrored per deck (Settings -> LANE MIRRORED / HALF), translucent, crimson kicks, less magenta,
  hits more distinct (x^1.7). Open: djay's two-deck lane is ~15 % high (ours 0.8), and djay draws the real wave at deep
  zoom (Mixxx keeps 441 peaks/s, unsigned): ask Marko.
- Mixer (V18, V23, Marko 17:22): one EQ dropdown (Equalizer / Neural Mix EQ, both decks), one FX button per deck under
  FILTER (tap on/off, hold 0.45 s = drop-up chooser), unit N slot 1; FxBar.qml removed. ROUND38 parts 1-3 PASS on V21.
  Neural Mix Crossfaders (djay's 3rd entry) not built.
- FX (V19) measured from djay's recordings (djmantra-test-songs fx/djay_20261013, testing/djay/fx/notes.md):
  Underwater lowpass 16k->3.3k (middle)->700 Hz; Cooldown band-pass 400 Hz-3 kHz, -6.5 dB, fade-out above 0.6;
  Echo Out runs while chosen, button_parameter1 = ON: the last 1/2 beats loop, x0.6 a round. No sliders in the UI yet
  (the FX run at djay's middle). Flanger = Mixxx's.
- CQ (V20-V22): CQ link over its USB MIDI (Settings -> CQ MIXER -> USB, CqMidi.java). USB audio: the CQ on the
  phone plays at ~0.3x (Output rate log ~14000 of 48000 frames/s) and the CQ hears nothing; Android lists no channel
  counts for it (Profiles 0) while its driver runs 16 ch 32-bit. V22 forces 16 channels for such a device (untested).
  If other apps also play slow on it, it is Android's USB audio: then the way is a direct USB audio driver (big).
- All-In-One: keeps dying on "Beyond Strings.mp3" (11 songs done): skip it in run.py if it dies again.

## 13.10.2026 afternoon: V16, djay FX, CQ USB without MMAP

- V16 (c672e09, a82f54e): djay Pro's favourite FX (Marko): Flanger (Mixxx's), Underwater / Cooldown / Echo Out
  (new, src/effects/backends/builtin/djayfx.h + djayfxeffect.h, tests src/test/djayfx_test.cpp); FX bar per deck on
  the MIXER page (res/qml/djmantra/FxBar.qml: deck N = effect unit N slots 1-3 + unit N+2 slot 1 for Echo Out).
  Tuned by ear; the tester records djay's own FX into testing/djay/fx/ to tune against.
- CQ-12T on the Pixel's USB (16 ch, 32-bit, 48 kHz): the MMAP stream timed out every 1.1 s (no sound). V16 reopens
  without MMAP after the first ErrorTimeout (sounddeviceoboe.cpp m_avoidMmap). Open: headphones on USB 3-4 opens 4
  channels, which Android may map as quad, not as USB 3-4: ROUND36 part 6 checks it. The CQ is on the phone's USB.
- After V16 (not yet in a build): beat lines only near the screen (WaveformRow.qml windowCenter), render-thread CPU.
- Marko away ~2 h from 14:25 UTC: work on alone with the tester; ROUND36 first.

## 13.10.2026 day: V14, djay waveform, Neural Mix, CQ-12T, CPU

- Tester: "mantra dj app tester" (session_01KmLixQnDquGdTPkysjRCNV) on Marko's Mac, Pixel 7 over Wi-Fi adb; voice to
  Marko only for hands (his MANTRA_VOICE). Rounds: testing/ROUND30-34.md, results in testing/results/.
- Done and passed on the phone: djay Pro waveform (colours fitted to djay's data: src/waveform/djaycolor.h + the
  lanes' shader res/shaders/djaysignal_qml.frag; the lanes' texture is now built on load, qmlplayerproxy.cpp);
  Neural Mix EQ mode (src/effects/backends/builtin/stemeq*); Settings -> SECTIONS / BEATS options; ✕ on every
  overlay; PLAY drawn like NEXT; GPU meter (render-thread time: no GPU timestamps on the Pixel); CPU: the matrix
  display paints only lit dots (interface thread 39-46 % -> 4-11 %; Phrasing page 48-52 %).
- CQ-12T (Allen & Heath): USB multitrack 16x16 @48 kHz (app master -> USB 1-2 = Ip1/2, headphones -> 3-4 = Ip3/4);
  CQ MIDI over TCP 51325 verified from the Mac (testing/results/20261013_cq12t/midi.md); the app has Settings -> CQ
  MIXER + Mixer page CQ DELAY + tap tempo (qml/cqlink.h). Not yet tested on the phone: waits for the Pixel's USB on
  the CQ and the CQ's network on the "mantra" router.
- Open: lanes still a bit taller than djay's thin band; render thread 18-21 % (beat lines as one pass); the
  Hercules as a CQ surface (CQ12T_FX.md); All-In-One run in /root/allin1run (the container restarts kill it:
  restart with nohup setsid); landscape song list sizing.

## 13.10.2026 night (latest): V7, the analysis measured against rekordbox

Read `docs/RESULTS_2026-10-13.md` first (results, "Summary for the morning"), `docs/HANDOVER_PHONE.md` (phone), `docs/COORDINATION.md` (the sessions: cloud builder,
phone tester "dj amp local" session_01MCmmCT2kk49c9sPPKrkUk8; the Mac helper is archived, not needed).
ROUND29 tests V7 (parts 0-2 PASS; V7 vs rekordbox: key 78 %, BPM 120/150 the same, grid 8 off-beat left).
Round 29 finished (4/4 PASS); sections on the phone F1 0.54 within ±1 bar vs rekordbox (bench 0.50). Open: double / half BPM (7 songs, listed in RESULTS);
the 8 off-beat grids; All-In-One installed in the cloud (tools/analysis/allin1/) but its weights need
huggingface.co + dl.fbaipublicfiles.com allowed in the environment's Network access (Marko); djay Pro
waveform look (testing/djay/). Next build: bump to V8 (f9e2d7a: `Beats:` log lines name the file).

## 12.10.2026 17:00 UTC: test rounds 22-26, the loop runs by itself

- Builder: this cloud session (branch `ccr-eb7d1019-d3kubd`). Tester: the local session on Marko's Mac (it watches
  for `testing/ROUND<N>.md` + a green Android run, pushes `testing/results/*_r<N>_phone/REPORT.md`, messages each part).
  This cloud session cannot message the tester back (auth): the ROUND file is the signal.
- Round 26 (a5fd87c): parts 0-3 PASS; paused while Marko uses the phone; parts 4 (CPU) and 5 (genre match) next.
- Fixed in rounds 22-26: audio router reads the outputs over JNI (no `__OBOE__` in the QML lib); Deezer plain search,
  English (`Accept-Language: en`), artist must match, match logged, Croatian saved genres re-looked up; CUE on a
  paused deck returns to the cue (`cue_gotoandstop`), cue set when the deck reaches the beat, only a hand-pressed beat
  sets the cue; AUTO PHASE never moves the on-air deck, ignores a CUE preview; hidden tabs are Loaders; 60 fps cap
  (WindowBridge.limitFrameRate); Android MIDI controller woken by data, not polled; `ControlProxy.maxRate` (C++)
  throttles playposition (4-10 Hz displays, 60 Hz waveforms) and VU (20 Hz); AUTOMIX EQ values logged; Settings ←
  fixed; tap map logged again 6 s after the start.
- CPU (both decks playing, % of a core): r25 Songs 39, Keys 43, Mixer 58, Phrasing 65 (render thread ~21 %).
- Open: landscape layout (song list ~3 rows, small wheel); router with the Mix Ultra / Bluetooth plugged in (needs
  Marko); BPM cap at 150 (Marko's answer pending); the strict Deezer query never matches (only the plain one does).

## 12.10.2026 07:15 UTC: the redesign, waiting for round 17

- Cloud session "DJ mantra app testing" (session_01Ggm1Tq5a9mnY3wAZMMS9jh), branch
  `ccr-eb7d1019-d3kubd`. Head a91fd92 (= ca2e6ee + a test fix). Android of ca2e6ee is green.
- The phone session "Wireless debugging connection" (session_01EPqFCfKVESsy64tgK2QFvQ) is
  **archived** since 06:34 UTC (its 5-hour limit was at warning). A new phone session (Remote
  Control in Termux) runs `testing/ROUND17.md` on ca2e6ee's (or a91fd92's) Android APK, results to
  `testing/results/*_r17_phone/`.
- New interface (Qt Quick, `res/qml/djmantra`), Marko's redesign: deck headers (total / remaining),
  SYNC CUE PLAY JUMP per deck, AUTOMIX (EQ bass→highs→mids and tempo blend follow the crossfader
  Marko moves; nothing automatic), 8/16/32 BEATS (phrase_length), the OXI-E16 dot-matrix display as
  the ONLY phrase lights (elapsed, bar.beat, countdown, running lights, beat launch by tapping a
  cell), overview waveforms, the playlist (tap = load onto the free deck), crossfader, tabs Songs /
  Mixer (LED-ring knobs, cut-only EQ) / Keys (wheel, KEY MATCH via camelot::matchShift, −/+).
- CI: linux.yml has a "QML check" job (`tools/qml/check_djmantra_qml.py`, headless load against a
  Mixxx stub) since round 16 went black on a QML error (Style.onAccent read as a signal handler).
- Open: the section countdown after JUMP (round 16b: -9.0 vs ~14 bars; the jump log now lists the
  cues); hot cues / loops still in the engine, classic interface and controller (manifest §6);
  progress page (website repo) to update after round 17 passes.

## 11.10.2026: round 11 on Marko's phone itself

- Cloud session "DJ mantra app testing" (session_01Ggm1Tq5a9mnY3wAZMMS9jh), branch
  `ccr-eb7d1019-d3kubd`. "Djapp local" (Mac) no longer exists; the local session now is
  **"download-9c"** (session_015uEBPKd7RHnchVAbjobVrB): Claude Code in Termux on Marko's Android
  phone, adb to the phone itself over wireless debugging. Message it with `send_message`.
- Marko's 69 test songs (public Drive folder) → `/sdcard/Music` with
  `tools/android/fetch_test_songs.sh` (list: `testing/songs/drive-songs.tsv`).
- Round 11 sent 11.10 ~23:55 UTC (android-latest = 657f953): songs, device_test --sound, ui_test,
  5 songs load/play, two-deck mix with SYNC + crossfader, One Deck. Results go to
  `testing/results/*_r11_phone/REPORT.md` on `ccr-eb7d1019-d3kubd`. Next: read them, fix, push to
  `main` for a new android-latest, send round 12. Loop until everything passes.
- Built on the branch while round 11 runs (Marko's requests via the phone session, not yet on a
  phone, CI building 2b54c80): **deck counter** `WDeckCounter` (tap: TIME "1:23 / 5:40 / -4:17" ↔
  BARS & BEATS "17.3  next -6.2  4/4"; sections = intro/outro markers + hot cues, else to the end;
  PhraseControl beat_number, beats_per_bar, beat_unit, section_beats_left, section_found);
  **Camelot** (`src/widget/djmantra/camelot.*`: notation forced to Lancelot = Camelot, `<KeyMatch>`
  badge between the decks, wheel screen from the badge and Settings). To do next: song-structure
  and time-signature detection; mark matching keys in the song picker.
- 12.10 ~00:40 UTC: **GPU drawing on Android** (fbb6ceb): `OpenGLChildWidget` (QOpenGLWidget) under
  `WGLWidget`, all-shader waveforms and GLSL VU meters, 4 s fallback to the CPU, Settings switch
  "Draw on the GPU" ([DJMantra],gpu). Not yet run on a phone. Round 12 plan: `testing/ROUND12.md`.
  Round 11 partial results (657f953): `testing/results/2026-10-11_2353_r11_phone/`.
  No phone session online at 00:40 (download-4b archived): a new one starts from ROUND12.md.
- 12.10 ~01:00 UTC: b1f26f9 green on Linux (tests) and Android: first build with the GPU drawing,
  BrandTranslator (Mixxx -> DJ Mantra in every shown text), Camelot, counter, ui_test fixes.
  Phone session now: **"Wireless debugging connection"** (session_01EPqFCfKVESsy64tgK2QFvQ, adb
  192.168.1.30:38589), running ROUND12.md on b1f26f9's APK.
- Claude Code breaking down on the phone = Android's phantom process killer (10 Termux children
  killed, 32-process cap) plus memory pressure (DJ Mantra 491 MB PSS, swap 78 %). Marko approved
  turning the monitor off: `settings put global settings_enable_monitor_phantom_procs false` (done
  02:45 CEST; undo: true). `tools/android/watch_host.sh` logs kills/memory during rounds.
  To do: cut DJ Mantra's memory.
- **New interface in Qt Quick (GPU)**, 12.10 (Marko: "Yes, I want it"): `res/qml/djmantra/*.qml` +
  `src/qml/qmldjmantra.*` (Mixxx.DJMantra: insets, Camelot, folders, waveform gain, interface switch).
  The default since 12.10 (Marko: "make new interface default"): ([DJMantra],classic_ui) false; the old
  quick_ui key is ignored. Settings -> "New interface (GPU)" off or menu -> classic sets classic_ui true.
  Round 13 (d2dbddb): loads, all controls answer, CPU 32-55 % vs 126 % classic. Fixes in 43e3538 (time
  from playposition x duration, drawn pause icon, waveform gain, zoom via waveform_zoom, overlays below
  the hole, library confirm, landscape layout). Round 14 (`testing/ROUND14.md`) sent 04:10 UTC.
  Still missing in it: pad modes, recording, controller mapping, native settings, One Deck.
- Open: waveform data looks low (5-25 % in every renderer and the overview): ROUND14 C measures the
  real loudness. Classic: Qt full screen off on Android (bars fight), StartInFullscreen forced false.
- Pushing to the branch cancels the Android run in progress: wait until the phone has its APK.
- Pushing to `main` (android-latest) was blocked by the auto-mode classifier: needs Marko's OK.
- The 7-day limit is at warning (resets 2026-10-15 01:00 UTC): keep this file current.

## Where things are

- Repo `markoboskoauroville/djmantra_app`, development branch `claude/admiring-feynman-hym3vp`
  (also pushed to `main`; a push to `main` builds the APK and, when green, replaces the
  `android-latest` release). Don't push doc-only commits to `main` while an Android build of
  `main` runs: they cancel it.
- Branch head: `6d19dcb` (UI test fixes). `main` = `6155c56` = android-latest (CI green).
- Website/progress page: repo `markoboskoauroville/djmantra-ecstatic-dance` (v24 now).
- Standing rules: `CLAUDE.md` (one action at a time for Marko, `!!!!!MARKO!!!!!>` prefix,
  progress page after milestones, no credentials, desktop Linux must build and pass tests).

## How testing works now (owner's rule, 9.10.2026)

- **Only the Pixel 7 emulator** (`emulator-5554`, on Marko's Mac) for now. The real phones
  come later. The Nothing Phone (2a) is out of the test loop.
- Tests run through the local session **"Claude Code local"** (session_01NtxDMqMEkD435RBFuqFs1s,
  Remote Control on Marko's Mac). Send it exact commands with `send_message`; it replies with
  `SendMessage` to "DJ APP cloud". Protocol: `testing/README.md`.
- Tools: `tools/android/device_test.sh --sound --apk-dir DIR --serial emulator-5554` (launch,
  sound, permissions), `tools/android/ui_test.py --serial emulator-5554 --out DIR` (taps every
  control in portrait and landscape; app started with `--ui-test --play`), the app's
  `--ui-test` widget map in logcat, `tools/skin/render_skin.sh` (desktop renders in Xvfb).
- Routines: "DJ Mantra: hourly resume (cloud)" (trig_01GEgoXS7Y35wD9tS6AyYGUs) is **paused**
  while the weekly limit is at warning; "DJ Mantra: restart after the weekly limit reset"
  (trig_01XgbkeGxhhMckq9m9nVUchT) fires once on 2026-10-15 01:05 UTC, re-enables it and
  continues. "(Claude Code local)" (:30) keeps the emulator awake and finishes interrupted
  rounds. A session on another account can simply start from this file instead.
- Round 4 (emulator only) was sent to "Claude Code local" at 08:52 UTC; its results go to
  `testing/results/*_ui_emulator/` (check whether they arrived).

## Done (9.10.2026)

- New DJ Mantra skin (`res/skins/DJMantra`, generated by `tools/skin/gen_djmantra_skin.py`)
  after Marko's djay screenshots (`docs/ui-reference`): album art instead of turntables,
  portrait + landscape (SizeAwareStack by width), tabs mixer | waveforms | pads, 1 | Mix | 2,
  double tap on waveforms = full screen, library with LOAD 1/2, gear (Preferences),
  CONTROLLER (virtual controller). Default skin on Android, LateNight fallback.
- Android: no action bar, immersive system bars (`WindowBridge.java`, `util/androidwindow.*`),
  permissions in the manifest, software VU meters, --play test hook, waveform launcher icon.
- Virtual controller + remapping engine (`src/controllers/djmantra/`): any Mix Ultra input
  can do any function; layout from Marko's Controller Mapper app
  (`res/controllers/djmantra/mix-ultra-layout.json`).
- Round 3 on 3 devices: sound passes everywhere; the UI test's 23 failures were the test
  itself (log buffer, slow double tap); fixed in `6d19dcb`, not yet re-run.

## Round 4 result (emulator, build 6155c56, `testing/results/2026-10-09_1052_ui_emulator/`)

- Sound test passes (peak 0.125); CONTROLLER screen works (controller.png).
- **VU meters still don't move** (vu-1.png, vu-2.png) although the software meters are in
  this build and the log shows output peaks ~0.3: check that `WVuMeterLegacy::maybeUpdate`
  gets `waveformUpdateTick` on Android and that `[ChannelN],vu_meter` changes (watch it with
  the --ui-test probe), fix, re-test.
- Update 09:15: the deck 2 "failures" were the test (the emulator logs the changes late; the
  test now waits up to 4 s, `d36dd61`). Desktop: deck 2 works, the software VU meters move.
  The probe now logs "UI levels deck1 deck2 main" every second to see whether vu_meter moves
  on Android (`a5a97b8`). Round 5 is scheduled (send_later 09:42 UTC).
- (old) **19 UI test failures that now look real** (the log-reading fix is in): in landscape almost
  every **deck 2 control on the right half** (pregain, volume, EQ, bend, sync, pitch) and in
  portrait deck 2's EQ/filter and both volume faders; the **waveform double tap** in both
  orientations. Deck 1's same controls pass. Look for an invisible widget over the right half
  (e.g. a stack page or the hidden SizeAwareStack child catching touches), compare the tap
  coordinates in `log.txt` with the widget map, and check the double-tap timing with
  `cmd input`.
- The report's "Sound: peak 0" is the report's parsing (the app logs "Output level: peak").

## Virtual controller (9.10.2026, after Marko's Controller Mapper screenshot)

- Drawn like Marko's Mac app "Controller Mapper (Mantra)" (repo
  `markoboskoauroville/mix-ultra-mapper`, `MixUltraMapper.swift`): body, caps, grooves, jogs,
  monospaced labels; colours swapped as Marko asked: **orange = a control, green = the
  selected one**, white text = a chosen function.
- A press or move on the real Mix Ultra selects that control (MIDI observer in
  `ControllerOverrides`); CHANGE (or a tap) opens the function list. Not yet tested with the
  real controller (needs Marko or a MIDI test sender on the emulator).

## Round 5 (emulator, a5a97b8, `testing/results/2026-10-09_1132_ui_emulator/`) and fixes

- Sound passes; controller page in the mapper style works (tap = green, CHANGE enabled).
- VU: level 0.58 on deck 1 but the meters stay grey → `WVuMeterLegacy::maybeUpdate` was called
  directly from the render thread (repaint there draws nothing on Android) → queued on Android.
- Portrait: status + navigation bars came back after rotation → hidden again after the
  rotation settles (androidwindow.cpp).
- Landscape: deck 2's EQ, sync, bend, pitch still don't respond on the emulator (desktop OK);
  the probe now logs "UI press <widget class> <name> x y" for every press, so round 6 shows
  which widget gets those taps.
- Controller: button labels shrink to fit; no tofu arrow in the function list.

## Round 6 (emulator, fb33586, `testing/results/2026-10-09_1201_ui_emulator/`) and fixes

- Same 23 failures as round 5; VU meters still grey (level 0.58); bars back in portrait.
- Cause found in the log: Android paints the software waveforms from `repaint()` into a null
  device: ~800 "QPainter: Painter not active" warnings per frame (13 000 log lines during the
  landscape mixer test, the "UI levels" timer starved), so the GUI thread lagged and touches
  were dropped. The VU meters' `repaint()` never reached the screen either (the earlier queued
  connection changed nothing: the tick already runs on the GUI thread; reverted).
  Fix: `update()` instead of `repaint()` on Android in `WaveformWidgetAbstract::render()` and
  `WVuMeterLegacy::maybeUpdate()`.
- Bars: first tried Qt's full screen window state (a4aa481): the app then hung at start on the
  CI emulator (black screen, round 7 build failed), reverted. Now `WindowBridge.java` watches
  every layout pass and hides the bars again when they are shown for good (swipe-revealed
  transient bars don't count).
- The "UI press" lines only show containers because controls take touch events (not mouse
  presses); the deck 2 SYNC tap worked by hand (1 then 0). Re-check deck 2 in round 7 once the
  lag is gone.

## Round 7 so far (14:20 UTC)

- a4aa481 (Qt full-screen state) ran fine on the Mac's arm64 emulator (bars gone, no hang);
  the CI x86_64 hang had no DJMantra log line at all: probably the CI emulator. The Java
  re-hide (1673838) replaced it anyway; if bars show again, go back to a4aa481's way.
- VU grey found: `Paintable::drawInternal` drew part of an SVG with the view box × pixel
  ratio (past the SVG at 2.625). Fixed in 9db7422 (test program in the commit message); the
  "Painter not active" flood was the same code with a 0-high target, now skipped.
- Deck 2 by hand on the emulator: pitch and volume drags work; HIGH knob drag and SYNC tap
  did nothing. ui_test's drags fail where hand drags work: check ui_test.py's drag speed.
- **Song picker done** (9db7422): note icons → `src/library/djmantra/trackpicker.*`;
  renders `docs/skin/picker-*.png`. Build 9353a98 on main = round 7 (sent to Claude Code
  local: emulator + Pixel 7).

## Round 7 results (emulator, 9353a98, `testing/results/2026-10-09_1636_ui_emulator/`)

- VU meters GREEN, "Painter not active" 0, no bars in portrait, deck 2 works by hand (HIGH,
  SYNC, volume). ui_test still marks 19 rows (deck 2 drags/turns, waveform 2x taps) where hand
  input works: fix ui_test.py's drag/turn (speed/length) and double-tap timing next.
- Picker on Android: bars over it, and opening a folder left the old rows half drawn (no
  songs visible). Fixed in the next build: screens hide the bars when they open, the Java
  watcher also checks every 1.5 s, the list lays out and repaints after each folder change
  (logs "TrackPicker <title> rows N").

## Android-style menu and settings (14:50 UTC, `docs/skin/menu-sheet.png`, `settings.png`)

- The header's round button opens djay's menu sheet (`[DJMantra],show_menu`): LIBRARY (the
  picker without a deck: a tapped song asks for deck 1 or 2), CONTROLLER, REC, Settings.
- Settings (`src/widget/djmantra/settingsscreen.*` on `mantraui.*`): Main Volume
  ([Master],gain), Split Output ([Master],headSplit), rows Sound / Library / Virtual
  controller / MIDI Devices / Advanced, Online Help, About, version. Sound, MIDI and Advanced
  still open Mixxx's preferences dialog: rewrite those pages natively next (output device,
  buffer size; MIDI devices list; the rest).

## 10.10.2026: real phone, eyes and ears (owner's request)

- Device tests now go through the new local session **"Djapp local"**
  (session_01HGBdB5gPBHRGZUks9DixYh) on Marko's Mac, with the real phone over USB.
- `tools/android/observe.py`: the Mac's microphone hears the phone's speaker (level, strongest
  frequency, `--expect 440` / `--silence -50`), a camera on the Mac looks at the Mix Ultra
  (`look`, and `lights` = which cells of a 32x18 grid changed between two pictures).
- Round 8 fixes in 8c83bcb (MP3 as MP3 on Android, /sdcard/Music and Download as the picker's
  folders, bottom sheets instead of QMenu, Settings scroll repaint, Qt full screen + Java
  re-hide). Round 9 sent to Djapp local: setup (cameras, mic, controller connection), sound by
  ear, lights by camera, the new screens on the real phone, ui_test.
- ui_test: its deck 2 "failures" are the test's check, not the touch (the replay of its exact
  swipes moved deck 2's knob, round 8 swipe-replay.txt). Still to fix in ui_test.py.

**Priority (Marko, 10.10.2026): user interface fixes first.** Round 9 also tests every audio
and video file under the phone's Music folder (load, play, heard by the mic) and, last, a short
automatic mix (two songs, SYNC, crossfader sweep, listened to by the mic).

## One Deck (10.10.2026, after Marko's djay Pro screenshot; `docs/skin/one-deck.png`)

- Menu sheet → ONE DECK / TWO DECKS (`[DJMantra],one_deck`). Album art (tap = the folder list),
  title / artist / BPM / key / time, overview, waveform, prev / play / next / CUE, then the
  songs of the folder: the main window puts an embedded `TrackPicker` into the skin's
  "OneDeckList". A tap plays at once (orange row), ⏮ ⏭ and the end of a song go through the
  folder.
- Also on the branch: engine sample rate from the Oboe stream (the 8.8 % fast playback), the
  UI making room for the system bars (insets → content margins), the controller-style mixer
  (HIGH / GAIN, MID, LOW, FILTER; 1 · 2, MASTER, headphones) and pad modes, CUE instead of
  SET, the "dj" logo, the app icon on the start screen.

## New requests from Marko (9.10.2026 afternoon, screenshots in `docs/ui-reference/`)

- (done 9db7422) **Note icons = file pickers per deck** (`djmantra-portrait-mixer-note-icons.png`): the
  album-art note icon top left loads a song into deck 1, top right into deck 2. The picker
  looks like `file-picker-folders.png` (dark, close X top left, folder source dropdown,
  "Folders" title, + and ⋮; folder rows; bottom tabs Files | Queue | History).
- **Settings and the library: Android style, rewritten from scratch.** The current library
  (`djmantra-library-now.png`: DECKS/LOAD 1/LOAD 2/CONTROLLER buttons, table with
  Prev/Cov/Last Played/Album columns) and Mixxx's Preferences are "a desktop UI moved to the
  phone". Model: `djay-settings-top.png` / `djay-settings-bottom.png` (back arrow + title; blue
  section headers; icon + label rows; Main Volume slider; switch with a grey description;
  version at the bottom) and Android's own `android-settings.png` (rounded grouped cards,
  search field). `djay-menu.png`: djay's top menu (REC, Settings, MIDI, brightness). The whole
  app is to feel like an Android app.
- **The real Pixel 7 is back in the loop** (wireless debugging), no lock since 10.10.2026 (one session drives it).

## Next, in order

1. **Round 4 on the emulator** with android-latest (6155c56): `device_test.sh --sound`,
   `ui_test.py`, two mixer screenshots 1 s apart (VU meters must move), library → CONTROLLER
   screenshot. Fix what fails.
2. **Beat Slices view, the 4th tab** (task "Beat Slices", Marko's innovation): N pads (any N:
   6, 8, 12, 16 ...) light the current beat of the phrase; tap = jump to that beat (playing:
   jump keeping the phase, in sync; paused: cue exactly on the beat); long press on the tab
   (or the grid) picks N, "set beat 1 here", "auto beat 1". **Next to the grid: a small
   zoomed waveform of the song with its sections**, so the view is waveform + lighting beat
   grid. Engine side exists: `src/engine/controls/phrasecontrol.*` (beat_in_phrase,
   phrase_length, phrase_anchor, phrase_set_one, phrase_jump); to do: allow any length
   1..64 (now 4/8/16/32 only), paused jump = exact beat, a `WBeatSlices` widget
   (`<BeatSlices>` in LegacySkinParser), the tab in the skin generator.
3. **Beat 1 and time signature detection** per track (robust, songs that don't start on
   beat 1, meters 3/4, 6/8, 4/4); slices follow it automatically.
4. **Redesign every screen** (Preferences, dialogs, menus, library) in the DJ Mantra style:
   no Mixxx look anywhere.
5. **Match the Hercules Mix Ultra layout on screen**: mixer column HIGH, MID, LOW, FILTER,
   then the faders (as on the hardware); pads, sync/cue/play like the hardware.
6. Later, with Marko present: the real phones, USB-C audio out, the Mix Ultra over USB/BLE
   (its LEDs can be checked with the Pixel 7's camera: scrcpy camera capture, see
   `testing/results/camera/`), USB stick.

## Open questions for Marko

- None blocking. Ask only for what needs a person, one action at a time (`!!!!!MARKO!!!!!>`).
