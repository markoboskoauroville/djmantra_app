# BeatStats scraping, tested from the cloud session (16.10.2026, V81 e46cc80)

Marko: "Please test the beatstats.com scraping and getting songs list in your local emulator."

| check | result |
|---|---|
| beatstats.com from the cloud (after Marko allowed beatstats.com / beatport.com) | **Cloudflare human check** ("Performing security verification ... Verify you are human", HTTP 403, `beatstats_cloudflare_check.png`), in Chromium with the Pixel 7 profile. It needs a person to tick it; not bypassed (by design the app's browser is passed by hand once and keeps the cookie, r41 part 61 PASS on the phone). So the live chart cannot be read from the cloud. |
| READ CHART's JavaScript (V81, taken from the compiled class) on the saved TRACKS chart page | **PASS**: 100 songs, "chart rows", title / artist / Beatport ID right (Don't This / Fairtone / 18983652, ...). |
| the same with the chart rows' markup removed (another page or layout) | **PASS**: 110 songs via "track links" (the 100 + 10 other track links on the page), title and artist right. |
| CAPTURE PAGE's scroll / crop / join steps, Chromium, 412x780 at DPR 2.625, a 56 px fixed header added | **PASS**: header found (56), screens joined without gaps or a repeated header (seams checked). |
| ISRC step: Beatport track page by plain HTTPS (as `BeatStatsActivity.isrc()`), from the cloud | **Cloudflare check too** (HTTP 403 "Just a moment..." for 18983652 and 22385734). From the Mac on 14.10 the page loaded (`testing/results/beatstats/beatport_track_22357262.html`). **Risk:** if Beatport challenges the phone as well, the chart reads but gets no ISRCs (status "N songs, 0 ISRCs"). To check on the phone in part 80. |
| the app on the cloud emulator (android-30 ATD x86_64, no KVM) | booted in ~8 min, V81 x86_64 downloaded, the proxy CA added; Android's network stack missed its start deadline in software (ANR) and took system_server down again and again (`Lost network stack`, the manifest's note), so the app was never installed. Even when up, it would stop at the same Cloudflare check with nobody to tick it. |

Next (phone, part 80): READ CHART on the page Marko used, the status line, the saved page if 0; a TRACKS chart:
100 songs and how many ISRCs; CAPTURE PAGE.
