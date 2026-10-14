# Round 41 on V70 (9e67b06), Pixel 7, 14.10.2026 evening (wireless adb; Mix Ultra on Bluetooth: pill "BT")

| part | check | result | notes |
|---|---|---|---|
| 61 | BeatStats browser fits | PASS | opens beatstats.com straight past Cloudflare in the WebView; no status bar over BACK / READ CHART / CLOSE, clear of the camera hole; page ends at the bottom edge, navigation bar hidden (p61_beatstats_browser.png). An empty ~130 px band above the buttons |
| 61 | BACK goes back a page | PASS | Top 10 -> Tracks -> BACK -> Top 10 (p61_beatstats_back.png) |
| 61 | Suno login browser fits | PASS | BACK / DONE on top, clear of clock and hole (p61_suno_browser.png); cookie banner answered "Reject All" |
| 48 | Suno links | PASS | 4 links pasted -> `Suno: 4 links for Suno`, "4 SUNO LINKS REMEMBERED" with titles ("1 svemir, one day porcupine tree", "1 svemir, one day oasis 2", ...); toast "SUNO: 0 SONGS IN SUNO" (none downloaded yet). GET waits for Marko's Suno login |
| — | **layout bug** | FAIL | Songs header at the top level: the PLAYLISTS button overlaps the LIST button ("…LIST" shows from under it) (p61_playlists_overlap.png) |
| — | note | NOTE | first tap on ADD TO SUNO with an empty field: "No Suno links found" (my tap missed the field; second try fine) |

## Parts 59–60, Room Correction (V70, real Pixel 7, 14.10.2026 21:43–22:08)
Mic permission granted ("While using the app"); AUTO CALIBRATE ran one 8 s sweep at -18 dB, phone microphone, output on BT.

**FAIL: a measurement of nothing is saved and applied.**
- Log: `sweep 8.0 s, input 0, peak -55.8 dBFS` — the mic barely heard the sweep, yet place 1 was stored.
- Measured columns sit at about -12 dB up to 1.2 kHz, then jump to +20…+62 dB above 1.3 kHz (noise, not a room).
- `My room.json` "correction": 0 dB up to 1 kHz, then **-12 dB on every band from 1.3 to 16 kHz**. Applying that would kill the highs.
- Wanted: below a sane input level (say peak < -40 dBFS, or a jump of more than 20 dB between neighbouring bands), refuse the place: "DIDN'T HEAR THE SWEEP: louder TEST LEVEL or closer to the speaker", with nothing saved.

**FAIL: the graph doesn't draw the applied correction.** The yellow curve shows 0 dB above 1 kHz, but the file applies -12 dB there. It also looked the same under FLAT and HOUSE.

**FINDING: the sweep file is a song.** `Room_Corrections/sweep_8s.wav` is scanned into the library: auto gain, beat grid, loaded into [Sampler9], and a genre lookup goes out to Deezer (wasted SIM data). Room_Corrections/ should be skipped by the scanner and the genre lookup, e.g. with a `.nomedia` file and a path filter.

**PASS:** MORE panel (target FLAT/HOUSE/CLUB, strength, sweep/pink, length, level, mic, ANALYZER, CORRECTION GUIDE). HOUSE target draws its tilt line, with the toast "AVERAGE & APPLY again". AVERAGE & APPLY gives "ROOM CORRECTION APPLIED" and the text says HOUSE. BYPASS lights and toasts "BYPASSED" (room_eq_on 0). NEW ROOM creates Test.json; RENAME gives Kitchen.json; DELETE removes the file and returns to My room.

**BUG: a word still being typed is ignored.** The name field ignores the keyboard's composing (underlined) word. "Test Room" typed became "Test", and RENAME did nothing until Enter committed "Kitchen". Read the preedit too, or commit it on button press (`Qt.inputMethod.commit()`).

**MINOR:**
- The keyboard stays open after NEW ROOM.
- The status line "Correction from 1 place (HOUSE)" stays in an empty new room.
- DELETE asks for no confirmation.
- Log: `getControl returning NULL for [DJMantra] room_current / room_eq_on` on every change.

Left BYPASS on, so the bad -12 dB correction isn't heard.

## V71 (67f8b89) installed 22:12 — parts 62, 51, 53, 54, 55, 57 (screen side)
- **62 PASS:** Songs top bar: FOLDERS / PLAYLISTS / LIST / FULL fit; filter field has room beside SUNO / TIDAL / FIND ON PHONE.
- **51 PASS (screen):** auto gain brings every song to -1 dBFS, e.g. Bissa peak +0.28 → gain -1.28, Cler Achel -0.84 → -0.16. Mixer column per deck: HIGH, MID, LOW, FILTER (blue), FX, headphones; no GAIN knob. Still open: two masters level on the VU meters; the Mix Ultra's FILTER knob; SHIFT+HIGH.
- **53 PASS (screen):** MID -16 in EQ, switch to NEURAL MIX gives "EQ → NEURAL MIX", VOCALS inner at 0, MID -16 kept on the outer. VOCALS cut to -13, back to EQ: MID -16 and VOCALS -13 both kept. Whether both are heard still needs Marko; the SHIFT part needs the controller.
- **54 PASS:** LOW -14, MID -16, VOCALS -13 and FILTER right on deck 1, then Diaraby Nene loaded onto deck 1. Everything is back at 0 / off, with "New song on [Channel1] : EQ, Neural Mix and FILTER reset". Deck 2 untouched.
- **55 PASS (screen):** a finger drag turns only the inner knob; the dropdown swaps inner and outer.
- **57 PASS:** inner dots in band colours (HIGH blue, MID green, LOW red), outer ring violet, violet number on top, band number at the bottom, 0 dB shown. Minor: the scale labels "−∞ / 0 dB" under each knob crowd the band caption below.
- The "BT" pill is gone because the Mix Ultra isn't connected (Bluetooth STATE_DISCONNECTED). 56 and the controller halves of 51/53 are waiting for it.

## V71, Mix Ultra over Bluetooth (22:29–22:50)
- **51 PASS (controller):** the Mix Ultra's deck 1 FILTER knob (B1 01 44→55) turned the screen's FILTER to the right.
- **BUG: connecting the controller presses BROWSE.** The first message after "Connected (bluetooth)" is `90 00 7F`, which the app treats as "controller: browse press" and opens the browser. That's probably the controller's state dump on connect. Ignore input for about 500 ms after connecting, or ignore note-ons that arrive with no matching release.
- **BUG: layout shift with the nav bar.** At 22:48 the Android nav bar appeared over the app. The Mixer shrank, captions overlapped the knobs, and the tab bar was drawn about 15 px higher than the app's tap map (tab y 2247). A tap at y 2232 on "Songs" did nothing; a tap at 2270 worked and immersive mode came back.
- **Marko's request (22:44):** "If there is any playlist with 0 songs, I want to remove it. I want only playlists which contain some songs." On the phone I deleted the empty folders in Music/djmantra_app/Library (no files in them, nothing else touched): HOTEL KORINJAK {by genre} (8 subfolders), made in Romania {by genre} (6), ECSTATIC-365 [by genre] (6), and 15 empty subfolders of SUNRISE DANCE {by genre} (MOTION PICTURE, which has a song, is kept). Playlists/ had no empty playlist (47 folders, 1–111 songs each).
  **Wish for the app:** FOLDERS / PLAYLISTS should never show a folder or playlist with 0 songs, counting subfolders. The app also kept showing the deleted HOTEL folder until I left it.
- **50 PASS:** the AutoMix tab's "AUTOMIX ▼" has a drawn arrow; the dropdown has a drawn ▲ collapse button, PREVIOUS SONG / NEXT SONG in words, no glyph boxes. Minor: MIX OVER shows 4 BEATS while the top bar's button says 8 BEATS. Are they the same setting?

## Stopped 22:55, 14.10.2026 (Marko away, controller off; Mac and phone shut down)
Still open in round 41: 56 and the SHIFT half of 53 (Mix Ultra), the listening halves of 51/53, 45–49 (BeatStats ISRC, AutoMix run, Suno GET after Marko's login, TIDAL login + SYNC ALL), 63 on V72+ (delete the bad "My room" first). V72–V75 not installed yet (CI was still running).
