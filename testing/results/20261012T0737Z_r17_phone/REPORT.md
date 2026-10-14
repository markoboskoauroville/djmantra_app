# Round 17 on the phone (Nothing Phone (2a), A142), stopped at part 3

Build: Android run 38219484582, commit 7171a0a (the new interface is the default). Tested over adb, 12.10.2026.
**Stopped by Marko at part 3** with new requests (below), per testing-on-a-budget §1.2. For the builder (cloud session).

| Part | Expected | Result | |
|---|---|---|---|
| Smoke | `Quick: interface ready`, no QML errors, not black | `Interface: Qt Quick (GPU)`, `Quick: interface ready 413x919` on a normal start (no `--qml`) | PASS |
| 0 | the first start after the round 17 install opened the classic interface (stored `quick_ui` false) | fixed in 7171a0a (`classic_ui`); launch test fixed in 125b26e | FIXED |
| 1 portrait | top row, headers (total/remaining), controls, modes, matrix, 2 overviews, playlist, crossfader, tabs | all present (02, 11) | PASS |
| 1 landscape | decks left, rest right | as described (03) | PASS |
| 1 key | each deck's key in its own header | deck 2's key (8A) sits at the left edge of deck 2's column, reads as deck 1's | FAIL (replaced in round 18) |
| 1 matrix | elapsed / bar.beat / countdown apart | apart, no run-together | PASS |
| 1 overviews | readable waveforms | flat, low contrast (Marko) | FAIL (round 18) |
| 2 beats | 8 → 16 → 32 → 8, `Quick: phrase beats N` | log 8, 16, 32, 8; cells and bars as described (22) | PASS |
| 2 launch | deck 2 starts on deck 1's beat 1 | `Quick: beat launch [Channel2] on beat 1`, lit cells aligned (25, 35) | PASS |
| 2 cancel | second tap cancels | deck 2 stayed paused after arm + second tap; one tap alone launches | PASS |
| 2 jump | playing deck jumps to beat N in time | deck 1 moved +2 beats against deck 2 after a tap on cell 5 (35 → 36); no log line for it | PASS |
| 3–8 | | not tested: stopped for round 18 | — |

Marko during the round: "play stop button doesn't work" (not reproduced over adb: `input tap` on PLAY played and paused
every time). Suggestion: toggles (PLAY, SYNC) act on the press, not on the click (DButton.qml toggles in onClicked, so a
finger that slides a little loses the press), and log the change so the tester can see Marko's own presses.

## Marko's requests for the next build (12.10.2026, word for word)

> "First update. User interface. Numbers. Running numbers of the song. I need time until the end of the song. Not how
> much I'm into the song. It needs to go backwards. Now, since we have now the time, everything in matrix display. In
> this display of the song data, we don't need time. Nothing. Just delete that part. So first line will be artist
> name, second line song name, third line if it's not there you need to find with connecting this up to the internet,
> genre of the song. Then sections need to be clearly seen. So the waveform is not good, it's too— looks— the
> contrast are not good. Make better waveform display when I can clearly see the sections of the song. And make this
> normal waveform display with— I mean, it can be also only half of the waveform, that's okay, but it needs to be much
> bigger, filling up the whole screen so I can really see the frequencies and the different colors, and make sure I
> can access that app detects— basically, that app runs in full screen over the Android. If I want to navigate, I just
> swipe my finger from the bottom of the screen up, and this 3-button navigation will come up. Hopefully. If not, we
> need to put some button to bring it back. Maybe clicking with finger, tapping on empty space with no buttons will
> invite this navigation from the Android phone. And any interaction with the screen will just remove it, hide it. And
> Camelot wheel in the key section needs to be big so I can see it clearly. Now it's too small. So fix this bug with,
> uh, interface which is not full screen and improve time counting down, and that's all. I'm not sure what this
> button— play stop button doesn't work. I think you noticed that. It's good that this— so at the bottom I need the
> fourth button, we can call it phrasing instead of playlist. Then I will have bigger LEDs for song beats in matrix
> style, but they are big so I can jump easily. In this small one, when there are many, when there is 16-bit, it's
> harder possible to jump. When it's 32-bit, almost impossible. So we need big user interface. That's the next tab,
> actually. That will be a second tab. Please fix this and then test again."

As a list:

1. Matrix display, left field: the time left in the song, counting down (now elapsed).
2. Deck headers: no times. Line 1 artist, line 2 title, line 3 genre; when the file has none, look it up online.
3. Overview waveforms: better contrast; the sections clearly visible.
4. A big "normal" (scrolling) waveform, half a waveform is fine, much bigger, frequencies visible as colours.
5. Full screen over Android: a swipe up from the bottom brings the 3-button navigation; a tap on empty space brings it
   too; any touch hides it again.
6. Keys tab: the Camelot wheel big (now ~40 dp: `WheelPage.qml` sizes it `height - 220`, room kept for help text
   that the embedded page does not need).
7. PLAY / pause: "doesn't work" for Marko (see above).
8. A fourth tab, second in the row: **Phrasing**, big matrix-style beat LEDs, easy to tap at 16 and 32 beats.

Tester's findings for the builder:
- Full screen: `hideBars()` runs only at start (`Component.onCompleted`). Coming back from another app nothing hides
  the bars again; hide them on `Qt::ApplicationActive` too.
- Genre: the APK has Qt's OpenSSL plugin but no libssl/libcrypto, so HTTPS from QML/Qt will fail; Android's own
  `HttpsURLConnection` (Java) works. The manifest has no `INTERNET` permission yet.
- The key of deck 2 is drawn at the inner edge of deck 2's column, next to deck 1's key: it reads as deck 1's.
