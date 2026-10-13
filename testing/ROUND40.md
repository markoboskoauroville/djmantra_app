# Round 40 (V34; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

Marko 13.10.2026 (night, built untested): (1) the FILTER knob is gone from MIXER, the other controls are bigger (4 knobs,
FX and headphones buttons 52 px) down to the CQ DELAY line; (2) a double tap resets a control: GAIN / HIGH / MID / LOW to
the middle (0 dB; the EQ knobs went to the maximum before), the volume fader to the top; (3) a new 5th tab AUTOMIX:
the folder open on the Songs tab, in its sort order: START plays song 1 on deck 1 with song 2 waiting on deck 2;
BEATS (4 / 8 / 16 / 32 / 64) before the playing song's end (its last sound) the next starts in sync on the beat, the
crossfader moves across over those beats, the basses swap at the middle, then the old deck stops and loads the song
after. Log lines `AutoMix: ...`.

## 0. Basics: PLAY / pause, faders, knobs, LEDs.
## 1. MIXER: no FILTER, nothing overlaps, knobs bigger; double tap each knob: middle; volume fader: top.
## 2. AUTOMIX: Songs -> open a folder with 4+ analysed songs, sort by BPM; AutoMix tab: 8 BEATS, START. Watch / record
2 transitions: the crossfader, beat alignment (no flam), the bass swap, the next song loading; switch tabs during a mix
(it goes on); STOP. Then 32 BEATS. The last song: "The playlist has ended".
## 3. V35 (Marko): the Songs header shows, instead of "tap a song: free deck", the playlist in the clock's matrix font:
"<songs> <total time>" and "<analysed %>" (green at 100 %). The analysis runs only in the open folder, starting 3 s
after it is opened (log `Analysis: folder ...`), and stops when it is left (`Analysis: stopped`); opening it again
goes on. Check: a new folder 0 % -> 100 %, leave halfway (stops), come back (goes on from where it was).
