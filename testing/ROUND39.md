# Round 39 (V30; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

Marko 13.10.2026 said yes to: (1) the real sound wave at deep zoom, as djay; (2) a depth slider for the effects.

## 0. Basics: PLAY / pause, faders, knobs, LEDs (quick look).
## 1. Deep zoom: load a song, wait ~5-10 s: log `Fine wave: N points, F frames each, for ...` (N ~ 2000 x the song's
seconds). Phrasing: zoom the lane in as far as it goes (pinch / zoom buttons): when the zoom shows under ~1.5 s across
the lane the envelope turns into the real wave (each kick a wiggle, the colour as before); zoomed out it is the
envelope again. Screenshots at 3 zoom levels + the same bars in djay zoomed in (when the Mac is awake). Time from
load to the log line on the Pixel, and any stutter while it is built (CPU, xruns).
## 2. FX depth: hold a deck's FX button: under the 4 effects DEPTH (a slider) or, for Echo Out, 1 BEAT / 2 BEATS.
Flanger: the slider = width (sweep depth); Underwater: left = open, middle = djay's, right = ~700 Hz; Cooldown: right
of ~60 % the deck fades out. Echo Out 1 beat vs 2 beats (log `FX: deck 1 ECHO OUT 1 beats`). The chooser opens above
the button, nothing cut. Record Underwater and Cooldown at left / middle / right.
