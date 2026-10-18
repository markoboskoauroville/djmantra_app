# Round 43: V91 on the Pixel 7 (rbxport's beat grid, named sections)

Tester: the local Claude Code session on the Pixel 7 (adb over wireless debugging, see docs/HANDOVER.md).
Screenshots to Download/djm_v91/. Report each part when done (PASS / FAIL, what you saw, logcat lines).

0. Install V91 (release "android-latest", phone-arm64); versionName V91; starts; crash grep empty.
1. Every song is analysed again once (the new grid): load 3 songs one after the other (Songs -> Library). logcat
   `Beats: <song>  rbxport <bpm> BPM (confidence ...), downbeat ... s, N phrases; 0.0 INTRO, ...` for each, and no
   crash. Note how long the analysis takes per song (the deck shows the waveform when it is done).
2. The grid on the kick: play each song, Phrasing tab, screenshot the scrolling waveform in a part with drums: the beat
   lines sit at the front of each kick (the bar.beat counter and the phrase lights run in time with the kicks).
3. Beat 1: the bar.beat counter shows 1.1 on a strong downbeat (where a new phrase starts); note any song where 1
   is on another beat (the "1 is here" button / SHIFT+pad 5 on the Mix Ultra fixes it by hand).
4. Section names: the overview lanes show the names (INTRO, BREAKDOWN, BUILD-UP, DROP, MAIN, OUTRO) at the section
   starts; logcat `Sections: [ChannelN] ... s INTRO MAIN BREAKDOWN DROP ...`. The next section's name is yellow.
   A song with a clear breakdown (the beat stops) shows BREAKDOWN there and DROP where the beat comes back.
5. Settings -> SECTIONS: tap RBXPORT PHRASES: the sections change (logcat `named (rbxport)`); back to ENERGY CHANGES.
   Settings -> BEATS: RBXPORT GRID on (default); off + ANALYSE THE STOPPED DECKS AGAIN -> the old grid (QM) comes back;
   on again + analyse again.
6. 2 minutes of play, then the crash grep. Send "round 43 finished".
