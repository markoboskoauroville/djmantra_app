# Round 41, V42 (2d135b4) on the Pixel 7, 14.10.2026 (wireless adb; Mix Ultra on the phone's USB)

| part | check | result | notes |
|---|---|---|---|
| 12 | controller found at start | PASS | `Looking for "DJControl Mix Ultra" over USB and Bluetooth`, `Connected (usb)`, MIDI in flowing |
| 12 | USB/BT pill + toast | FAIL | no pill, no toast, no `controller link:` line (builder: control created after binding; fixed in V43 bd4963c) |
| 15 | folder filter | PASS* | "aum" -> only Aum Ganesha (5:16 114.6 10A). *Not live while typing: Gboard keeps the word "composing" (underlined) and the list changes only on space/Enter (adb `input text` + Enter). The ✕ clear button draws as an empty box (missing glyph) |
| 15 | FIND ON PHONE | PASS | `DJMantraStorage: findSongs 'aum': 2`, `find on phone 'aum': 2`: Aum Ganesha + Boom (Traumer Remix) (matches "tr-aum-er"); hits show length but no BPM, key or cover |
| 15 | tap a hit loads it | PASS | `load [Channel1] .../Aum Ganesha...` |
| 16 | steady grid re-analysis | PASS | `Beats: Aum Ganesha ... a beat map, the steady grid is chosen: again` |
| 16 | same BPM start / middle / end | PASS | PITCH handle at 0 %: **116.7** at the start, the middle and 0:12 before the end (the scale's middle now also 116.7) |
| 16 | BPM vs djay | FAIL? | 116.7 vs djay 115 and our own library 114.6 |
