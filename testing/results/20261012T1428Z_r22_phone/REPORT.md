# Round 22 on the phone (Nothing Phone (2a), A142, Android 16, 1084x2412)

Build: run 38260844240, commit 94fc08d (the builder's head after 98e3fae, 5a23541, 2df39cf). Started by the tester's build watcher, 12.10.2026.
Round 21 was skipped at the builder's request (its fixes are in this build).

## Part 0: smoke - PASS
- Ready in 6 s, no qml error. `AudioRouter: 9 output(s) added`; `Sound output open: "Phone audio output (speaker, USB or Bluetooth)" (clock) 48000 Hz, 2 channels, 1024 frames per callback, burst 256, API AAudio, device 3`; `Sound buffer: 3072 of 4096 frames`.
- It started in landscape (`interface ready 919x413`): the phone was lying sideways with auto-rotate on. Locked to portrait for the test with `cmd window user-rotation lock 0` (`settings put system user_rotation 0` alone did not turn it); auto-rotate is given back at the end.

## Part 1: transport - PASS (in landscape)
- Marko turned auto-rotate back on with the phone sideways, so the rest of the round runs in LANDSCAPE as he holds it (my first taps used a stale map and loaded a ganesha song; redone after a restart).
- Loaded 3 Sense (tap) and Addis (deck 2's NEXT). PLAY deck 1: `button [Channel1],play 1`, `played in this set: 3 Sense`. PLAY deck 2: `align [Channel2] to beat 6 ...`, `start [Channel2] from the cued beat 6 with the other deck's beat 6`, `play armed (aligned start)`, `beat launch [Channel2] on beat 6`.
- 30 s both playing: xruns 0, late 0, slowest 2.1-3.5 ms.

## Part 2: sliders and knobs - PASS
- Crossfader full left and back (deck 2 cut: headphones stayed on deck 2, right by the AUTO CUE rule).
- HIGH deck 1, slow drag 150 px down: pointer counter-clockwise, ring dims (lower); double tap: back. The neutral position is still drawn at ~4-5 o'clock (round 20 question).

## Part 3: LEDs and cueing - PASS (one note)
- Deck 2 stopped (deck 1 playing), Phrasing LED 3: `cue [Channel2] at beat 3`; the deck went there, the LED turned solid white (asked: a white frame), nothing started (`p3_cued.png`; matrix 49.3).
- PLAY: `start [Channel2] from the cued beat 3 with the other deck's beat 3`, `play armed (aligned start)`, `beat launch [Channel2] on beat 3`; both rows lit on the same cell afterwards (`p3_started.png`).
- Deck 2 stopped, LED 5: `cue [Channel2] at beat 5` → 60.1 (bar 60 beat 1 = phrase beat 5); CUE: `cue_default pressed`, still 60.1 (`p3_c1.png`, `p3_c2.png`). PASS
- Both stopped, LED 5, PLAY: `button [Channel2],play 1`: started at once (no aligned wait), as asked.
- **Note:** after that start, a pause and CUE, the deck stood at 59.2 (`p3_state.png`), 3 beats BEFORE the cued 60.1. Either the cue point moved with the start or CUE went somewhere else; please check which.
- Playing deck jumps: unchanged from round 20 (not repeated).

## Part 3b: NEXT and played songs - PASS
- Deck 2 stopped, its ▸|: `next [Channel2]: load Aktysia` (after Addis, skipping 3 Sense on deck 1), again: `load Alba`. Deck 2 playing: `next [Channel2]: the deck is playing, nothing loaded`. Held 0.7 s: `next section [Channel2]`.
- `played in this set: <path>` for every song that started (3 Sense, Addis, Alba). In the list: ✓ before the title, title and artist dimmed (`p3b_list.png`, `p3b_list2.png`); Aktysia (only loaded) is not marked.
- After a restart the marks are gone (`p3b_restart.png`).
- Landscape note: the song list shows only ~3 rows beside the decks (`p3b_list.png`); usable, but very short.

## Part 4: AUTO CUE - PASS
- Deck 1 PLAY (fader up, crossfader middle), deck 2 loaded and stopped: `auto cue: headphones to [Channel2]` at once; Mixer: deck 2's 🎧 lit, deck 1's off (`p4_deck1_on.png`).
- Deck 2 started (both on air, crossfader middle): no change. Crossfader full right (deck 1 cut): `auto cue: headphones to [Channel1]` (`p4_xf_right.png`). Back to the middle: no change. Deck 2 stopped: `headphones to [Channel2]`.
- Menu → SETTINGS: AUTO CUE: `auto cue off` (`p4_off.png`); crossfader right and back, deck 2 started and stopped: no switch. Restart: still off (`p4_after_restart.png`). On again: `auto cue on`.

## Part 5: audio router - FAIL (partly untested: nothing was plugged into the phone)
- Menu → SETTINGS: AUTO CUE, MASTER (the dance floor), HEADPHONES (pre-listen), APPLY (`p5_settings.png`, `p5_scroll1/2.png`).
- **FAIL: the lists show only "Phone default (follows plugging in)" (MASTER) and "Off" / "Phone default" (HEADPHONES). No "speaker" entry**, although the start log says `AudioRouter: 9 output(s) added` (Android's `dumpsys audio` shows earpiece and speaker). ROUND22 expected "Phone default, the speaker, and whatever is plugged in". Please log each output the router sees (`AudioRouter: <id> <type> <name> <channels>`) and show the speaker.
- MASTER = Phone default, HEADPHONES = Off → APPLY: `Sound output closed`, `Sound output open: "Phone audio output ..." (clock) 48000 Hz, 2 channels`, `audio route master -> phone default channels 1-2`, `audio route headphones -> none`; the page says "Done."; a song played afterwards (peak 0.32, xruns 0, late 0).
- NOT TESTED (needs Marko to plug something in): the list changing on plug / unplug, a second output (wired / Bluetooth / USB) with the 200 ms warning, MULTI-CHANNEL 1-2 / 3-4 on the Mix Ultra, the route kept / fallback after a restart, 2 minutes on the second output.
- Tester slip (no app fault): with the page scrolled, a tap meant for ← hit the AUTO CUE switch (off), switched back on at once (`auto cue on`). ← scrolls away with the page; a fixed header would avoid that.

## Part 6: Keys tab - PASS (font); note on size
- `p6_wheel_zoom.png` (full resolution): every code (12A ...), every key name (C#m ...) and the middle text (`1 10A`, `2 -`, `key`) in the normal font, no matrix dots; rings and borders smooth, no stairs or broken strokes.
- Note (landscape): the wheel is small there: ~310 px across (~118 dp), the key names ~15 px high, hard to read at arm's length. Portrait not re-checked this round (Marko had the phone sideways).

## Part 6b: AUTOMIX EQ curves - PASS (by the knobs; not by ear)
AUTOMIX on, EQ on, deck 1 playing, deck 2 playing synced, crossfader from full left to full right in 10 steps; a Mixer screenshot at each (`p6b/eq_0.png` ... `eq_10.png`; left column deck 1, right deck 2). No knob values in the log, so read from the pointers and rings:
- 0 %: deck 1 all bands full; deck 2 HIGH lowered (~1 o'clock), MID lowered, LOW killed.
- 10-40 %: deck 2's HIGH and MID rise step by step to full; its LOW stays killed; deck 1 untouched.
- 50 %: the swap in progress: deck 1's LOW half down, deck 2's LOW half up (neither killed, neither both full).
- 60 %: deck 1's LOW killed, deck 2's LOW full.
- 70-100 %: deck 1's HIGH and MID go down (HIGH lower than MID); its LOW stays killed; deck 2 full.
- Never both LOWs full, both LOWs down only together at half in the swap step, no band killed on both decks. As expected.
- Not judged: the sound ("no hollow moment"): Marko's ear. A knob-value log line (`Quick: automix eq [ChannelN] low x mid y high z`) would let the tester check the dB values.

## Part 7: round 21 leftovers - FAIL (CPU, genre; the rest PASS)
- **CPU: FAIL, no better.** `top`, both decks playing, landscape: Songs 81.6 %, Phrasing 79.0 %, Mixer 77.6 %, Keys 80.3 % of one core (round 20 portrait: 74-78 %; target 32-55 %). PSS 556 MB. Audio clean: xruns 0, late 0, slowest 3.1 ms.
- **Genre (Deezer): FAIL, 1 of 5 looked up.** NEXT loaded Aktysia, Alba, American Dream, Amulet, Astral Voyage, Beautiful Place:
  - Jakatta - American Dream: House (MusicBrainz).
  - Anonimat - Aktysia, Amonita - Amulet, Awka - Astral Voyage, Viel - Beautiful Place: `no recording ...` and then `Deezer has no track for ...` for every one. Deezer's search does know such tracks; four misses in four suggests the Deezer query or its parsing is wrong (log the URL and the HTTP status / result count once).
  - Alba logged no lookup at all (cached from an earlier round?).
- Empty-list tap: Songs, root folder, tap on the empty area below Download: `showing the navigation bar`, `bars shown, insets [126, 0, 126, 0]`, and the map is logged again after the insets change (`map tab0 1314 916 ...`). PASS (round 20 fail fixed).
- Landscape Phrasing at 16 BEATS: every number in its own cell, readable, not run together (`p7_land16.png`). PASS (round 20 fail fixed); still small (~18 px digits).

## Round 22 finished: 8 pass, 2 fail
Pass: 0 smoke, 1 transport, 2 sliders and knobs, 3 cueing a stopped deck, 3b NEXT and played songs, 4 AUTO CUE, 6 Camelot wheel font, 6b AUTOMIX EQ curves.
Fail, ranked:
1. Audio router: the lists show only "Phone default" (no speaker), though `AudioRouter: 9 output(s) added`. Second output, plug / unplug, channels 1-2 / 3-4, fallback: NOT TESTED, nothing plugged in (needs Marko with the Mix Ultra or Bluetooth headphones).
2. CPU ~80 % of one core with both decks playing, every tab.
3. Genre: Deezer finds nothing (4 of 4 misses); 1 of 5 genres overall.
Notes: CUE after a play-from-cue + pause stood 3 beats before the cued beat; EQ knobs' neutral drawn at ~4-5 o'clock; landscape: the song list shows ~3 rows, the Camelot wheel is small; Settings' ← scrolls away with the page; no knob values in the log for AUTOMIX.
Not tested: by ear (AUTOMIX "no hollow moment", jumps), the second output's 2 minutes.
The round ran in landscape (Marko's phone sideways, auto-rotate on). Rotation was not changed by the tester in the end.
