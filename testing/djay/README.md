# djay Pro as a reference (reverse engineered on Marko's Mac, 13.10.2026)

djay Pro 5 (Algoriddim, Mac App Store build `com.algoriddim.djay-iphone-free`) analysed the 150 Sunrise Dance test
songs (`~/Downloads/GDriveDownloader/❂ Sunrise Dance ❂`, 146 distinct titles). Everything here was read from local
files; no djay online service was touched.

## Where djay keeps what

| What | Where | Content |
|---|---|---|
| Library | `~/Music/djay/djay Media Library.djayMediaLibrary/MediaLibrary.db` | YapDatabase (SQLite, table `database2`, collections `mediaItems`, `mediaItemAnalyzedData`, `localMediaItemLocations`, ...). Values are Algoriddim's own "TSAF" serialisation. `mediaItemAnalyzedData` holds only title, artist, duration, BPM, key index (~200 bytes). No grid or waveform. |
| **Analysis per song** | `~/Library/Group Containers/VJXTL73S8G.com.algoriddim.userdata/Library/Application Support/Algoriddim/Metadata/<2 hex>/<md5>.djayMetadata` | A **binary plist** (Python `plistlib`), ~150 kB per song. **Everything that matters is here**: waveform colours, amplitudes, beats, key, gain, song structure. |

`djay_metadata.py` decodes one file into plain JSON; `decoded_150.tar.gz` holds that JSON for the 146 titles
(`sunrise/<title>.json` + `sunrise/index.json` mapping to the mp3 file names).

## The fields of a .djayMetadata

All `compressed*` blobs are zlib. Floats inside are **big-endian float32**.

- `info`: Name, Artist, Duration (s).
- `keyInfo`: `keyIndex`, `secondKeyIndex`, `keyConfidence`, `keyReferenceTuning` (440). **The key index uses the
  same order as libkeyfinder's `key_t`**: 0 A, 1 Am, 2 Bb, 3 Bbm, 4 B, 5 Bm, 6 C, 7 Cm, 8 Db, 9 C#m, 10 D, 11 Dm,
  12 Eb, 13 Ebm, 14 E, 15 Em, 16 F, 17 Fm, 18 F#, 19 F#m, 20 G, 21 Gm, 22 Ab, 23 G#m. Checked on screen: Nanana 21 =
  Gm (6A), That Girl 17 = Fm (4A), About You 7 = Cm (5A), 4B 21 = Gm (6A).
- `deepBeatTrackerInfo` (version 8, model `ARDeepBeatTrackerModel1011_9`, a neural beat tracker):
  `bpm`, `analyzedBPM`, `straightBPM`, `bpmConfidence`, `straightGrid` (constant tempo), `firstDownBeatIndex`,
  `timeSignatureIndex` (4), `compressedBeats` (every beat time in s), `compressedAlternateBeats` (second candidate
  grid), `compressedTransientPositions` + `compressedTransientEnergies` (onsets, s + energy),
  `compressedTransientActivation` (the network's onset activation at `transientActivationFPS` = 105 fps),
  `compressedBPMChangeTimes`, `compressedPrevalentBPMs`.
- `songStructureInfo`: **only the mix-in and mix-out ranges**, not labelled sections: `startRangeInPoint` /
  `startRangeOutPoint` (where the intro is mixable), `endRangeInPoint` / `endRangeOutPoint` (outro), all candidates
  in `allStartRangePoints` / `allEndRangePoints`, plus energies. Missing on 25 of 146 songs. djay has no phrase labels like
  rekordbox's PSSI.
- `newGainInfo`: `AutoTitleGain` (dB, loudness normalisation), `AutoTitleGainLoudnessRange`.
- `waveInfoCompact` (the amplitude): `compressedLowRateWaveSamples` (uint8 average) and
  `compressedLowRateWaveSamplesMax` (uint8 peak), 4097 points over the whole song at `lowRateWaveFinalSampleRate`
  (~17.7 Hz for 232 s), normalised (`lowRateWavePeak` is the scale).
- `waveColorsInfo` (the colour, version 5):
  - `normalRateWaveSamplesColors` (raw when the song is on a deck) or `compressedNormalRateWaveSamplesColors`:
    **one colour per 256 audio samples** (`normalRateSampleRate` = 44100/256 = 172.27 Hz), **RGB565 little-endian**
    (r 5 bits, g 6, b 5). Silence is `0x3186` (dark grey).
  - `compressedLowRateWaveColors`: 6 bytes per item at ~4.5 Hz for the overview (format not yet pinned down; the
    normal-rate colours resampled reproduce the overview exactly, see `renders/`).

## How the waveform looks (screenshots/)

- **Colour = spectral content per 5.8 ms**, precomputed: bass-heavy hits (kicks) are **red / orange**, mids
  **green**, highs **blue / violet**, mixtures in between (pink, purple, teal). Breakdowns without a kick turn
  **green / teal**; the whole drop reads red-pink. That is why the overview of Nanana is red, then a green break at
  ~55 %, then red, then green again (compare `renders/It_Goes_Like_Nanana_-_Edit.png` row 1 with
  `screenshots/nanana_deck_header_overview.png`).
- **Overview** (deck header, ~680 x 25 px): a mirrored waveform, height from the low-rate max, colour from the
  colour array, a darker outer envelope (peak) and a brighter inner body (average). Played part to the left of
  the red playhead line; a small white triangle marks the cue.
- **Big scrolling lane** (`nanana_zoomed_beats.png`, `two_lanes_*.png`): dark background (#141414), a **thin
  mirrored waveform centred vertically** (low height at this zoom), each transient a small coloured "arrow" (sharp
  attack, decaying tail) in its own colour, so kicks show as red spikes, hats as blue / violet ticks. **Bar lines
  yellow-orange** with the bar number at the top (negative bars before the first downbeat: -11 ... -1, then 1, 2 ...),
  **beat lines thin grey**. The amplitude in the lane is not in the file at that rate: djay computes it from the audio
  when the song loads; only the colours are stored at 172 Hz.
- `renders/<song>.png`: drawn from the decoded data alone. Row 1 the overview (peak darker, average brighter),
  row 2 the colour array alone, row 3 8 s from 60 s at one pixel per colour sample. Use it to check a port.

## To reproduce djay's look in DJ Mantra

1. Per 256 samples (mono, 44.1 kHz) compute the band energies (low / mid / high) and map them to RGB the way the
   `colors` arrays do: fit the mapping with `decoded_150.tar.gz` (the target colour per frame for 146 songs) against
   your own band energies of the same mp3 frames. A first guess: R ∝ low, G ∝ mid, B ∝ high, normalised per frame
   to the loudest band, then gamma; check it against the decoded arrays.
2. Draw a mirrored waveform, peak envelope darker, body brighter, each column in its frame colour.
3. Bar lines orange-yellow with numbers, beat lines grey, on #141414.

## Compare with the app's analysis

`testing/sections/djay.json`: per mp3 file name, djay's BPM, key (standard notation), second key, confidences,
first beat, every beat time, downbeat index, BPM change times and the mix-in / mix-out structure.
