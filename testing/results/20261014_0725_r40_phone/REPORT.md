# Round 40, V41 (0fa0d39) on the Pixel 7 (41241FDH2S10SJ, Android 16), 14.10.2026

Tester: Claude Code on the Mac (hub session), adb over USB. APK DJMantra-V41-0fa0d39-phone-arm64.apk, versionName V41.

| part | check | result | notes |
|---|---|---|---|
| 0 | install, start | PASS | `Success`; app up in ~1 s |
| 0 | `Quick: interface ready` | PASS | `Quick: interface ready 411x914` |
| 0 | DJM filter loaded | PASS | `DJM filter: loaded on [Channel1]`, `... [Channel2]` |
| 0 | not black, no QML error | PASS | Songs tab drawn (Music, Download folders); no QML warning/error in logcat |
| 0 | load from Songs | PASS | tap -> `Quick: load [Channel1] ...Aum Ganesha...`, then `[Channel2] ...cura prox...` (Music folder, 8 songs 36:02, 100 %) |
| 0 | PLAY / pause each deck | PASS | `button [Channel1],play 1`; deck 2 `play armed (aligned start)` -> `beat launch [Channel2] on beat 8`; `play 0` both |
| 0 | audio engine | PASS | `Output rate` 48171-48212 of 48000, xruns 0, late 0, slowest callback 11-14 ms |
| 0 | CUE, SYNC | PASS | `cue press [Channel1] ... back to the cue point`; `sync_enabled 1` / `0` |
| 0 | knobs move | PASS | GAIN drag up/down both decks: pointer + dots follow |
| 0 | **EQ knobs at start** | FAIL? | right after start HIGH / MID / LOW on BOTH decks were drawn fully right (pointer at max, all dots lit) — the old "EQ at maximum" look. A double tap brought each to the middle. Either the stored/initial EQ value is max, or the drawing is wrong; not checked by ear |
| 0 | double tap resets | PASS | GAIN, HIGH, MID, LOW -> middle; volume fader -> top; crossfader -> middle |
| 0 | volume fader cuts sound? | ? | deck 1 alone: fader near bottom `Output level: peak 0.036`, fader top `0.041` (quiet part of the song). As in r39: the logged peak barely follows the fader — to check by ear |
| 0 | LEDs matrix | PASS | time left, bar.beat (4.4, 5.1, 7.3), countdown and the running lights move while playing (screenshots) |
| 0 | **BPM / key on load** | NOTE | library row says 114.6 BPM 10A and 118.0 BPM 2B; after loading, `sync [Channel1]: deck 1 90.07 bpm, deck 2 90.07 bpm` and deck 2 shows key **1B** (header `10A > 1B clash`). The deck's tempo differs from the library's |
| 1 | MIXER: no FILTER knob | PASS | each deck: GAIN, HIGH, MID, LOW only, FX button, headphones button |
| 1 | nothing overlaps, bigger | PASS | 411x914 portrait: knobs, labels, faders, FX/headphones buttons clear of each other, above the crossfader |
| 1 | double tap: knob middle, fader top | PASS | see part 0 rows |
| 2 | AutoMix START (Music, sorted BPM ▲, 8 beats) | PASS | `AutoMix: 1/8 Mantra - Ganesha Sharana -> [Channel1]`, `2/8 Gambara... -> [Channel2]`, `start, 8 songs, 8 beats`; crossfader went fully to deck 1; "mix in 111 s"; NEXT UP list in order |
| 2 | transition 1 (8 beats) | PASS | `mix ... over 8 beats (4.8 s)`; "Mixing into ... 1 % -> 85 %", crossfader travelled left -> right over the 8 beats, old deck ended and `3/8 Aum Ganesha -> [Channel1]` loaded (p2_automix_8beats_1fps.png, 1 frame/s). Beat alignment / flam and the bass swap: no log line, needs the ear |
| 2 | 32 BEATS chosen while running | PASS | `Settings: automix_beats = 32`; next `mix Gambara -> Aum Ganesha over 32 beats (26.5 s)` |
| 2 | switch tabs during a mix | PASS | went to Songs during the 32-beat mix: it went on, `4/8 cura prox -> [Channel2]` loaded (p2_automix_32beats_tabswitch.png) |
| 2 | STOP | PASS | `AutoMix: stop`, tab shows "Stopped", START AUTOMIX again |
| 2 | "The playlist has ended" | not tested | 8 songs = ~35 min; left for a short folder |
| 2 | layout | minor | while running, NEXT UP item 7 sits under the crossfader handle (the list is not clipped above the crossfader) |
| 2 | xruns during mixes | PASS | xruns 0, late 0 throughout |
| 3 | Songs header in matrix font | PASS | Music: "8 36:02" + "100%" green |
| 3 | analysis only in the open folder, 3 s after open | PASS | Partner Yoga (26 m4a, never opened before): `Quick: Analysis: folder .../Partner Yoga, 26 songs`, then `Analysis: 01. ... bpm 145.96 key 8A ...`, ~2 s a song; header 0 % -> 38 % |
| 3 | leave halfway: stops | PASS | at 15/26 `Analysis: stopped (the folder was left)` |
| 3 | come back: goes on | PASS | `Analysis: folder .../Partner Yoga, 11 songs`, continues with 16., 17., ...; `analysis in advance finished`; header 100 % green |
| 3 | **total time in the header** | FAIL | on opening Partner Yoga it showed "26 2:56:03", ~5 s later (when the analysis began) "26 0:00", and it stays 0:00 at 100 % (screens h2/h5) |
| 3 | note | NOTE | some analysed BPMs are doubled for this slow devotional music: 258.4, 208.x, 206.7, 184.6, 178.2 (`Analysis: 03. ... bpm 258.4000`) |
