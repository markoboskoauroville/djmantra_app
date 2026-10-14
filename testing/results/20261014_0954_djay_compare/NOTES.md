# djay vs DJ Mantra, Pixel 7, 14.10.2026 — part B: analysis (BPM, key)

- djay: com.algoriddim.djay_free **5.6.9** (Marko's subscription), portrait.
- DJ Mantra: **V41** (0fa0d39).
- Folder: /sdcard/Music, the same 7 audio files in both (DJ Mantra also lists `Return to Oz (ARTBAT Remix).mp4`, djay does not show it).
- djay had analysed 2 of 7 before; the other 5 were analysed by loading each once on deck 1 (11:47-11:49).
- Screenshots (full resolution): `djay_library_music_bpm_key.png`, `djmantra_library_music_bpm_key.png`.

| song | djay BPM | ours BPM | djay key | ours key | key match |
|---|---|---|---|---|---|
| Mantra - Ganesha Sharana | 86 | 84.1 | 9B | 9B | yes |
| Gambara Ganesha Mantra (Sanskrit Devotional Choir) | 111 | 109.9 | 3B | 3B | yes |
| Aum Ganesha Ganesha short english | 115 | 114.6 | 10A | 10A | yes |
| cura prox cura, cura, cura | 118 | 118.0 | 11A | **2B** | no |
| Mantra - Deep Waters 3 (vox_1) | 119 | 118.4 | 7A | 7A | yes |
| djmantra-test-tone | 120 | 120.0 | 1B | **11B** | no |
| djmantra-test-tone-2 | 120 | 120.0 | 2B | **12B** | no |
| Return to Oz (ARTBAT Remix).mp4 | — (not listed) | 124.0 | — | 7A | — |

- BPM: every song within 2 BPM; djay shows whole numbers. Ours is 1-2 BPM lower on the three slow devotional songs (84.1 / 109.9 vs 86 / 111).
- **Aum Ganesha: djay says 115** — so the song's tempo is ~115, not the 91.0 / 109.9 / 117.8 our deck showed at rate 0 (r40 parts 6-8): our beat grid's tempo drifts along the song while the library value 114.6 is right.
- Keys: 4 of 7 match; cura (11A vs 2B) and the two test tones (1B/2B vs 11B/12B) differ — the tones are both 2 steps apart on the wheel in the same direction.
- djay shows no song sections or automatic cue points in its library list (not checked inside the deck yet).

Still to do in this job (after V42 with REC is on the phone): waveform screenshots (paused, playing, zoom in / default / out, overview) + 5 s recordings, and the sound states.
