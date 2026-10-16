# BeatStats browser on the cloud emulator (16.10.2026, Marko: "fix user interface, this part, and test it right in this chat in your emulator")

Emulator: android-30 ATD x86_64, no KVM (booted in 6.5 min after -wipe-data). The app's own BeatStats code (the package's
Java, unchanged) in a small test APK (`com.djmantra.bsold` = V82's code, `com.djmantra.bsnew` = V83's), BeatStatsActivity
as its only screen, theme Theme.Material.NoActionBar as in the app. The headless emulator draws no frames (screencap and
PixelCopy give nothing), so the layout is read from Android's own view dump (uiautomator), bounds in px of 1080x2400.

| | V82 (Marko's screenshot) | V83 |
|---|---|---|
| button row | `[0,0][1080,1279]`, buttons at y 576-702 | `[0,0][1080,138]`, buttons at y 8-134, four equal (258 px) |
| status | in the button row, ~0 px wide (the cause) | its own line `[0,138][1080,195]`; the longest status (READ CHART on a page with no songs) 2 lines, `[0,138][1080,235]` |
| the site (WebView) | `[0,1279][1080,2274]` = 43 % of the screen | `[0,195][1080,2274]` = 91 % (90 % with the 2-line status) |

Files: `layout_v82.xml`, `layout_v83.xml`.

READ CHART in the emulator's real WebView on the saved ORGANIC HOUSE chart (served to the emulator from the session):
**100 songs, "chart rows"**, title "ORGANIC HOUSE Top tracks". The ISRCs could not be fetched (no internet path for plain
requests from this emulator); that step was tested in Chromium (`../20261016_beatstats_cloud/`).

CAPTURE (V83): the screen copy fails on the headless emulator ("screen copy 3"); V83 then draws the browser into the image
itself. On a test page of a 60 px sticky header + 120 numbered 100 px colour bands: **1080x31658 JPEG from 16 screens**
(expected 12060 css px x 2.625 = 31657.5), the header **once** at the top, **bands 0..119 each once, in order**, each
258-263 px (expected 262.5). `capture_bands_top.jpg`: the top of it. On the saved chart page (unstyled offline, so its
icons are huge): 55 screens, stopped at the size limit (V83's status now says when a page was cut).
