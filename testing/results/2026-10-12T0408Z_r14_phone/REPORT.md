# Round 14 on the phone (Nothing A142, Android 16), 12.10.2026

Build: Android run 38195388871 of 43e3538, sha256 `164bbbf07f27bd339d644fa334d24711053e2bac9e5a69f5725573c5704b4622`,
installed 06:08 CEST. Watcher `~/djmantra-watch/r14`. `-hole` copies: camera hole red, 126 px line yellow.

## Marko, 06:09, while the round ran

> "please continue testing and take a screenshot, and you're going to see that text from application is under my wi-fi and clock"

Confirmed in A1b below: as soon as the bars come back (any swipe from the top edge), the clock and the notification
icons sit on the dj logo, and Wi-Fi, LTE, signal and battery sit on the Camelot badge "8A > 5B clash". **The bars then never hide again.**

| # | what | expected | what happened | pass/fail |
|---|---|---|---|---|
| A1a | Classic: bars hidden after the start | hidden | `06:09:00.639 WindowBridge: hiding the system bars` → `requestedVisibleTypes -528`. `dumpsys`: statusBars and navigationBars `visible=false` (`a1-classic-start-hole.png`) | PASS |
| A1b | Swipe from the top: bars show, the app makes room, they hide again after ~4 s | room made, hidden again | `06:09:23.007 requestedVisibleTypes to -525 (was -528)`: bars visible. **The app does not make room**: the clock sits on the logo, Wi-Fi/LTE/battery on the badge (`a1-swipe-0s-hole.png`). **They never hide again**: still `visible=true` at 5 s and 15 s (`a1-swipe-5s`, `a1-swipe-15s`). **No "WindowBridge: bars … insets …" line** was logged after the swipe: the bars watch did not run | **FAIL** (Marko's report) |
| A1c | Logo while the bars show | menu | No menu, no log line (`a2-logo-bars-shown-hole.png`) | FAIL (A1b) |
| A2a | Logo → menu (bars hidden, after a restart) | menu | Menu opens (LIBRARY, CONTROLLER, ONE DECK, REC, Settings). **The ✕ close button sits right under the camera hole** (`a2-menu-hole.png`). No "Menu" log line for opening it | PASS / **hole FAIL** |
| A2b | Menu → Settings | opens, logged | `06:10:44.591 Menu: Settings tapped`, `06:10:44.608 Settings screen open QRect(0,0 413x919) visible true`. Settings shows Main Volume, Split Output, New interface (GPU), Draw on the GPU (test), Sound, Library, Camelot wheel (`a2-settings-hole.png`). Title and ← clear of the hole | PASS |
| A3a | Counter readable | readable size | Not cut (`0:43/5:56/-5:13`), white, but **still ~8 px high** on a 1084 px screen | FAIL (size) |
| A3b | Waveform width | (report) | **~30 % of the lane** on both decks (round 13: ~10 %) (`a3-waveforms-hole.png`) | better, not full |
| B1 | New: TIME counter runs | elapsed/length/-remaining | `0:08/5:56/-5:47` → `0:23/5:56/-5:33`, both decks; BARS `71.1 -44.0 4/4` | PASS |
| B2 | New: play/pause icon | ▶ / two bars | Two bars drawn while playing (`b1-mixer-hole.png`), no tofu | PASS |
| B3 | New: waveforms fill their lane | most of the lane | `Quick: waveform gain [Channel1] 2.1055 (loudest 109 of 314336 points)`, `[Channel2] 2.08636 (loudest 110 …)`: the loudest point of the waveform data is only 109–110 of 255. **On screen still ~5–15 % of the lane** (`b3-waveforms-hole.png`) | **FAIL** |
| B4 | New: zoom −/+ | scale changes | +2: beat lines ~2× farther apart; −4: much closer (`b4-zoom-plus2`, `b4-zoom-minus4`). **But each deck zooms on its own: Marko says both decks must always share one zoom level** (below) | PASS (works) / design FAIL |
| B5 | New: Camelot wheel title below the hole | clear | Title and ← below the top row, hole clear (`b5-camelot-hole.png`). **Marko: the wheel itself "looks like rubbish"** (below) | hole PASS / look FAIL |
| B6 | New: library | Music once; ask on a playing deck | **Music still listed twice** (`b6-library-hole.png`). LOAD 2 while deck 2 plays → "Deck 2 is playing. Load the new song anyway?" CANCEL / LOAD; CANCEL kept Dune (`b6-load2-playing-hole.png`) | Music FAIL / ask PASS |
| B7 | New: menu header | DJ Mantra 0.5.0 | "DJ Mantra 0.5.0" (`b7-menu-hole.png`) | PASS |
| B8 | New: knob labels under their knobs | under | GAIN/HIGH/MID/LOW/FILTER right under each knob (`b1-mixer-hole.png`) | PASS |
| B9 | New: landscape | — | **Not tested.** A stray tap sent the phone to Android's recent-apps screen, then Marko stopped the round. Rotation restored to 0 | not run |
| C | Loudness | — | **Not run** (Marko stopped the round) | not run |

## Marko, 06:20: stop testing, fix everything, then test the new build

> "Please update the app and then test the new app. Don't continue testing. Whatever you found out until now, you fix,
> fix what I told you, and then test."

His feature request and two "very big" mistakes, in his words:

> "Please feature request. Loading song is pretty straightforward. You don't need to put into library or file picker,
> load into one deck or load in deck 2. Song is always loaded in the deck which is free, which is not playing, and goes
> in the logic 1-2, 1-2, 1-2. If both are not playing, then you replace one, the next song replace two. You need to see
> how this is done inside DJ Pro Chrome algorithm. They have the same feature, load into free deck, and they want to
> have it also here. And to load in specific deck, the users just click on the Artwork."

(Probably **djay Pro by Algoriddim**: "DJ Pro Chrome algorithm" is the dictation of "djay Pro, Algoriddim". Its "load to free deck" is meant.)

> "And I noticed that you are changing the zoom of zoomed-in waveform independently. This is not working. The both
> tracks need to be always the same zoom level so user can mix them. They need to be comparable like one track.
> That's very big mistake."

> "And second mistake: song sections are not recognized correctly. This countdown for the next section is counting down
> to the end of the song. That's nonsense. It's very clearly seen when there is intro, there is instrumental part, there
> is breakdown. So changes in the structure It's very easy to detect in the song because one section is doing the same
> patterns, then it changes. So you simply can count down to the actual next section when arrangement is changing. Bass
> is gone, vocal is gone, something is changing. So sections are easy to recognize. You don't need to recognize what is
> it. You just need to recognize its changes and then count down, count down until change of the section."

> "Camelot wheel looks like rubbish. Should be nicely drawn wheel. Like, look at my horoscope application. The wheel in
> horoscope is perfect. Same technology you need to use here."

The horoscope app is Donnie Horoscope, private repo `markoboskoauroville/astro_dos`, branch `ccr-2d5376a4-jo2igv`.
Its wheel is `android/core/src/main/kotlin/com/astrodos/zodiac/modern/Wheel.kt`: a true circle drawn with
`Vg.band()` (filled ring sectors between two radii), thin sector boundary lines, glyphs centred in the band, degree
ticks, colours from a `Theme`; `Vg.kt` is the drawing interface, `AndroidVg.kt` the Canvas implementation.

### The full list for the next build

1. **Load to the free deck** (feature): one LOAD per song; it goes to the deck that is not playing; if neither plays,
   alternate 1, 2, 1, 2. Tap a deck's artwork to choose that deck explicitly.
2. **One zoom for both decks**, always the same, in both interfaces.
3. **Sections**: the "next" countdown goes to the next change in the arrangement (bass / vocal / pattern change), not to the end of the track.
4. **Camelot wheel** drawn like the horoscope wheel (true circle, ring sectors, clean boundaries).
5. Classic: bars come back after a swipe and never hide; the app does not make room (fix pushed after part A, not yet tested).
6. Classic: menu ✕ under the camera hole (fix pushed, not yet tested). Counter size (fix pushed).
7. Both: waveforms narrow (data peaks at 109/255; classic ~30 %, new ~5–15 % of the lane).
8. New: library lists "Music" twice.
9. New: landscape and loudness (C) still to test on the next build.

## Marko, 06:27: phrase LEDs, the horoscope style, and which build to test

> "add phrasing of 8 just next to the artwork, which is basically tiny squares, 4 at the top, 4 at the bottom on both
> decks, and they are just lighting up for each beat in the 8-bit phrase. We have also this logic already built in
> inside the slicer. They're the same LED Algorithm. Build this, update, and then test. Don't test the next version,
> test this one with new interface. And make interface same as my horoscope, the same style of interface, make it here."

So: **round 15 is not run on a1b044e.** The build to test is the one that also has the phrase LEDs and the
horoscope style, in the new interface.

- **Phrase LEDs:** next to each deck's artwork, 8 tiny squares, 4 on top and 4 under them. Beat 1–8 of the 8-beat
  phrase lights its square, the same LED logic as the slicer's.
- **Horoscope style** (Donnie Horoscope, `astro_dos`; screenshots in branch `phone-reports`, `manual/01-start.png`,
  `manual/08-planets.png`; colours in `modern/Vg.kt` `Theme.NIGHT`):
  - background `#0B1026` (deep navy); cards `#151C3D` / `#1F2850` with rounded corners;
  - ink `#EEF0FF`, secondary text `#9AA3C7`, lines `#6B76A8`;
  - one accent `#FFD54A` (yellow): the primary button is filled yellow with dark text `#1A1A1A`, the other buttons
    are navy cards with white text;
  - section titles in the yellow accent;
  - list rows: glyph left, title + grey second line, value right;
  - a bottom navigation bar of icon + label tabs, the selected one in yellow;
  - element colours fire `#FF6B5E`, earth `#6CCB72`, air `#F2D35B`, water `#5AA2F0`; tinted ring bands at alpha `0x38`.

## Marko, 06:33: the phrase lights are his mixing tool, more than the waveform

> "And this is the whole theme of the whole aligning these running dots. I remove waveform as such, give me this
> running lights which needs to be aligned, and we need to optimize this in user interface and in controller. It's
> always seen. This is my tool. My tool for mixing is section change, which I see the counter and Aligning the beats,
> meaning aligning the phrasing. The first beat of the phrase of one song needs to be aligned to the second, and light
> helping me to know where I am. Although I have counter now, but seeing the light helps also."

What this asks for:
- The 8-step phrase lights of both decks are **the** mixing view: always visible on every tab, in the interface and on the controller's LEDs.
- He mixes by two things only: (1) the **section change** (the countdown counter), and (2) **phrase alignment**:
  beat 1 of one song's phrase lands on beat 1 of the other's. The two rows of running lights side by side show at a
  glance whether the phrases are in step, and where in the phrase each deck is.
- The waveform matters less to him than these lights ("I remove waveform as such").
- The same lights on the controller (the slicer's LED algorithm).
