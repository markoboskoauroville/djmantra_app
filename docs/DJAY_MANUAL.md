# djay for Android: user manual reference

A reference for the DJ Mantra team, taken from the official Algoriddim manual "djay for Android"
(https://help.algoriddim.com/user-manual/djay-android), read on 14.10.2026.

How to use it:
- Every statement below comes from the manual. Menu and setting names are verbatim, in **bold**.
- **[Vague]** marks places where the manual does not give a value, default or behaviour. Those gaps
  were left open on purpose, not filled from memory or from other platforms' manuals.
- **[Conflict]** marks places where two manual pages disagree. Both versions are quoted.
- The manual is written for "djay Pro for Android" and also calls the app "djay" and "djay Pro".
- Screenshots were not readable as text. Only their alt captions were available, so anything shown
  only in an image is missing here.

---

## 1. Pages read

The manual is a Docusaurus site. Its sitemap (`https://help.algoriddim.com/sitemap.xml`) lists 94
pages under `/user-manual/djay-android`. Every one was downloaded and read twice. Every in-manual
navigation link on those pages is among those 94 pages; none was missing. All URLs start with
`https://help.algoriddim.com/user-manual/djay-android`. The list shows only the path after it.

| Section | Pages (path suffix) |
|---|---|
| Home | `` (welcome page) |
| Getting started | `/getting-started`, `/getting-started/requirements`, `/getting-started/installation`, `/getting-started/launching`, `/getting-started/interface`, `/getting-started/dashboard`, `/getting-started/decks`, `/getting-started/audio`, `/getting-started/account`, `/getting-started/subscription`, `/getting-started/glossary` |
| Mixing basics | `/mixing-basics`, `/mixing-basics/loading`, `/mixing-basics/mixing-tracks`, `/mixing-basics/crossfader`, `/mixing-basics/mixer`, `/mixing-basics/audio-settings`, `/mixing-basics/waveforms`, `/mixing-basics/waveform-options`, `/mixing-basics/zoom`, `/mixing-basics/setting-cues`, `/mixing-basics/match`, `/mixing-basics/automix`, `/mixing-basics/using-automix`, `/mixing-basics/recording`, `/mixing-basics/dj-tools` |
| Creative DJ tools: Beatgrids, BPM, and sync | `/dj-tools/beatgrids-bpm-sync`, `/dj-tools/beatgrids-bpm-sync/beatgrids`, `/dj-tools/beatgrids-bpm-sync/adjusting-bpm`, `/dj-tools/beatgrids-bpm-sync/beatgrid-editor`, `/dj-tools/beatgrids-bpm-sync/sync` |
| Creative DJ tools: Cueing and looping | `/dj-tools/cueing-looping`, `/dj-tools/cueing-looping/cues-points`, `/dj-tools/cueing-looping/cue-loops`, `/dj-tools/cueing-looping/loops`, `/dj-tools/cueing-looping/pitch-cue`, `/dj-tools/cueing-looping/skip`, `/dj-tools/cueing-looping/slice` |
| Creative DJ tools: Effects | `/dj-tools/effects`, `/dj-tools/effects/overview`, `/dj-tools/effects/effects-section`, `/dj-tools/effects/audio-effects`, `/dj-tools/effects/essential-tools`, `/dj-tools/effects/crossfader-fx` |
| Creative DJ tools: Key lock | `/dj-tools/key-lock`, `/dj-tools/key-lock/key-lock` |
| Creative DJ tools: Performance tools | `/dj-tools/performance-tools`, `/dj-tools/performance-tools/sampler`, `/dj-tools/performance-tools/sequencer`, `/dj-tools/performance-tools/sample-packs`, `/dj-tools/performance-tools/customizing-samples`, `/dj-tools/performance-tools/recording-samples`, `/dj-tools/performance-tools/looper`, `/dj-tools/performance-tools/building-loops` |
| Neural Mix™ | `/neural-mix`, `/neural-mix/overview`, `/neural-mix/tracks`, `/neural-mix/eq-crossfaders`, `/neural-mix/fx`, `/neural-mix/mute-fx` |
| Managing your music library | `/music-library`, `/music-library/navigating`, `/music-library/searching`, `/music-library/files`, `/music-library/djay-music`, `/music-library/streaming-services`, `/music-library/advanced`, `/music-library/advanced/analyze`, `/music-library/advanced/sorting`, `/music-library/advanced/queue`, `/music-library/advanced/history` |
| Connecting hardware | `/hardware`, `/hardware/connecting`, `/hardware/audio-interfaces` |
| Customization and settings | `/settings`, `/settings/general`, `/settings/sound`, `/settings/library`, `/settings/automix`, `/settings/advanced`, `/settings/midi` |
| Pro tips and troubleshooting | `/tips`, `/tips/audio-file-quality`, `/tips/audio-levels`, `/tips/cables`, `/tips/know-your-tracks`, `/tips/clean-library`, `/tips/mixing-in-key`, `/tips/tutorials`, `/tips/support`, `/tips/canceling`, `/tips/deleting`, `/tips/deleting-djay-account` |

Some pages outside the manual tree are linked from it. These were also read, and their facts are
marked "(linked page)":
- https://help.algoriddim.com/topic/first-steps/audio-setup?os=android (Android tab: audio setup and pre-cueing)
- https://help.algoriddim.com/topic/first-steps/free-vs-pro (what PRO unlocks)
- https://help.algoriddim.com/topic/music-streaming/streaming-services
- https://www.algoriddim.com/hardware/precueing
- https://help.algoriddim.com/user-manual/djay-ios/midi/mapping. The Android Key lock page links
  to the **iOS** MIDI mapping page; the Android manual has no MIDI mapping page of its own.

Not followed: the store, streaming-signup and social links, and the DJ Gear controller list
(https://www.algoriddim.com/hardware). That list is a product catalogue, not manual text.

---

## 2. App structure, views and interface

### System, install, account
- Requires an Android phone or tablet on **Android 10** or later.
- The app is a free download from the Google Play Store; a PRO subscription unlocks advanced features.
  With djay 5.0 and later, PRO can be used on up to **five devices** (Android, macOS, iPhone, iPad,
  Windows) when signed in with a djay account.
- Account: **Dashboard button** > **Settings** > **My djay** > **Sign in** under **My djay account**.
  An account is not required for single-platform use. It does not restore purchases or sync the
  music library between devices.
- Restore purchases: the **Restore purchases** button, or **Settings** > **My djay**.
- Delete account: **Dashboard** > **Settings** > **My djay** > dropdown next to the account >
  **Delete Account**.
- Features that need PRO, according to the manual: Pre-cueing, Sampler, Looper. The linked free-vs-pro
  page also lists Neural Mix™, Automix, Crossfader FX, Recording, content packs, MIDI Learn,
  playlists and batch analysis as PRO features.

### Launch and deck views
- On first launch the app shows the **Starter** layout: two decks, a crossfader, "all the features
  you need to start mixing".
- **Deck views** (Dashboard > View Modes menu): **Starter**, **Classic**, **Automix**.
  - **Starter**: a simplified interface. When the crossfader is used, the tracks are
    **automatically tempo-blended**: djay syncs the tempo between the decks as the crossfader moves.
  - **Classic**: "The default Classic view displays the decks as virtual turntables."
    **[Conflict]** The launching page says the first-launch layout is Starter, while this page calls
    Classic "the default" view.
  - **Automix**: automates transitions (see section 8).
- **Tools** (cue points, loops, FX, Neural Mix, slice and so on) need **Classic mode** ("Make sure
  you're in Classic mode to access Tools"). The Sampler/Looper icon is also reached "in Classic mode".
- The layout adapts to device orientation. Landscape gives more space and "mimics the standard DJ layout".

### Dashboard (Dashboard button, main menu)
Shortcuts in the menu:
- **Deck views**: Starter / Classic / Automix.
- **Audio recording**: start a mix recording.
- **Settings**: all settings. The settings tabs named in the manual are General, Sound, Library,
  Automix, Advanced, MIDI devices, My djay and djay School, plus Main volume and Split Output.
- **MIDI**: quick access to controller connection settings.
- **Light/Dark mode**: toggles light and dark mode.

### Interface elements (Classic mode, from "Understanding the interface")
- **Track load**: tap to add a track to the deck.
- **Deck 1** (left) and **Deck 2** (right): digital turntables, or hardware.
- **Dashboard button**: opens the main menu (Deck modes, Mix Recorder, Automix, and more).
- **Track information**: shows the track's **key** and its **elapsed or remaining time**. *Tap to toggle
  between different time views.*
- **Waveform overview**: the whole track, colour-coded by frequency. It shows saved cue points, loops
  and cue loops.
- **Cover art display**: the artwork of the loaded track.
- **Sync button**: "Tap to match the BPM of the current track to the one playing on the opposite deck."
- **BPM**: shows the tempo of the loaded track.
- **Tools**: opens the creative controls (Neural Mix™, cue points, loops, audio effects, and more).
- **Play button**: play or pause for each deck.
- **Cue set**: sets a temporary cue point at the current position of the **red play marker**.
- **Cue jump**: returns playback to the set cue point. *If held, it plays from the cue point.* If no
  cue point is set, it returns to the start of the track.
- **Essential Tools**: two FX channels next to each deck. The dropdown switches between Neural Mix™,
  FX and Beat Skip.
- **Crossfader**: left plays the left deck, right plays the right deck, centre mixes both.
- **Crossfader FX**: transition effects applied while blending.
- **Tempo slider**: move it up or down to slow down or speed up playback. **[Vague]** The manual does
  not say which direction is faster by default. "Invert slider" exists (see section 7).

---

## 3. Loading tracks

- Load to **Deck 1**: tap the **music note icon** on the left side, next to **Tap to load**, then choose
  a track. Repeat for **Deck 2**.
- **The track starts playing automatically when loaded.** To stop this: **Settings** > **General** >
  turn off **Start Playback**. The setting is **ON by default**, by implication; the manual gives no
  explicit default value.
- **Track options**: *press and hold the track artwork* next to the waveform of a loaded track:
  - **Double**: loads the same track on both decks.
  - **Load previous / load next**: the previous or next track in the current playlist.
  - **Scratch tools**: loads a scratch-ready track from the built-in scratch tools. Tip: *tap and hold
    load track* on an empty deck opens Scratch Tools directly.
  - **Add to Queue**.
  - **Eject**: unloads the track.

---

## 4. Decks, waveforms and zoom

### Waveform colours and markers
- Frequency colours: **Red = Bass**, **Yellow = Low-mid**, **Green = High-mid**, **Blue = High
  frequencies**.
- **Waveform overview**: the entire track from start to finish. *Tap or swipe across the waveform to
  skip to a specific section of the track.*
- **Scrolling waveform**: a real-time, zoomed-in view. It "responds instantly to actions like scratching,
  scrubbing, or jumping to a cue point". **Each bar is numbered at the start.** A **vertical red play
  marker** shows the playback position.
- Downbeat or bar marking. **[Conflict]**
  - Beatgrids page: "the **yellow vertical line** shows the downbeat of each bar, while the **lighter
    vertical lines** indicate the beats in between."
  - Mixing tracks page: "**bar markers** (yellow vertical lines in the waveform)".
  - Scrolling waveform page: "downbeats are marked with a **solid white line** in the beatgrid".
- Automix view: a **white line** at the beginning of the waveform marks the Automix entry region, and
  a **white line** at the end marks the exit region.
- Cue points appear on the waveform as coloured markers.

### Waveform options
- Open them with the **dropdown menu in the corner of the waveform next to the deck number**. Waveforms
  must be displayed.
  - **High Contrast**: increases waveform visibility by boosting contrast against the background.
  - **Zoom**: the level of detail.

### Zoom
- **Five levels** of zoom for each waveform.
- In the waveform dropdown: **+** zooms in, **–** zooms out, and the **Reset button** returns to the
  default zoom level.
- **[Vague]** Which of the five levels is the default is not stated. The manual mentions no pinch-zoom
  gesture.

### Nudging and manual beatmatching
- "Hover over the waveform or deck on Deck 2 and **scroll gently** left or right to nudge the track:
  Scroll left to slow it down. Scroll right to speed it up." With a controller, nudge the jog wheel.
  **[Vague]** The wording ("hover", "scroll") is desktop-style. The manual does not define the touch
  gesture.
- Tip: start beatmatching practice with tracks within **2–5 BPM** of each other.

---

## 5. Mixer: EQ, filter, gain, volume, crossfader

### Mixer overview
- The **Mixer** button opens the mixer.
- The **EQ** button reveals the low, mid and high EQ. **Without the EQ button the mixer is simplified
  to filters, gain, and volume faders only.** The Neural Mix overview says the same: when EQ options
  are not active, it shows "volume faders, gain knobs, and a single filter".
- **Low EQ**: bass and kick drums. **Mid EQ**: guitars, synths, keys, vocals. **High EQ**: hi-hats,
  cymbals, percussive elements.
- **Volume fader**: the output volume of each deck.
- **Filter**: one knob that "lets you apply sweeping effects". The resonance is set in **Settings** >
  **Sound**. **[Vague]** The manual does not say which direction is low-pass or high-pass, and does not
  mention a centre detent or the cut-off range.
- **Gain**: the input level for each deck, used to balance tracks produced at different levels.
- **EQ and Neural Mix™ dropdown**: switches between **classic EQ**, **Neural Mix™**, and **Neural Mix™
  crossfaders**. When Neural Mix is selected, the EQ knobs change what they control.
  **[Conflict]** This page gives Low = Drums, Mid = Bass, High = Melodic. The Neural Mix pages give
  Low = Drums, Mid = Harmonics, High = Vocals. The Sound settings page explains that **both layouts
  exist** and are selectable (see below).
- "When switching between EQ and Neural Mix™ EQ, the EQ parameters will **reset back to zero**."

### Sound settings > Mixer (exact options)
- **Crossfader curve**: **Default**, **Linear**, **Constant Power**, **Dipped**, **Cut**. **[Vague]** The
  manual does not describe what each curve's shape is, nor what "Default" means.
- **EQ Type**: **Classic** or **Isolator**.
  - **Classic EQ**: high, mid and low have a range of **−24 dB to +6 dB** at **3,300 Hz / 1,000 Hz /
    264 Hz**, "similar to a standard club mixer". There is **no full kill**: "If all three are set to 0,
    some sound will still be heard at lower volume." Here "set to 0" means turned fully down.
  - **Isolator EQ**: **full kill to +6 dB**. Bands: low **275 Hz**, mid **275 Hz–5000 Hz**, high
    **5000 Hz+**. Turning the bass fully down cuts those frequencies completely.
  - **[Vague]** The default EQ type is not stated.
- **Neural Mix™ EQ**: the stem layout for the EQ knobs, either **Drums/Harmonic/Vocals** or
  **Drums/Bass/Melodic**.
- **Filter resonance**: **Low / Medium / High**. Lower sounds smoother; higher "adds character".
  The default is not stated.
- **FX routing**: **Pre-fader** (effects stay active when the channel fader is lowered) or
  **Post-fader** (effects fade out with the track as the fader moves). The default is not stated.

### Sound settings > Volume
- **Auto gain**: when a track is loaded, the channel gain is set automatically to match the mix.
  Disable it to set the gain by hand.
- **Unlink controller gain from on-screen gain**: the controller's gain knobs work independently from
  the on-screen gain. This is useful with Auto gain, because it avoids sudden jumps in software gain.

### Sound settings > Audio output
- **Mono output**: identical mono audio on both left and right channels.
- **Audio limiter**: stops clipping and distortion. The Tips page calls it "**Auto Limiter**".
- **Output headroom**: reduces the main output headroom by **0 (disabled), −6 dB, −9 dB, −12 dB, or
  −24 dB**.

### Crossfader
- Left plays the left deck, right plays the right deck, the centre mixes both.
- MIDI settings > Scratch settings:
  - **Crossfader cutting mode**: changes the hardware crossfader from a smooth fade to instant cut.
  - **Invert crossfader**: reverses the crossfader's left and right orientation.
- **Crossfader FX**: see section 9.

### Volume and pre-cueing (Settings)
- **Main volume slider**: Dashboard > **Settings** tab.
- Android has only two audio settings: **Main Volume** and **Split Output** (for pre-cueing). All other
  routing is set by the device or the connected controller or interface. There is no mixer mode or
  device selector on Android (linked page).
- **Split Output**: master goes to one side of the stereo channel and cue to the other. The linked page
  specifies **Master = left, Cue = right**. djay creates **binaural mono** on each side. This needs a
  **stereo/mono DJ split cable**; a generic aux splitter does not work (linked page). PRO only.
- With a controller or interface, while pre-cueing: headphone volume, the **Cue/Mix** knob (balance
  between the cued track and the main mix), and **Split Cue** (cued track in one ear, main mix in the
  other).
- Advanced > Pre-cueing > **Auto select**: automatically selects the inactive deck for the headphones.
- The linked page notes that some Android USB audio drivers do not fully support multichannel audio.
  That can cause missing master or cue audio, distorted sound, or silence. Workarounds: switch the
  controller to a two-channel mode, or use Split Output. Pre-cueing is **not supported over Bluetooth
  audio**.

---

## 6. Neural Mix™ (stems)

- Separates a track into **vocals, drums, bass and harmonics** using AI. Not available for **Spotify
  or Apple Music** tracks; it works with TIDAL, SoundCloud and Beatport (linked page).
- Layout (Neural Mix overview):
  - **Neural Mix™ icon**: shows or hides the Neural Mix section (in Tools).
  - **Neural Mix™ dropdown**: grouped stems (for example Instrumental/Acapella) or individual stems
    (for example Drums, Harmonics, Vocals).
  - **Solo icons**: solo the selected stems instantly.
  - **Stem volume faders**: one per stem.
  - **Mute FX**: echo tails when muting or soloing (see below).
  - **Mixer**: Neural Mix EQ and Neural Mix crossfaders **require the mixer to be open**.
  - **EQ mode dropdown**: Classic EQ / Neural Mix™ EQ / Neural Mix™ crossfaders.
  - **EQ options**: reveals the EQ mode dropdown.
- **Neural Mix™ tracks** (Tools > Neural Mix icon > dropdown), the number of stems shown:
  - **Two stems**: "Instrumental & Acapella" or "Percussive & Tonal".
  - **Three stems**: "Drums & Harmonic & Vocals" or "Drums & Bass & Melodic".
  - **Four stems**: Drums, Bass, Harmonic, Vocals.
  - Each stem can be muted, soloed and set in volume.
- **Neural Mix™ EQ**: the EQ knobs control stems. On the Neural Mix pages: Low = Drums, Mid = Harmonics,
  High = Vocals. The alternative layout is Drums/Bass/Melodic, chosen in Settings > Sound.
- **Neural Mix™ crossfaders**: blend or swap individual stems (vocals, drums, bass) between the two
  decks. **[Vague]** The number of crossfaders and their layout are not described.
- **Mute FX** (Mute FX dropdown in the Neural Mix section): tap the **Mute FX button** to activate it,
  then choose when the **Echo Out** begins: **1 beat**, **¾ beat**, **½ beat**. Muting or soloing a stem
  then plays an echo tail instead of an abrupt cut. The default length is not stated.
- **Essential tools > Neural Mix™** (next to the crossfader): quick stem isolation in two groups:
  - **Instrumental – Acapella**: melodic and harmonic elements versus isolated vocals.
  - **Percussive – Tonal**: drums and percussion versus basslines, chords and melodies.
  - Mute FX can be enabled here too.
- **General > Reset Neural Mix controls**: resets the stem levels when a new track is loaded.
- **FX routing to stems**: see section 9.

---

## 7. Sync, tempo, key and pitch

### Beatgrids
- The grid is built from the track's **tempo (BPM)** and the position of the **first downbeat**. Each
  track is analysed automatically when it is loaded.
- **Straight beatgrid**: steady tempo (most House and Techno). **Dynamic beatgrid**: tempo changes
  (live drums, tempo switches, older recordings). "The beatgrid follows the song—the song doesn't follow
  the beatgrid." The type is detected automatically and can be changed in **Edit BPM**. Tempo change
  detection can be turned off in **Settings** > **Library**.

### Edit BPM (tap the BPM value next to the tempo slider > Edit BPM)
- **/2** or **x2**: halve or double the BPM.
- **Manually set BPM**: type a value, for example **127.50**. djay reanalyses the grid at that tempo.
  The resulting BPM may deviate slightly from the value entered.
- **Tap to set BPM**: tap along with the beat, on screen or on a MIDI controller.
- **Beatgrid type**: Dynamic or Straight.
- **Apply** saves; **Reset** restores the previous setting (or djay's original beatgrid, in the editor).
- **Tempo tab** (same menu): **percentage range** of the tempo slider and **Invert the slider direction**.

### Beatgrid editor (Tools > Beatgrid editor; also reached from the Skip dropdown > **Edit beatgrid**)
Four sections:
- **BPM section**: /2, x2, and a dropdown with halve/double, set the value, tap BPM, Dynamic/Straight.
  **Reset** reverts to djay's original grid.
- **Downbeat section**: with the scrolling waveform visible, the arrows shift the grid to the correct
  downbeat, and **Set** confirms. The new downbeat applies across the whole track. A "Quick access
  editor" is also mentioned. **[Vague]** It is not described further.
- **Anchor section**: **Add** drops an Anchor Point where the tempo change begins. Add more anchors on
  downbeats until the grid realigns. Left and right arrows fine-adjust an anchor. djay recalculates the
  grid between anchors.
- **Grid section**: move the play marker and tap **Set** to define the **1st bar**; bar numbers count up
  from 1 from that point. The **arrows** nudge the whole grid left or right.

### Sync (Settings > General > Sync Mode > **Sync Type**)
- **BPM Sync** ("Tempo"): matches only the BPM to the **current main deck**; the beats are not locked.
  When active, the **Sync button label turns blue**.
- **BPM + Beat Sync** ("Beat sync"): matches the BPM and the beat position, so the downbeats are aligned
  and the grids locked. When active, **the whole Sync button turns blue**.
- **[Conflict / Vague]** The sync page syncs to "the current main deck". The interface page says the
  "opposite deck". The mixing page says pressing Sync on Deck 2 "matches the BPM and aligns the beats
  with the track on Deck 1". How the "main deck" is chosen is not stated.
- Crossfader FX "will still beat sync regardless of your sync mode", following the beat sync interval.
- **Soft takeover**: after Sync moves the on-screen pitch fader, the hardware pitch fader has no effect
  until it is moved to match the on-screen position.
- **Ghost Fader**: an on-screen marker showing where the hardware tempo fader currently sits.

### Settings > General > Sync mode (exact options)
- **Sync type**: BPM only (Tempo) or BPM and beats (Beat sync).
- **Beat Sync interval**: **1 beat** or **1 bar**.
- **Resume sync after pause, scratch, or cue jump**.
- **Maintain sync on song load**.
- **Reset tempo when turning sync off**: returns to the original BPM.
- **[Vague]** The defaults of these options are not stated.

### Settings > General > Tempo
- **Slider range**: **±8% to ±75%**. **[Vague]** The intermediate steps and the default are not listed.
- **Invert slider**: reverses the slider direction, in software and on hardware.

### Settings > General > Play/Pause
- **Start time**: how gradually the track speeds up on play (vinyl start).
- **Stop time**: how gradually it slows down when stopped (turntable power-down).
- **[Vague]** Units, range and default are not stated.

### Key lock and key
- **Key Lock**: changes tempo without changing pitch. Toggle it with the **musical note icon next to
  the deck's tempo slider**; the **icon is blue when active**. **Key Lock is turned on by default.**
  It can be MIDI-mapped. Note: "Extreme tempo changes may affect audio quality when Key Lock is on."
- **Key Shift** (Tips: Mixing in key): shift the key manually "in semitone steps (**±12**)".
  **[Vague]** The manual does not show where the key shift control is in the Android UI.
- **Key display**: the key shows in the deck's Track information area and in the library browser
  "alongside the BPM".
- **Settings > Library > Key**:
  - **Match**: **Exact** key matching, or **Fuzzy** (harmonically related keys via the circle of fifths).
  - **Format**: **Musical Key**, **Musical Key with Major/Minor**, **Numerical Key**, **OpenKey**.
  - **Sort by**: when browsing by key, **Similarity** (circle of fifths) or **Pitch distance**
    (chromatic).
  - **Show keys in different colors**: colour-codes the key labels.
  - **[Vague]** Defaults are not stated, and the colour scheme is not described.
- **Settings > Library > BPM Analysis**:
  - **Range**: the expected BPM range, which avoids half or double detection. The values are not listed.
  - **Tempo Change Detection**: tracks tempo fluctuations within a song.
- **Pitch Cue**: see section 10.

---

## 8. Automix

### Views
- Switch to **Automix view** via the Dashboard. Then choose **Mix options**, pick a playlist or add
  tracks to the **Automix playlist**, and tap **Play** if music does not start automatically. Repeat,
  shuffle and add tracks are available.
- Elements: **Mix options** (transition effects, blend duration, tempo), **Automix playlist**, **Play**
  (start or pause Automix), **Next track** ("Tap to mix into the next track in the playlist"), **BPM**
  (tap to adjust the playing track's tempo), **Scrolling waveform**, **Waveform overview**, **Automix
  entry region** (white line at the start), **Automix exit region** (white line at the end), **Cue
  point** markers.
- **Portrait**: a player-style view (artwork, playback controls, track info).

### Settings > Automix > Mix
- **Transition**: "**Automatic, Fade, Filter, EQ, Echo, Dissolve, Neural Mix™, and more**".
  **[Vague]** The full list is not given ("and more"), and the individual styles are not described.
- **Duration**: **Automatic**, or a **Manual** duration given as a number of **bars (measures)** or
  **seconds**. **[Vague]** The allowed values and the default are not stated.
- **Tempo adjust**:
  - **Off**: no beatmatching.
  - **Sync**: keeps a consistent BPM throughout the mix.
  - **Sync + Tempo blend**: gradually adjusts the outgoing track's tempo to match the incoming one.
  - **Automatic**: if there is a noticeable tempo difference, djay adjusts the BPM to match the
    currently playing song.

### Settings > Automix > Song range
- **Start point**: **Automatic**, or **Manual** (X seconds from the start of the track).
- **End point**: **Automatic**, or **Manual** (X seconds before the end of the track).
- **Use start and end point of song if available**: honours the track's own Automix start and end
  markers.
- **Maximum song play duration**: how long each track plays before djay mixes into the next. The values
  are not listed.
- The manual has a second heading also titled "Song range", which is evidently a copy error. Under it:
  - **Shuffle**: random play order.
  - **Repeat**: loops the playlist from the beginning when it ends.

### Custom Automix start and end points for each track
Dashboard > **Classic mode** > **Tools** > **Skip**. In the **Automix section**, set the transition
start and end points; **Edit mode** (pencil) adjusts or deletes them. The Beat Skip page also says that
custom Automix start and end points are set here.

---

## 9. Effects (FX)

### Effects section (Tools in Classic mode). Three modes:
- **Pad**: an **X/Y pad** controls the selected effect with finger gestures. Choose the effect with the
  dropdown next to its name, or with the **skip button** (next effect). **[Vague]** The manual does not
  say what the X and Y axes control.
- **Instant**: **eight one-tap effects**. **Edit Mode** (pencil) chooses which effects appear. Trigger
  them by **tap or hold**, or turn on **Hold mode**: when **Hold** is highlighted blue, the effect stays
  on until turned off.
- **Manual**: **up to three effects per deck**. Change one by tapping its name. Each has **on/off**,
  **dry/wet**, and an intensity control. "Depending on the effect you select, a **slider or beat
  control** will appear beneath its name." In Manual mode, tap **On** to activate, then adjust "using
  the slider, knob, or beat options".
- The glossary says effects can be chained: "up to three audio effects and two video effects per
  channel". Video applies to other platforms.

### Effect list
- The manual names effect **categories** only: **reverbs, echoes, sweeps, macros, filters, modulations**
  (flangers, phasers) "and more". **Macros** "combine multiple effects into one powerful control".
- **[Vague]** **The manual does not list the individual effects or any effect's parameters, ranges
  or defaults.** The Effects dropdown "opens the full list of Audio FX", but that list appears only in
  the app or in screenshots. The linked free-vs-pro page says PRO content packs include "50+ audio
  effects".

### FX routing (Neural Mix stems)
- Apply FX to individual stems: open the FX section, choose **Pad** or **Manual**, pick the effect,
  then tap the **drums**, **bass**, **harmonic** or **vocals** icons in **FX routing**. One or more
  stems can be chosen. The routing options depend on the stems selected in the Neural Mix dropdown.
- **Bounce Pads** can also be routed to stems (vocals, harmonics, bass, drums) via the **FX routing**
  dropdown.
- Pre-fader or post-fader behaviour: Settings > Sound > **FX routing** (section 5).

### Essential tools (circular button next to the crossfader)
- The button shows the icon of the last-used tool and is highlighted when active. Its dropdown switches
  between **Neural Mix™**, **FX**, and **Beat Skip**.
- **FX**: **two dedicated FX channels**, available even when the FX panel is closed; two favourite
  effects can be assigned and swapped.
- **Beat Skip**: the beat skip controls, available even when the Beat Skip section is closed.

### Crossfader FX
- Effects applied while crossfading: **Echos, Filters, Tremolos, Sweeps, Risers, Neural Mix™**, plus
  **Tempo Blend**. The transition length is set in **bars**. **[Vague]** The bar values are not listed.
- **Tempo Blend**: with tracks of different tempos, it "will automatically sync them as you move the
  crossfader".
- **Neural Mix™ FX**: separates and blends stems during the crossfade.
- How to use it: load both decks, then tap the **dropdown icon next to the crossfader** to open
  **Crossfader FX**. Toggle the **Crossfader FX button** on, choose the effect(s), and select the number
  of bars. With **both tracks playing**, either **move the crossfader** or tap **Mix now**.
- When Crossfader FX is on, a **blue line down the centre of the crossfader** shows it.
- Crossfader FX "can override your selected Sync mode and change how the crossfader behaves". For
  manual beatmatching practice, the manual suggests turning **Crossfader FX Sync Mode** off.
- **Settings > General > Crossfader FX**:
  - **Auto-play when moving crossfader**: "If Crossfader FX is on and Deck 1 is playing while Deck 2 is
    paused, moving the crossfader toward Deck 2 will automatically start playback on that deck."
    The manual describes only the Deck 1 to Deck 2 case. **[Vague]** The default is not stated, and
    the reverse direction is not described explicitly.
  - **Sync mode**: **Off** (no sync), **On** (sync maintained during crossfades), **Sync + Tempo Blend**
    (sync plus tempo blending).
- In **Starter** view, crossfading is always tempo-blended (section 2).

---

## 10. Looping, cue points, skip, slice

### Cue points (Tools > Cue points icon > **Cue**; this view also gives access to Pitch Cue and Skip)
- Up to **16 cue points per track**; they are saved in the collection. Switch between **1–8** and
  **9–16** with the **three-dot menu**.
- Create one: move the playhead and tap an **unassigned cue pad**. Each is colour-coded and can be
  renamed. Cue points appear on the pads, on the waveform, and on the controller's pad matrix.
- Delete or rename: tap the **cue point editor (pencil)**. The **X** on a pad deletes it. The dropdown
  next to the timestamp renames it or assigns a colour.
- **Quantize**: a switch plus a snap value of **1/16, 1/8, 1/4, 1/2, 1 beat, or 4 beats**. It also
  applies to loops.
- **Cue set / Cue jump** buttons on the deck: see section 2.

### Cue loops
- Create a loop (Loop section, tap an empty pad); while it repeats, create a cue point (cue section,
  tap an empty pad). Both are stored on the same pad, and triggering the cue activates the loop.
- A **Cue Loop icon** on the right of the pad exits the loop.

### Loops (Tools > Loop icon). The **Auto** dropdown offers three views:
- **Auto / In - Out**: the **arrow buttons** set the loop length. Tap the **loop button between the
  arrows** (it shows the number) to start the loop; it turns **blue** while active. Tap again to exit.
  If the loop is inactive, pressing **Loop repeat** jumps back to it. **In / Out**: tap **In** at the
  start and **Out** at the end; Loop repeat exits.
- **Manual / Skip**: a condensed manual loop and skip view. Skip by **1–128 beats** with the skip
  arrows (all skip functions except the Autocue start and end points). There are **two loop slots**:
  new loops go there, or the first two saved loops are shown.
- **Auto (1–32)**: one-tap loops of **1, 2, 4, 8, 16, 32 beats**; tap a number to toggle. There are
  two loop slots as above.
- **Saved** tab: up to **eight saved loops** per track, shown as **blue** pads with optional names.
  Tap a pad to jump to its loop. Tap the **loop icon on the pad** to activate the loop *without
  jumping*. Edit (pencil) then **X** removes a loop; loops can be renamed. Quantize is available with
  the same snap values as cue points.
- **Bounce** tab:
  - **Bounce Pads**: **six pads**, loop lengths from **2 beats down to 1/16 beat**. *Tap and hold* a
    pad to loop; release to return to normal playback. They can be routed to stems.
  - **Pad**: an X/Y pad that combines loop length with a **high-pass or low-pass filter**.
    **[Vague]** The axis mapping is not described.
- **[Vague]** The default loop length of the Auto loop arrows is not stated.

### Beat skip
- Set the jump size (**1 to 128 beats**); the skip arrows move forward or back. While a loop is
  active, skipping **shifts the entire loop** by the skip amount.
- The dropdown next to **Skip** opens **Slice** and **Edit beatgrid**. Automix start and end points
  are set here too (section 8).

### Slice (Tools > Slice)
- **Eight pads** divide the loop into equal slices. Holding a pad plays its slice; on release,
  playback jumps back to the correct position in the track.
- Dropdown: **repeat length** (**1/8 beat to 1 beat**), **Quantize Jumps** on/off, **4- or 8-beat**
  slicing.

### Pitch Cue
- The pads show the selected cue point **transposed into eight different keys** within a scale.
- Dropdown: view and switch cue points, then choose a scale: **Chromatic (-7 to 0)**, **Chromatic (-4
  to 3)**, **Chromatic (0 to 7)**, **Major**, **Minor**, **Blues**. **Reset** returns to the original
  key.

### Settings > Advanced > Cue points & Loops
- **Auto-play when triggering cue point**: starts playback when jumping to a cue point.
- **Save active loop when setting cue point**: the active loop is stored with the cue.
- **Set Start Cue at loop beginning**.
- **Deactivate loop when jumping outside of loop**.
- **Always quantize loops to the beat**.
- **[Vague]** Defaults are not stated.

### Settings > General > Song loading
- **Start playback**: autoplay on load.
- **Reset EQ, FX, tempo**: resets them to default when a track loads.
- **Reset Neural Mix controls**.
- **Protect active deck**: prevents replacing a track while it plays.
- **Jump to cue point**: load position, either **Off** (track beginning), **Start Cue**, **Cue Point
  1**, or a specific cue point (**1–16**).
- **Activate loop**: automatically enables the **first saved loop**, the **earliest saved loop (1–8)**,
  or the **earliest cue loop (1–16)**; when playback reaches it, the section loops.
- **[Vague]** Defaults are not stated, apart from Start Playback being on, by implication (section 3).

---

## 11. Library

### Navigation and sources
- The **library browser** opens by tapping the **musical note icon** on an empty deck.
- The **sources dropdown** (top-left) chooses own collection, streaming services, or djay Music.
  The **pencil icon** shows or hides libraries.
- Tabs and views mentioned: **Match** tab, **Queue** tab (at the bottom of the screen), **History**
  (from the library dropdown), **Files** collection, and djay Music's **Spotlight**, **Genres**,
  **Labels** tabs.
- **Search**: select a library (Local Music, djay Music, or a streaming service) and use the search bar.

### Files (own music)
- **Files** tab > **Add Folder**, or **+** (upper right) > **Add Folder**. This opens the Android
  system picker: **Use this folder**, then **Allow**. USB drives work through an OTG adapter.

### Sorting
- The **Sort dropdown** at the top of the playlist sorts by **Artist, BPM, Key, Time, Title**. **BPM**
  sorts slowest to fastest. *Tapping the category again reverses the order.*
- Key sorting follows Settings > Library > Key > **Sort by** (Similarity / Pitch distance).
- **[Vague]** **No list of library columns appears anywhere in the manual.** Visible track fields are
  mentioned only in passing: key and BPM ("in the browser, alongside the BPM"). The default sort is not
  stated, and there is no list-versus-grid view option.

### Track Match
- Choose a library, select a track, then tap the **Match tab**: suggestions "based on key and tempo",
  across the library and connected streaming services. Custom tracks can be added to the Match playlist.

### Queue
- Three-dot menu on a track > **Add to Queue** (from libraries, playlists or search), or from Track
  options on a deck. The Queue shows what is playing now and what comes next.
- Settings > Library > Options > **Remove songs from Queue when played**.

### History
- Library dropdown > **History**: tracklists saved by date. To start a new session manually: History
  tab > **+** (top right) > **New Session**.

### Analysis
- Each track is analysed automatically the first time it is loaded. Batch analysis: dropdown next to
  the track counter (or a track's three-dot menu) > **Analyze Songs**. Some streaming tracks cannot be
  pre-analysed.

### Streaming services
- **Spotify** (Premium), **Apple Music**, **TIDAL** (subscription with **DJ Extension** add-on),
  **SoundCloud** (Go+), **Beatport** (Advanced or Professional plan), **Beatsource** (Standard or Pro+).
  The linked page says Beatsource customers are being migrated to Beatport.
- Neural Mix is not available for Spotify or Apple Music. **Mix recording is disabled when using
  streaming services other than djay Music.**
- **djay Music**: a free catalogue in the sources dropdown.

---

## 12. Recording a mix

- Dashboard > **record button** > **Start New Audio Recording**. While recording, the **Dashboard
  button shows a red ring**. To stop: tap the Record button > **Stop recording**.
- "Your mix will be saved in the **recording menu**."
- Close the app only after stopping; otherwise the file may be corrupted.
- **Format** (Settings > Advanced > Recording > **Format**): **AAC** (compressed, smaller) or **WAV**
  (uncompressed, higher quality). **[Vague]** The default is not stated; nor are sample rate, bit depth
  or bitrate.
- "You can record either audio only or audio with video." **[Vague]** The linked free-vs-pro page says
  video mixing is macOS and iOS only, so this line may not apply to Android.
- Recording is not available with streamed tracks (Spotify, Apple Music, TIDAL, Beatport/Beatsource,
  SoundCloud).
- **[Vague]** **The manual says nothing about where files are saved on the device, how they are named,
  or how to export or share them.**

---

## 13. Sampler, Sequencer, Looper (PRO)

- **Sampler** (Sampler/Looper icon in Classic mode > **Sampler**): a **4×2 pad grid** of one-shot
  samples. A volume knob left of the pads sets each sample's level. Controls: Sampler/Looper icon,
  Sampler and Looper selectors, **Volume slider**, **Metronome**, **Sampler Sequencer options**
  (sequence length, Quantize), **Record button**, **Sample grid** (8 samples), **Sample packs**
  dropdown, and **Sample editor** (pencil: swap or remove samples, record from Deck 1 or 2).
- **Sequencer**: the default loop length is one **4-beat bar**, extendable to **8 beats**; quantization
  is in the **Record options dropdown**. The metronome follows a playing track's BPM, otherwise the
  Record-options BPM; its downbeat click has a slightly higher pitch. **Record** starts sequencing, and
  overdubbing is supported. Sample packs can be switched while recording.
- **Recording samples from a deck**: Sampler > Edit (pencil) > the three dots on a pad > **My
  Samples**, then source **Deck 1** or **Deck 2**. *Tap and hold* to record; release to stop. Samples
  are saved to **My Samples**, and **Edit** renames them. *Swipe left on a sample name to delete it.*
  The **Add folder** icon (top left) imports files.
- **Custom sample packs**: Sample pack menu > **+** (top left) > **Add Custom Pack**, then name it.
  Each pad's three-dot menu chooses a sample (from the **Sample Packs** tab or **My Samples**).
- **Looper** (Sampler/Looper icon > **Looper**): a loop grid of **2x4 pads** ("eight synced
  channels"), with loops shown in columns, each column one element (for example kicks, hi-hats,
  vocals). Every loop is quantised to a **4-beat** measure and stops at the end of the bar when
  disabled. Packs come from the **Looper packs dropdown**. **Play** (next to the volume) launches one
  loop from each column; tap it again to stop all. The **BPM dropdown** changes the tempo; **Reset**
  returns to the pack default. When a deck is playing, the Looper follows its tempo.

---

## 14. MIDI controllers and hardware

- Supported controllers are **plug-and-play**: djay detects them and configures audio automatically
  (controller master = main output, headphones = pre-cue). The list is on the DJ Gear page; support
  differs by OS.
- USB: use an **OTG-compatible USB-C or Micro-USB adapter**, connect power to the hub or device
  (recommended for long sessions), plug in and power the controller, then open djay.
- **Bluetooth controllers** (for example Hercules DJControl Mix): turn on Android Bluetooth and put
  the controller in pairing mode, then **Dashboard** > **Settings** > **MIDI devices** > **Connect
  Bluetooth Contoller** (sic). An offline, previously paired controller reconnects when its name is
  tapped.
- Audio interfaces: Android supports **class-compliant** interfaces (no extra drivers needed). There is
  no support for untested hardware.
- **MIDI settings**: **Scratch settings** (Crossfader cutting mode, Invert crossfader) and **Bluetooth**
  (pair a Bluetooth MIDI device).
- Hardware tempo: **Soft takeover** and **Ghost Fader** (section 7). Gain: **Unlink controller gain
  from on-screen gain** (section 5). Key Lock can be mapped to a controller button.
- **MIDI mapping / MIDI Learn**: the Android manual has no page for it. The Key lock page links to the
  **iOS** page. On iOS, PRO is required: map in the MIDI Configuration window (move a control, choose
  **Target** and **Action**, **Save**), and edit via **Dashboard** > **Settings** > **MIDI devices**. The
  linked Android audio page also mentions "creating a custom MIDI mapping" for unsupported controllers.
  **[Vague]** Whether the iOS flow is identical on Android is not stated.

---

## 15. Gestures (everything the manual states)

| Gesture | Where | Effect |
|---|---|---|
| Tap | Track information | Toggles between time views (elapsed or remaining) |
| Tap | BPM value next to tempo slider | Opens **Edit BPM** |
| Tap or swipe | Waveform overview | Skips to that section of the track |
| "Scroll gently" left or right | Waveform or deck | Nudge: left slows down, right speeds up |
| Hold | **Cue jump** button | Plays from the cue point while held |
| Press and hold | Track artwork of a loaded deck | Opens **Track options** (Double, Load previous/next, Scratch tools, Add to Queue, Eject) |
| Tap and hold | Load track on an empty deck | Opens Scratch Tools |
| Tap and hold | Bounce Pad | Loops while held; release returns to normal playback |
| Hold | Slice pad | Plays the slice while held; release jumps back to the correct position |
| Tap or hold | Instant FX pad | Triggers the effect (or **Hold mode** latches it) |
| Finger on X/Y pad | Pad FX, Bounce > Pad | Controls the effect, or loop length plus HP/LP filter |
| Tap and hold | Deck 1 / Deck 2 record button (Sampler) | Records while held |
| Swipe left | Sample name in My Samples | Deletes the sample |
| Tap loop icon on pad | Saved loop pad | Activates the loop without jumping; tapping elsewhere on the pad jumps |
| Tap again | Sort category | Reverses the sort order |

- **[Vague]** **The Android manual does not mention any double-tap gesture.** It also mentions no
  gesture to reset a knob or fader to its default (the only "Reset" items are the zoom, beatgrid, Pitch
  Cue and Looper BPM buttons), and no pinch-to-zoom. The glossary mentions pinching or swiping only as
  a general description of multi-touch.

---

## 16. Other facts

- Recommended files: **MP3 320 kbps** or lossless (**WAV**, **AIFF**); avoid below **192 kbps**.
- Gain staging: play the loudest part and raise the gain until the meter peaks just below red.
- **djay School**: Dashboard > Settings > **djay School** tab (tutors Emma Short-E, DJ TLM, DJ Hapa).
- Support and social links: Dashboard > Settings.
- Uninstall: the usual Android route; it removes all app data.

---

## Blocked hosts

None. All required hosts (`help.algoriddim.com`, `www.algoriddim.com`) answered HTTP 200.
`community.algoriddim.com` was not needed, because the manual only links to it as a forum.
