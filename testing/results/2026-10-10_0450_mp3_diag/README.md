# MP3 diagnosis, Pixel 7, 10.10.2026 (Djapp local)

Round 9 step F (0 of 6 MP3s) ran on **7837ffd**, not 8c83bcb. On **8c83bcb** ("trust the .mp3 suffix"):

| Variant of "Mantra - Deep Waters 3 (vox_1).mp3" | BlackHole | Giving up | engine peak |
|---|---|---|---|
| a original (48 kHz, ID3v2.4, mjpeg cover) | -22.2 dBFS | 0 | 0.404 |
| b no cover | -33.9 dBFS | 0 | 0.404 |
| c no tags, no Xing | -32.7 dBFS | 0 | 0.402 |
| d re-encoded 44.1 kHz 192k | -33.3 dBFS | 0 | 0.390 |

All 6 original MP3s on the phone, 8c83bcb: `all-mp3-8c83bcb.md` (all play, -22 to -33 dBFS, no "Giving up").
Logs (all tags and levels, filtered as asked): `mp3-<variant>.log`; `orig-xxd.txt`, `orig-ffprobe.txt`.
