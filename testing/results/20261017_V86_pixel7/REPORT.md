# Round V86 on the Pixel 7 (17.10.2026)

Tester: the local session on the Pixel 7 itself (Termux / proot, adb over wireless debugging, paired once on
17.10.2026 as 127.0.0.1). Screenshots and logcat stay on the phone in Download/djm_v86/ (not pushed).

| step | result | notes |
|---|---|---|
| 0 install | PASS | V86, versionCode 91 |
| 1 start | PASS | main screen, no crash or QML error |
| 2 play | PASS | a song loads on deck A, PLAY, time counts; AAudio stream started. Sound not heard: Termux:API has no microphone permission |
| 3 mixer knobs | PASS | concentric, no LED rings, HIGH blue / MID green / LOW red, FILTER ring |
| 3b volume fader | PASS | long press TEMPO -> VOLUME, drags, VU lights while playing |
| 4 waveform style | FAIL -> fixed in V87 | the style was read only when the waveform was made (also LANE, LANE HEIGHT); now `settingChanged` |
| 5 manual | PASS -> polished in V87 | 21 sections, pictures, lists, tables; EXPORT MD (+14 images), EXPORT PDF 1.7 MB, 26 pages. Fixed: binding loop on the pictures' width, empty area under the room picture, the ☰ glyph missing from the font; an unused picture removed |
| 6 documents | PASS | BeatStats, Room_Corrections, Manual under Documents/DJ Mantra; none left in Music/djmantra_app. Download_Queue appears on first use; Keys is read from both places |
| 7 BeatStats | PASS | buttons in one row, status line under them, page fills the rest; the status bar shows above it (~120 px) |
| 8 logs | PASS | no FATAL / SIGSEGV / QML errors |

## V87 (e04e9b6) and V88 (8517f56)

- V87: waveform styles DJAY / 3-BAND / ICON differ and follow Settings at once; LANE HALF / MIRRORED too; THIN vs TALL
  differ little by design (0.62 / 0.8 of the lane). Manual: no ☰ box, room picture tight, binding loops 0. 2 min play,
  no crash.
- V88: Download Queues picture with 4 songs and no empty area; the footer says V88 in the reader and in the exported
  MD; EXPORT PDF 1.76 MB; binding loops 0; 30 s play, no crash. **All PASS.**

## 3-song AUTOMIX (V88) and V89 (1e22a0f)

- V88: AUTOMIX from a centred crossfader to a STOPPED deck faded the room to silence (auto-play arms only at an
  end). With the incoming deck playing, the blend was fine (EQ swap, tempo blend, output 0.58-0.74).
- V89: AUTOMIX starts the stopped incoming deck on the beat ("automix: deck 2 started"), output never 0 (peak
  0.59-0.69); back to deck A fine; no song on the other deck -> toast "AUTOMIX: NO SONG ON DECK 2", no fade.
  **All PASS.** By design: the key pill shows the pitched key during TEMPO BLEND (no keylock); the off-air deck
  keeps playing.
