# Round 35 (V15; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

Marko's djay screenshots (zoomed lane, 13.10.2026): djay's lane wave is TALL (~90 % of the lane), each hit an arrow
(sharp attack, fading tail), strongly saturated (red kicks, beige / yellow mids, blue / violet highs, green breaks),
translucent edges, no dark outline. V15: lanes 0.9 / scrub 0.85 of the height, saturation x1.4, a solid core fading
to translucent edges. Cloud preview: testing/djay/v15_lane_nanana.png.

## 1. Side by side, the SAME moment and zoom (Marko asked for more detail): in djay Pro on the Mac, zoom the lane
so ~6 bars show (as Marko's screenshot, bars 125-130), screenshot 3 spots per song (a drop, a break, an intro) for
Nanana, 4B, Brick House; the app's Phrasing lane at the same bars (screenshot). Crop both to the lane, push
testing/results/<date>_r35_pixel/pairs/. Also note djay's waveform settings (Preferences: waveform colour mode /
style) and its zoom level.
## 2. Install Allen & Heath's CQ MixPad app on the Pixel (Play Store) and use it to control the CQ (Marko): it
replaces the Mac's MixPad for the CQ tests on the phone.
## 3. Then testing/CQ12T.md part B and ROUND32 part 5 if not done.

## 4. STOP TIME (new in V15, Marko): Settings -> STOP TIME OFF / 0.5 / 1 / 2 / 3 s. OFF: PLAY on a playing deck
stops at once. 1 s and 3 s: the deck slows down as a turntable (the pitch falls) and stops after ~1 / 3 s (log
`turntable stop [ChannelN] 1 s`; record the sound). PLAY again during the slow-down: it plays on at its speed. A
synced deck (tempo not 100 %) starts the slow-down from its own tempo (no jump). The controller's PLAY does the same.
