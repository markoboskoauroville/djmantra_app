# Round 41 on V70 (9e67b06), Pixel 7, 14.10.2026 evening (wireless adb; Mix Ultra on Bluetooth: pill "BT")

| part | check | result | notes |
|---|---|---|---|
| 61 | BeatStats browser fits | PASS | opens beatstats.com straight past Cloudflare in the WebView; no status bar over BACK / READ CHART / CLOSE, clear of the camera hole; page ends at the bottom edge, navigation bar hidden (p61_beatstats_browser.png). An empty ~130 px band above the buttons |
| 61 | BACK goes back a page | PASS | Top 10 -> Tracks -> BACK -> Top 10 (p61_beatstats_back.png) |
| 61 | Suno login browser fits | PASS | BACK / DONE on top, clear of clock and hole (p61_suno_browser.png); cookie banner answered "Reject All" |
| 48 | Suno links | PASS | 4 links pasted -> `Suno: 4 links for Suno`, "4 SUNO LINKS REMEMBERED" with titles ("1 svemir, one day porcupine tree", "1 svemir, one day oasis 2", ...); toast "SUNO: 0 SONGS IN SUNO" (none downloaded yet). GET waits for Marko's Suno login |
| — | **layout bug** | FAIL | Songs header at the top level: the PLAYLISTS button overlaps the LIST button ("…LIST" shows from under it) (p61_playlists_overlap.png) |
| — | note | NOTE | first tap on ADD TO SUNO with an empty field: "No Suno links found" (my tap missed the field; second try fine) |
