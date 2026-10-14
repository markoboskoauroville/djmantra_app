# Round 31 (V9; rules: testing/PROTOCOL.md; tester "mantra dj app tester" on Marko's Mac, Pixel 7 over USB)

Build: V9 (the newest green Android run on ccr-eb7d1019-d3kubd, releases/tag/android-latest; the menu shows "V9").
**Paramount (Marko 13.10.2026): djay Pro's waveform shape.** V9 draws:
- the overview (deck header) as djay: mirrored around the middle, a darker outline + brighter body, djay's colours
  (red-pink kick parts, green / teal breaks, blue-violet highs), on #141414;
- the scrolling lanes (Phrasing page; the zoom window while scrubbing the overview): a thin mirrored wave (hits
  read as small coloured arrows), orange bar lines with the bar number at the top, grey beat lines, #141414.
Preview drawn in the cloud from the same formula: testing/djay/v9_preview_nanana.png.
Voice to Marko only for a hand on the controller (his MANTRA_VOICE).

## 1. The waveform, side by side with djay Pro (the main part)
- Load (It Goes Like) Nanana - Edit, Brick House, Lovely Day, 4B on the decks. Screenshot the deck header overview
  and the Phrasing page lanes (play 10 s so the lanes show bars), and the scrub window (hold on the overview).
- Next to each, the same song in djay Pro on the Mac (or testing/sections/djay_vs_app.py / testing/djay/render.py).
- Report per song: colours (where red, where green, where blue), the shape (thickness, the arrows), the bar lines and
  numbers (bar 1 on the song's first downbeat?), anything that looks different from djay. Push the screenshots to
  testing/results/<date>_r31_pixel/ and a short REPORT.md. Message the result of this part at once.

## 1b. Neural Mix (new in V9, as djay Pro)
- Mixer page: each deck has an EQ / NEURAL button under the knobs. NEURAL: the knob names become HARMONY / VOCALS /
  DRUMS. Log: `Neural Mix [ChannelN] on`.
- Play a song with a clear voice (e.g. Lovely Day). NEURAL, VOCALS to 0: the voice mostly goes, the band stays;
  DRUMS to 0: the kick and hats go; HARMONY to 0: drums + voice stay. Record 10 s each with the Mac's microphone
  (tools/android/observe.py) and push the recordings; say what you hear.
- Switching EQ <-> NEURAL while playing: a short smooth change, no click, no jump in time against the other deck.
- CPU with both decks in NEURAL and knobs off centre (`top`), compared with EQ mode.

## 1c. Every overlay closes (Marko: "I'm stuck")
- Landscape: tap a deck's cover -> the song list in front; open a folder; tap ✕ (top right of the list): back to
  the decks, nothing loaded (`song list closed (✕)`). Again, then Android's back gesture: the same.
- Menu: ✕ closes it; Settings: ✕ closes it. Portrait too.

## 2. Smoke + basics: PLAY / pause both decks, faders, knobs, aligned start, 30 s both playing: xruns, late 0.
CPU of the Phrasing page with both lanes playing (`top -n 1 -b | grep djmantra`) vs round 25 (65 %).

## 3. Continue round 30's parts 2-6 (sections of the 150 songs, djay comparison, Settings options, Bluetooth
controller) on V9. Also check: `Sections:` logged once per load (was 3 times); 4B's key in the song list vs the deck
header (was 6A vs 1A).
