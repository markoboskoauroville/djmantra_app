# Round 21 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 21 finished")

Build: the green Android run of the newest commit with this file. Fixes for round 20's fails (r20 REPORT.md).
Grep once per part: `adb logcat -d | grep -E "Quick:|WindowBridge|GenreLookup|Output level|automix"`.

## 0. Smoke (10 s): `Quick: interface ready`, not black, no `qml` error.

## 1. Transport (quick): PLAY / pause, aligned start, AUTO PHASE once; 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick): crossfader, one EQ knob, double tap.

## 3. LEDs (quick): 32 BEATS portrait (cells ~129 px); LANDSCAPE Phrasing at 16 and 32: the beat numbers fit their
LEDs, no two-digit numbers running together (round 20 FAIL). Screenshot landscape 16.

## 4. CPU (round 20 FAIL: ~75 % of one core)
- Both decks playing, SYNC on, 60 s on each tab (Songs, Phrasing, Mixer): `top -n 3 -d 5 | grep -i djmantra`.
  The overviews' play line now moves 10 times a second instead of with every engine update (~47 a second): the window
  is drawn far less often. Report each tab, and with the screen OFF for comparison (`input keyevent 26`, 30 s, wake).

## 5. Screen
- Songs tab: a tap on the EMPTY area under the songs (e.g. below Download in a short folder) brings the navigation bar
  (round 20 FAIL); a press anywhere hides it. Tap map re-logged after the bar comes / goes (`Quick: map ...` after
  `Quick: insets ...`).

## 6. AUTOMIX (round 20: "did nothing"): it is NOT an automatic fade (Marko: "automix ... means that we'll do EQ and
tempo blend as automix. So I'm manually moving the fader"). Test it as a DJ would:
- Deck 1 playing, crossfader FULL LEFT; deck 2 playing (SYNC on, another BPM before SYNC is fine). AUTOMIX on: deck 2's
  EQ knobs (Mixer tab) drop to the bottom (killed).
- Move the crossfader slowly to the right in ~10 steps (`input swipe` on the crossfader, or drag): the button shows the
  percent; on the Mixer tab deck 2's LOW comes up while deck 1's LOW goes down first (bass swap), then HIGH, then MID;
  deck 1's BPM (`sync` log or the header) glides toward deck 2's. Screenshots at ~30 % and ~70 %.
- At the right end: log `automix: [Channel2] is the outgoing deck`; moving back left runs the blend the other way.
- AUTOMIX off: all EQs back to the top.

## 7. Genre (round 20: 2 of 6)
- Load songs without a genre (the 4 from round 20: Awka - Astral Voyage, Viel - Beautiful Place, Gorje Hewek - Children,
  Deaf Unique - Connecting Beat, plus 2 more): MusicBrainz first, then Deezer (`GenreLookup: Deezer ...: <genre>`).
  How many found now? Sensible?

## 8. Notes to check
- The BPM of Yshai Afterman (187.3 in round 20, double time?): the value now; Marko to decide on a BPM range.
- Re-open the app, open Music: values there at once, no or few re-analyses.
