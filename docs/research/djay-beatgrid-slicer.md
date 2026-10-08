# Running beat lights, phrase position and slicer jumps (djay-style) on Mixxx 2.5.6

Research and design report, 2026-10-08. No app code has been written. This file decides what to build.

**Goal (from the DJ).** In djay Pro, with the Hercules DJControl Mix Ultra in SLICER mode, the 8 pad LEDs
light one per beat and run 1→8. He uses them to:

1. line up the phrases of two songs by eye ("1 with 1, 8 with 8"),
2. see which beat a cue point sits on,
3. press PLAY on the paused deck exactly when the playing deck's light reaches that beat.

He wants this in DJ Mantra:
- beat lights **on by default in every pad mode, from app start**, on the controller and in the app, with a
  switch to turn them off;
- a pad press that **jumps to that beat** of the phrase, as slicer does;
- **the beat number shown on every cue point**.

How sure each claim is:
- **[confirmed]**: read in our source tree, in upstream Mixxx `main`, or in a primary page/snippet that is cited.
- **[snippet]**: confirmed only by search-engine excerpts of the page. The proxy blocks `help.algoriddim.com`,
  `community.algoriddim.com`, `virtualdj.com`, `bonedo.de`, `manual.mixxx.org` and `web.archive.org` (CONNECT 403),
  so those pages could not be opened directly.
- **[inferred]**: my reasoning, or the DJ's own description. It needs a test on the hardware or in djay.

---

## 1. djay Pro: what it does

### 1.1 Beat grid, downbeats, bars
- djay's grid is built from the **BPM plus the position of the first downbeat**. In the waveform a **yellow line
  marks the downbeat (beat 1) of every bar**, and lighter lines mark the beats in between. [snippet]
  <https://help.algoriddim.com/user-manual/djay-pro-mac/dj-tools/beatgrids-bpm-sync/beatgrids>,
  <https://help.algoriddim.com/user-manual/djay-ios/dj-tools/beatgrids-bpm-sync/beatgrids>
- The **Beatgrid editor** opens from the Tools bar or from the pencil icon on the waveform. It has four parts:
  **BPM, Downbeat, Anchor, Grid**. The help page's own example: if "1" falls on beat 3, set the downbeat by hand in
  the quick-access editor or the Beatgrid editor. BPM offers ×2, /2 and tap; after a tap, djay analyses the grid
  again. **Reset** goes back to djay's own analysis. [snippet]
  <https://help.algoriddim.com/user-manual/djay-pro-mac/dj-tools/beatgrids-bpm-sync/beatgrid-editor>,
  <https://help.algoriddim.com/user-manual/djay-ios/dj-tools/beatgrids-bpm-sync/beatgrid-editor>
- There are **Straight and Dynamic grids**. A Dynamic grid follows tempo changes ("the beatgrid follows the song"),
  and tempo-change detection can be turned off under Settings > Library. **Anchors** handle drift and sections that
  are off the grid; staff say an anchor can go anywhere. [snippet]
  <https://community.algoriddim.com/t/how-do-i-edit-downbeat-in-multiple-places-in-one-track-in-djay-pro/24424>,
  <https://community.algoriddim.com/t/bars-with-odd-numbers-of-beats-in/42982>
- Users report that the analysis **sometimes puts bar 1 two beats late even when the beats themselves are right**.
  That is the same kind of error we will hit, so a quick "set 1 here" matters. [snippet] (same thread, 42982)
- **There are no phrase markers or phrase countdowns.** Users keep asking for phrase analysis and a "beat
  countdown to the next cue". A third-party macOS panel reads djay's grid and shows beat / bar-in-phrase / phrase
  number with a 16- or 32-beat countdown. The waveform does show **bar numbers** ("at bar 121 …"). [snippet]
  <https://community.algoriddim.com/t/phrase-analysis-tool-energy-cue-countdowns/42806>,
  <https://community.algoriddim.com/t/can-we-have-phrase-analysis/23688?page=2>,
  <https://community.algoriddim.com/t/beat-countdown-to-next-cue-point/28162>,
  <https://community.algoriddim.com/t/accessibility-workflow-feature-requests/42496>
- Phase meter and sync: djay syncs tempo and beat phase. I found **no page that says djay's SYNC lines up bars or
  phrases**. [inferred: it lines up beats, like Mixxx; that the DJ lines up "1 with 1" by eye suggests sync does not
  do it for him.]

### 1.2 Slicer ("Slice")
- With Slice on, **the 8 pads split the section into 8 equal slices**. A pad plays its slice while held. On
  release, playback **jumps back to where the track would have been** (slip). The dropdown sets the repeat length
  (1/8 to 1 beat), **Quantize Jumps** on/off, and **4-beat or 8-beat slicing**. Slices can also be triggered from
  the waveform. [snippet] <https://help.algoriddim.com/user-manual/djay-pro-mac/dj-tools/cueing-looping/slice>
  (also the `djay-pro-windows`, `djay-android` and `djay-ios` versions of that page)
- Algoriddim support on the forum: "Toggle Slicer slices the **current bar (4 beats) into 8 slices**; when a slice
  pad is pressed, playback jumps to that beat". The MIDI-learn target is *Deck → Slicer → Toggle Slicer*. One user
  needed the Quantize setting before controller-triggered slices were quantized. [snippet]
  <https://community.algoriddim.com/t/slice-mode-how-to-automate-it-or-map-it-to-a-controller-button/13339>
- **Slice length.** With 8-beat slicing, one pad is one beat, which matches what the DJ sees (one light per beat).
  With 4-beat slicing, one pad is half a beat. [inferred from the two snippets]
- **How the window moves, and whether it is tied to the bar.** Neither is documented anywhere I could reach. The
  DJ sees "1" light on the bar/phrase start, which points to a **grid-aligned window** (8-beat sections counted
  from the downbeat) and not one that starts where Slice was switched on. [inferred: check in djay by switching
  Slice on in mid-bar and seeing whether pad 1 still lights on the downbeat.] Mixxx's own scripts do it the other
  way (§2.4).

### 1.3 Looper, Neural Mix, other pad modes
- **Looper**: 8 synced channels of short loops from genre packs, which follow the playing deck's tempo. Users
  report that Looper starts are sometimes not on the downbeat. PRO only. [snippet]
  <https://help.algoriddim.com/user-manual/djay-pro-mac/dj-tools/performance-tools/looper>,
  <https://community.algoriddim.com/t/looper-doesnt-start-stop-on-the-downbeat/41763>. It is not needed here.
- **Neural Mix**: stems (vocals / drums / instruments) in real time. On the Mix Ultra it has its own pad mode and
  the centre N button. [confirmed by our capture: pad base `30`, `90 01`]. Retail and review pages:
  <https://www.bonedo.de/artikel/hercules-djcontrol-mix-ultra-test/> [snippet: lists the modes, including Slicer
  and Bounce Loop, and says the pads light **only blue/cyan**, with no colour per mode].
- **Hercules Mix Ultra in djay.** No public page describes the slicer lights. The running 1→8 light is the
  **DJ's own observation** (primary source for this report). The VirtualDJ page for the Mix Ultra contradicts
  itself (FX = "PAD FX" vs "Slicer (led will blink)"). [snippet]
  <https://virtualdj.com/manuals/hardware/hercules/mixultra/controls.html>. Our capture settles the layout:
  `testing/results/2026-10-08_mix-ultra.md` [confirmed].

## 2. Mixxx 2.5.6: what exists

### 2.1 The beat model has no bar and no downbeat [confirmed]
- `src/track/beats.h`: `Beats` is a list of `BeatMarker{position, beatsTillNextMarker}` plus one closing
  (position, BPM) marker. It has **no time signature, no beats-per-bar, no downbeat flag and no bar index**. The
  comment "the last marker is positioned at the first downbeat" is only a wish.
- `src/proto/beats.proto`: `Beat{frame_position, enabled, source}`, `BeatGrid{bpm, first_beat}`. There is no bar
  information here either.
- `src/track/beatutils.cpp:309-319`: after analysis the first beat is moved **as close to the start of the track
  as it can go** (`fmod(firstBeat, beatLength)`). The comment says *"ideally the anchor point … should be the first
  proper downbeat, or perhaps the CUE point."* So **the grid's first beat has a correct beat phase but no bar
  meaning**. Counting "beat 1" from the grid origin is wrong 3 times in 4.
- **Upstream `mixxxdj/mixxx` main** (shallow clone `b02e84a`, 2026-10-08): `src/track`, `src/proto` and
  `src/engine/controls` still have **no downbeat or bar concept**. Nothing upstream will give us bars.
- **Imports drop bar information.** Rekordbox ANLZ beats carry `beat_number` (1–4)
  (`lib/rekordbox-metadata/rekordbox_anlz.h:237`), but `RekordboxFeature` keeps only the times
  (`src/library/rekordbox/rekordboxfeature.cpp:896-920`). Serato beatgrid markers store `beatsTillNextMarker`
  only. The rekordbox downbeat is **free data that we can use to seed the anchor**.
- `CueType::Beat = 3` is marked "unused (what is this for?)" (`src/track/cueinfo.h:15`). The engine ignores it
  (`cuecontrol.cpp:677`) and the Serato export ignores it (`src/track/serato/tags.cpp` handles only HotCue and
  Loop). That makes it **a safe, already-persisted slot for a per-track bar anchor**.

### 2.2 Controls (COs) to build on [confirmed in source]
| Control | Where | Meaning |
|---|---|---|
| `beat_active` | `clockcontrol.cpp` | 1 at a beat while playing forward, 2 when reversing; 0 after 20 % of the beat. It handles loops. Updated in `EngineBuffer::updateIndicators` (`enginebuffer.cpp:1506`) |
| `beat_distance` | `bpmcontrol.cpp:1244` | 0..1 fraction since the previous beat (minus the sync user offset) |
| `beat_prev` / `beat_next` / `beat_closest` | `quantizecontrol.cpp` | beat positions in engine samples |
| `playposition`, `track_samples`, `track_samplerate`, `duration`, `bpm`, `local_bpm`, `file_bpm` | | position and tempo |
| `beatjump` (any ±n, including fractions), `beatjump_size`, `beatjump_forward/backward` | `loopingcontrol.cpp:1738` | `findNBeatsFromPosition` + `seekExact`, so an integer n **keeps the sub-beat phase**. **Inside an active loop it moves the loop instead** (`slotLoopMove`) |
| `beats_translate_curpos`, `beats_translate_earlier/later`, `beats_translate_match_alignment`, `beats_set_*`, `beats_undo_adjustment` | `bpmcontrol.cpp` | grid editing |
| `hotcue_N_position`, `hotcue_N_status`, `cue_point` | `cuecontrol.cpp:2485`, `:110` | cue positions in engine samples |
| `quantize` + `play` | `enginebuffer.cpp:776-787` | with quantize on, pressing play calls `requestSyncPhase()` → `BpmControl::getNearestPositionInPhase` (`bpmcontrol.cpp:707`). That **snaps the starting deck to the other deck's beat fraction, within ±1 beat**: beats line up, bars do not |

The control reference is <https://manual.mixxx.org/2.5/en/chapters/appendix/mixxx_controls> (blocked here; the
names above were checked in the source).

Useful API: `Beats::iteratorFrom(pos)` and `ConstIterator::operator-(other)` give **the number of beats between
two positions** in O(markers), and that works for beat maps with varying tempo too. Avoid
`Beats::numBeatsInRange`: it calls `findNthBeat` in a loop (O(n²) walk, `beats.cpp:738`).

### 2.3 What the waveform and QML show [confirmed]
`src/waveform/renderers/waveformrenderbeat.cpp` draws every beat the same way. There are no bar or phrase lines,
in 2.5.6 or upstream. The QML UI (`res/qml/WaveformHotcue.qml`, `Hotcue.qml`) shows a hotcue's number and label
only. `res/qml/djmantra/` does not exist yet (M6).

### 2.4 Slicer and beat counters in existing Mixxx mappings [confirmed, read in `res/controllers`]
| Mapping | How it finds the beat in 8 | Problem for us |
|---|---|---|
| Hercules Inpulse 500 (`Hercules-DJControl-Inpulse-500-script.js:1486`) | `floor(playposition*duration*file_bpm/60) % domain`, run on `beat_active`. Pad = `beatjump(index*domain/8 - beatsPassed%domain - beat_distance)` with slip on and a JS timer to end the slip | Counts from **time 0**, not from the grid. Wrong whenever the first beat ≠ 0 s; constant BPM only. The timers are fragile |
| Hercules Inpulse 300 (`…-Inpulse-300-script.js:484-640`) | Window = 8 slices of `beatloop_size`, anchored at **`beat_closest` when slicer mode is switched on**. It moves forward when the position passes slice 8. Driven by `beat_distance` (the file itself warns that this is "resource intensive") | The window depends on **when you pressed the mode button**, not on the bar. Pads build loops |
| Denon MC7000 (`Denon-MC7000-scripts.js:1111-1161`, "experimental") | `floor((playPos - cuePos) * bpm/60) % 8`, counted **from the main CUE**, via `playposition` | The anchor is the main cue (a good idea); the arithmetic uses time and constant BPM |
| Traktor S4 MK3 screens (`TraktorKontrolS4MK3Screens/.../DeckInfo.qml:130-224`) | `bar.beat` = elapsed time × BPM, with `propGridOffset` **stubbed to 0** | No grid offset, so the counter is off |
| Pioneer DDJ-SX, Numark NS4FX, Roland DJ-505 (TODO) | as the Inpulse 500 (`domain` 8/16/32/64) | same |

The Hercules family shares one pad-note scheme (`0x96`/`0x97`, base + pad). On the Inpulse the velocity picks the
LED colour (`0x03`, `0x62`, `0x7F`); on the Mix Ultra it does not (cyan for every value) [confirmed: our LED test].

**Conclusion.** No Mixxx mapping computes "beat in bar" correctly from the beat grid. Each one copies its own
time × BPM arithmetic, and none has a downbeat. We should do the counting **once, in C++, from the grid**, and give
scripts and the UI ready-made values.

## 3. Design

### 3.1 C++ `PhraseControl` per deck (new `EngineControl`)
**Where.** `src/engine/controls/phrasecontrol.{h,cpp}`, created in `EngineBuffer` next to `ClockControl`
(`enginebuffer.cpp:229`). Its update is called from the same `updateIndicators` hook (`enginebuffer.cpp:1506`), so
it runs once per audio callback, in the engine thread, with no locks. `trackLoaded` / `trackBeatsUpdated` keep the
`BeatsPointer` (immutable and shared, as in `ClockControl`).

**Computation**
```
anchorBeat  = beats.iteratorFrom(snapToClosestBeat(anchorPos))  // see the anchor rules below
curBeatIt   = beats.iteratorFrom(prevBeat(currentPos))           // recomputed only when crossing beat_prev/next
k           = curBeatIt - anchorBeat                             // integer, may be negative (intro before "1")
beatInPhrase = floorMod(k, phraseLength)                         // 0..N-1, wraps correctly for k < 0
beatInBar    = floorMod(k, beatsPerBar); barInPhrase = floorMod(floorDiv(k, beatsPerBar), N/beatsPerBar)
phraseIndex  = floorDiv(k, phraseLength)
```
These are integers on beat boundaries. `beat_distance` already supplies the fraction within the beat. The cost is
one iterator subtraction per beat.

**Anchor ("where is 1"): order of precedence**, stored per track:
1. **User anchor**: "Set 1 here" in the UI, or SHIFT + slicer pad N on the controller (meaning "this beat is N").
   Stored as a **`CueType::Beat` cue** in the existing `cues` table (persisted, ignored by the engine and the Serato
   export; §2.1). It could later go to an optional proto field.
2. **Imported downbeat**: the first rekordbox beat with `beat_number == 1`, saved as the same Beat cue when the
   track is imported. Serato: none.
3. **Main cue** (`cue_point`), when it lies within ±¼ beat of a beat. Mixxx usually puts it on the first sound,
   which is usually beat 1 of the intro. The Denon MC7000 mapping also counts from it.
4. **The grid's first beat.** The phase is right but the bar is a guess. The app marks it "auto" so the DJ knows.

The anchor is kept as a frame position and **snapped to the closest beat every time it is used**. A grid that is
moved by less than half a beat (`beats_translate_*`), or halved or doubled, therefore keeps its "1" on the same
musical beat.

**Controls** (group `[ChannelN]`; names start with `djmantra_` so they cannot clash with upstream ones):
| Control | R/W | Value |
|---|---|---|
| `djmantra_beat_in_phrase` | R | 0..N-1; **-1 = no grid / no track** |
| `djmantra_beat_in_bar`, `djmantra_bar_in_phrase`, `djmantra_phrase_index` | R | as above |
| `djmantra_phrase_length` | RW | 4 / **8** / 16 / 32 (default 8, saved in `[DJMantra] PhraseLength`) |
| `djmantra_beats_per_bar` | RW | **4**, or 3 (per track, kept in the Beat cue's label or comment) |
| `djmantra_phrase_anchor` | RW | anchor in engine samples, -1 = auto |
| `djmantra_phrase_anchor_source` | R | 0 none, 1 grid, 2 main cue, 3 imported, 4 user |
| `djmantra_phrase_set_beat` | W | n (1..N): "the current beat is beat n". Moves the anchor and saves it as a user anchor |
| `djmantra_phrase_jump` | W | n (1..N): jump to beat n of the **current** phrase (§3.2.3) |
| `djmantra_phrase_slice_n` (n = 1..8) | W, momentary | as a jump, but with slip while held (djay "Slice") |
| `djmantra_hotcue_N_phrase_beat`, `djmantra_cue_phrase_beat` | R | 1-based beat of that cue (double: 3.0 on the beat, 3.5 half way); 0 = not set |
| `djmantra_play_on_phrase` | W | arms "start when the other deck reaches my beat" (§3.2.4) |
| `[DJMantra],beat_lights` | RW | **1** (default) / 0. One switch for the controller and the UI; `beat_lights_ui` overrides it for the UI |
| `[DJMantra],beat_lights_latency_ms` | RW | compensation for the lights (§4) |

Values are set only when they change. A script connection then fires once per beat, not on every callback, which
avoids the Inpulse 300's "connected to beat_distance" cost.

### 3.2 Mix Ultra mapping (`res/controllers/Hercules-DJControl-Mix-Ultra-script.js`)

#### 3.2.1 Tracking the firmware's pad mode [confirmed facts, inferred design]
- The firmware switches the pad mode on its own and lights the mode buttons; the host cannot change those LEDs.
  Pads light **only for notes of the active mode** (the LED test lit `96 30–37` while in NEURAL MIX; other bases
  did nothing).
- So the script keeps `mode[deck]` up to date from two sources:
  - mode-button notes on `91`/`92`: `0F` HOT CUE, `10` LOOP, `11` FX, `12` NEURAL, and with SHIFT `13` PITCH,
    `14` BOUNCE, `15` SLICER, `16` SAMPLER (mode index 0..7 = note - `0F`; pad base = index × `0x10`);
  - **every pad press**, since its note gives the base (`note & 0x70`) and so tells us the mode for certain.
- At startup we assume HOT CUE: the capture shows "after power-on the firmware is in HOT CUE mode". The firmware
  sends nothing on connect and has no mode query. If the app restarts while the controller stays on, the first
  mode-button or pad press corrects it. Until then we send to the HOT CUE base, and **optionally to all 8 bases**,
  which costs 8× MIDI traffic (open question Q3).

#### 3.2.2 Drawing the running light in every mode
- `engine.makeConnection(group, "djmantra_beat_in_phrase", …)`: on change, send `0x96+deck, base(mode)+old, 0x00`
  and `…+new, 0x7F`. That is 2 messages per beat per deck (about 6 bytes/beat over BLE, which is negligible). When
  the mode changes, redraw all 8 pads of the new base, because the firmware may have repainted them.
- **Pads per beat.** Phrase 8 → one pad per beat. Phrase 16 → 2 beats per pad. Phrase 32 → one pad per bar (the
  pad flickers on each beat in the bar). Phrase 4 → pads 1–4, with 5–8 showing the next bar dimmed if brightness
  turns out to work (Q2).
- **3/4 tracks** (`beats_per_bar = 3`): the phrase becomes 6 (2 bars) and pads 7 and 8 stay dark.
- **Paused deck**: its current beat stays lit. That is the "target pad" for the DJ's PLAY timing. An option makes
  it blink.
- **Clash with what each mode shows.** Colour is fixed per mode (cyan in Neural Mix) and velocity does not change
  it, so there is a single "on" state to share. Rules:
  - **Beat lights on (default)**: the beat light is drawn over the mode state. HOT CUE: pads with a hotcue are lit
    and the beat pad is **inverted** (a lit hotcue pad goes dark on its beat, a dark pad lights), so the run stays
    visible.
  - Other modes: only the beat pad is lit.
  - While a pad is held, it shows press feedback.
  - **Beat lights off**: the plain mode mapping.
  - Whether brightness levels exist is to be tested (Q2). If they do: dim = mode state, full = beat.

#### 3.2.3 Pads jump to a beat
- **SLICER mode (`60`–`67`)**: pad n → `djmantra_phrase_slice_n` (press/release), which works like djay:
  - Playing: jump to beat n of the current phrase now, with an integer beat offset so the sub-beat phase is kept
    (a quantized jump with no wait). Slip while held; on release, back to where the track would have been. Option
    "Slicer: jump and stay" turns slip off.
  - Paused: move to the **exact** beat n (fraction 0), so the DJ can set "start here on 5".
  - Inside an active loop: if beat n is inside the loop, seek there; otherwise leave the loop and jump. This is done
    in C++, because `beatjump` moves the loop instead (§2.2).
  - It is done in C++ so that no JS timers are needed (the Inpulse 500 needs `beginTimer` and a `reloop_toggle`
    dance).
- **SHIFT + slicer pad n (`68`–`6F`)** → `djmantra_phrase_set_beat = n`: "the beat playing now is n".
- **Other modes** keep their own meaning (HOT CUE stays hotcues). An option "Pads jump to a beat in every mode"
  is off, because it would take away hotcues, loops and FX.

#### 3.2.4 Helpers for "press play on beat N"
1. **Visual (no automation; matches what djay does today)**: deck A's pads run, and paused deck B's pads show B's
   beat. Press B's PLAY when A's light reaches that pad. With `quantize` on (make it the default), Mixxx's
   phase-snap on play (§2.2) corrects a press that is up to about ½ beat early or late. Being right to the bar is
   the DJ's job.
2. **Ghost light (option)**: on the **paused** deck's pads, also show the *playing* deck's running beat, blinking,
   next to B's own steady target pad. The DJ watches one pad row.
3. **Phrase-quantized start (option, off)**: SHIFT + PLAY or `djmantra_play_on_phrase` arms the deck. B then starts
   on the next beat where `beat_in_phrase(A) == beat_in_phrase(B)` (at most N beats of waiting). The waiting pad
   blinks.
4. **Phrase-aware quantize (option, off)**: extend `getNearestPositionInPhase` so play/sync also moves B by whole
   beats (±N/2) to match A's beat in the phrase. It is off by default because it shifts the DJ's chosen cue by
   beats.

### 3.3 App UI (QML, `res/qml/djmantra/`)
- **Beat strip per deck**: 8 cells (N cells, grouped by bar) under the waveform, on the same controls. The current
  cell is lit, cell 1 is accented, a cell fills over the beat using `beat_distance`. "—" means no grid. The anchor
  source is shown ("1 = auto / cue / rekordbox / you"), with a **"Set 1 here"** button and a long-press menu
  (phrase 4/8/16/32, 4/4 or 3/4).
- **Cue markers**: a badge with the cue's beat number in the phrase ("Cue 2 · 5", or "5½" when off the beat), read
  from `djmantra_hotcue_N_phrase_beat`. The same number appears on the hotcue pad buttons in the app.
- **Waveform**: a strong line at every bar start, a stronger line with the phrase number at every phrase start.
  This is a new QML waveform renderer layer driven by the same anchor; `waveformrenderbeat` itself stays unchanged.
- Setting **Settings → Decks → Beat lights: On (controller + app) / App only / Off**, on by default from the
  first start.
- Look: our own design, no djay artwork (rule in `docs/ANDROID_PORT.md` §4).

## 4. Edge cases
| Case | Behaviour |
|---|---|
| **Tempo changes / beat maps** | Beats are counted on the grid (iterator difference), not as time × BPM, so variable-tempo maps and pitch/tempo-fader changes count correctly. The Inpulse/Denon arithmetic fails here |
| **Grid moved** (`beats_translate_*`, `…_match_alignment`) | The anchor snaps to the closest beat when used, so moves under ½ beat keep "1". A larger move can change the count; the strip then shows the new count at once, and "Set 1 here" fixes it |
| **BPM halved/doubled** (`beats_set_*`) | The anchor stays on the same audio beat. At double tempo the "phrase 8" is half as long in time; a ×2/÷2 hint could be shown |
| **No grid / analysis pending** | All `djmantra_*` beat controls are -1, pads show only the mode state, the strip shows "—". It turns on when `trackBeatsUpdated` arrives |
| **Before the anchor** (intro) | `floorMod` keeps the count running backwards (…7, 8, **1**), so lead-in beats show the right numbers |
| **Loops** | The beat follows the position, so a 4-beat loop from beat 5 shows 5-6-7-8-5-…, which is musically right. Loops shorter than a beat hold one pad. Loop-roll and slip: the lights follow the audible position; slip's hidden position is not shown |
| **Reverse / scratch / jog** | Position-based, so the lights run backwards. When paused and jogging, the lit pad follows |
| **Keylock / pitch** | No effect: position-based |
| **Sync** | Mixxx sync lines up beats only. Bars can be off by k beats, which is exactly what the lights let the DJ see. Phrase-aware sync is optional (§3.2.4) |
| **3/4 and odd meters** | `beats_per_bar = 3`: phrase 6 or 12, pads 7–8 dark. A bar of odd length inside a 4/4 track (djay thread 42982) needs a second anchor. v1 has one anchor per track; several anchors are a later extension (several Beat cues; the nearest earlier one wins) |
| **Output latency** | `djmantra_beat_in_phrase` changes when the *engine* crosses a beat, which is ahead of what is heard by the buffer, plus up to ~150–250 ms on a Bluetooth speaker (Main delay, ANDROID_PORT §1), plus ~10–15 ms BLE-MIDI to the controller. For "press play on the light", the light must be on time with what the DJ **hears from the master**. So a `beat_lights_latency_ms` setting (default = output latency + Main delay) delays the edge in the controller script or in PhraseControl. Do the same for the strip in the UI, or reuse Mixxx's `VisualPlayPosition`, which already compensates for latency |
| **Two decks, one deck** | Each deck's pads show their own deck. 4 decks: not on the Mix Ultra |
| **Track load** | Reset to -1, then recompute. Pads are redrawn |

## 5. Plan (once M5 MIDI works)
1. `PhraseControl` + COs + unit tests: grid with an offset first beat, beat map, negative k, loop, translate,
   halve/double, 3/4 (`src/test/phrasecontrol_test.cpp`). Run the desktop suite (CI rule).
2. Anchor persistence (Beat cue) + rekordbox `beat_number` import + main-cue fallback.
3. Mix Ultra script: mode tracking, running lights, slicer jump/slice, SHIFT = set beat. Test per mode on the phone
   (`/test-on-phone`).
4. QML strip, cue badges, bar/phrase lines, settings.
5. Latency compensation, measured with the Bluetooth speaker.

## 6. Open questions
- **Q1 (djay)**: is djay's slicer window tied to the bar/downbeat, or to the moment Slice was switched on? Does it
  move on every 8 beats or only on demand? Are the running lights shown in modes other than SLICER? Ask the DJ for
  a 20 s video in djay: switch Slice on in mid-bar, with a track whose "1" is known.
- **Q2 (hardware)**: per mode, do the pads accept host LED notes (only NEURAL MIX was tested)? Does any velocity
  give a dimmer level? Does the firmware draw its own pad LEDs in some modes (HOT CUE "pad 1 lit" after power-on),
  and would it overwrite ours? Do SHIFT notes (+8) light anything? The capture marked this "to be checked per mode".
  Re-run the LED test in all 8 modes.
- **Q3**: can we read the firmware's current mode at connect (SysEx?), or must we guess HOT CUE / send to all bases?
- **Q4**: on the LED test, `91 10`–`12` (mode-button notes) lit pads 2–4. Is that a camera ROI mix-up or a real
  mapping? It affects whether mode-button notes are safe to send.
- **Q5**: default slicer behaviour: djay-style slip ("return on release") or "jump and stay"? The DJ described
  "jumps to that beat", which suggests he may want it to stay.
- **Q6**: should a pad jump also work in other modes with a modifier (e.g. SHIFT + pad in HOT CUE)? SHIFT + pads are
  already taken in some modes.
- **Q7**: default phrase length: 8, which matches djay's 8-beat slicing and his "1 with 1, 8 with 8". Should 16 or 32
  be offered as an 8-pad display (2 or 4 beats per pad)?
- **Q8**: should the beat lights follow master latency or headphone latency? The DJ cues with headphones, but times
  PLAY against the master.
