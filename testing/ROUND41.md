# Round 41 (V42; Pixel 7; rules: testing/PROTOCOL.md)

Marko 14.10.2026: "write the length of the song overlaid over the album art ... similar as YouTube ... the library
needs to have two views: list view and thumbnail view."

## 0. Basics: installs, starts, `Quick: interface ready`, PLAY / pause both decks.
## 1. Songs tab, LIST view: every song's cover has its length bottom right, white on a dark box (3:07, 1:02:45);
songs without a cover too (on the note). Sort by TITLE / BPM: the lengths stay with their songs (they were lost by a
sort before). Compare 3 lengths with the deck's time display after loading.
## 2. The view button in the top bar (left of the stats): THUMBNAILS: 3 covers a row (portrait; more in landscape),
title and artist under each, the length bottom right on the cover, BPM and key top left; played songs dimmed with a
tick; a tap loads onto the free deck; folders open. Tap again: LIST. Leave the app and come back: the view is kept
(log `library view: ...`). The controller's BROWSE knob / LOAD work in both views.
## 3. Screenshots of both views (downscaled) into the results folder.
## 4. r40 fixes: right after start HIGH / MID / LOW on both decks are in the middle (log: no EQ at max); the
volume fader: its label must say VOLUME while testing it (a still hold of 0.5 s flips it to PITCH, which would explain
r40's "fader barely changes the peak" and "90.07 BPM": grep `fader [Channel` in logcat).
## 5. r40 part 6: the PITCH scale's middle = the deck's BPM where the song is (local_bpm), the same number as the
handle at 0 % (was the library's 114.6 while the deck played at 91).
## 6. Marko 14.10.2026, the knob rules and toasts. The EQ is CUT ONLY: the TOP of HIGH / MID / LOW is 0 dB (no
cut, the reset and start position); r40's "EQ at max at start" was right. Now:
- a LONG PRESS (0.5 s, finger still) resets every knob / fader without another long-press job: HIGH / MID / LOW
  -> top (0 dB), the crossfader -> middle, the FX DEPTH -> 50 %; no reset on a double tap there any more;
- GAIN (long press = GAIN <-> FILTER) and the volume faders (long press = VOLUME <-> PITCH) reset on a DOUBLE TAP;
- every such change shows a TOAST near the top for 1.4 s (log `toast: ...`): "DECK 1  LOW → 0 dB",
  "DECK 1  GAIN → FILTER", "DECK 2  VOLUME → PITCH", "DECK 1  PITCH → 0 %", "CROSSFADER → middle",
  "DECK 1  FX → DJM ECHO", "DECK 1  FLANGER ON", "EQ → NEURAL MIX", "SORTED BY LENGTH ▲", "VIEW → THUMBNAILS".
## 7. Songs: a LENGTH column (sortable, tap again = reverse; songs without a length last).
## 8. PITCH on DECK 2: the BPM numbers on the left of the scale are there (they were hidden under the VU meter), the
% on the right; both decks' scales fully readable (no number cut).
## 9. RECORDING (Marko 14.10.2026): Menu -> "● RECORD MIX" starts recording the master output, lossless FLAC
(Settings -> RECORDING: FLAC / WAV, and the NAME, default "DJ Mantra Mix") into /sdcard/Music/DJ Mantra Recordings as
"DJ Mantra Mix - 2026-10-14 15h30.flac" (log `Recording: started ...`, toast "● RECORDING ..."). The hidden easter
egg: a LONG PRESS on the CLOCK starts / stops it too, and the clock is RED while recording. Stop (menu or clock): the
toast "RECORDING SAVED: ...", log `Recording: stopped <path> <bytes>`. Pull the file: plays, 48 kHz stereo, the length
right, the sound = what was heard (EQ / FILTER / FX moves audible in it). Change the name in Settings: the next file
has it.
## 10. AUTOMIX / EQ / TEMPO BLEND (Marko 14.10.2026): EQ and TEMPO BLEND work whenever lit, also on a crossfader
moved by hand (`automix eq ...%` lines while dragging, AUTOMIX not pressed). AUTOMIX is an ACTION: a tap glides the
crossfader from the end it rests on to the other over Settings -> AUTOMIX FADE (4-64 BEATS of the outgoing deck, or
2-30 SECONDS; toast "AUTOMIX → DECK 2 (16 beats)", log `automix: crossfader -1.00 -> 1 in 7742 ms`); the button shows
the %; a tap while it moves stops it (toast). Time it with a stopwatch: 16 beats at 124 BPM = 7.7 s; 10 s.
## 11. AUTO-PLAY WHEN MOVING CROSSFADER (djay's option; Settings, default ON): fader at deck 1's end, deck 2 paused
with a song: moving the fader towards deck 2 starts deck 2 at once, on the beat (toast "AUTO-PLAY → DECK 2", log
`crossfader auto-play`). Not when the deck plays already or is empty; once per move away from an end. OFF in
Settings: nothing starts. Compare with djay's own (Settings -> Crossfader FX -> Auto-play when moving crossfader).
## 12. CONTROLLER LINK (Marko 14.10.2026): a green USB / BT pill on the menu button's corner while the Mix Ultra is
connected, nothing when not; toasts "CONTROLLER CONNECTED · USB" / "· BLUETOOTH" / "CONTROLLER DISCONNECTED" (log
`controller link: ...`). Switching: on Bluetooth, plug the USB cable (phone <-> Mix Ultra): it switches to USB at once
(logcat DJMantraMIDI `USB plugged in: switching from Bluetooth to USB`); unplug: DISCONNECTED, then within ~3-10 s
back on Bluetooth (the scan). Power off the controller: DISCONNECTED; on again: connected. After each switch: move
the crossfader, a volume fader, an EQ knob, press PLAY: the screen follows (Marko moves them; one action at a time).
## 13. Console SYNC (r40 part 9: "SYNC from the console did not register"): a press on the Mix Ultra's SYNC toggles
the deck's sync lock, the same as the screen's SYNC (it lights / unlights; log `MixUltra: SYNC [ChannelN] -> 1/0`).
## 14. TABS = the Mix Ultra's pad-mode buttons (Marko 14.10.2026): the tab bar is now Songs | Phrasing | Mixer |
AutoMix | Keys. On the controller (either deck): HOT CUE -> Songs, LOOP -> Phrasing, FX -> Mixer, NEURAL MIX ->
AutoMix, SHIFT + NEURAL MIX (SAMPLER) -> Keys (log `controller: page 1..5`). Also the key badge (top left) opens Keys.
## 15. FILTER and FIND ON PHONE (Marko 14.10.2026), above the song list: type in "Filter this folder": the list (or
thumbnails) shows only the folder's songs whose title / artist / file name has every typed word, at once while
typing; ✕ clears; sorting still works. "FIND ON PHONE": the same words through every song on the phone (Android's
media index: phone, SD card, USB stick), at most 300, toast "FOUND ON PHONE: N" (log `find on phone ...`, logcat
DJMantraStorage `findSongs`); a tap loads the hit onto the free deck; typing more searches again. Also from the
Folders level. Opening another folder clears the search. Check with a song in Download and one in Music.
## 16. BEATS = a STEADY GRID by default, as djay (djay compare: Aum Ganesha djay 115, ours drifted 91 / 109.9 /
117.8). Settings -> BEATS: "STEADY GRID (default, as djay)" / "FOLLOW LIVE DRUMS". Songs analysed into a beat map
before are analysed again (logcat `Beats: ... a beat map, the steady grid is chosen: again`). Aum Ganesha: the deck
shows ~114.6-115 at the start, the middle and the end.
## 12b. (V43) the controller badge again: V42 showed no pill (the control was made after the screen bound to it).
At start with the Mix Ultra on USB: the green USB pill at once; then the switching steps with toasts.
## 17. (V43) r41 part 15/16 fixes: the FILTER follows every letter while typing (Gboard's composing word too); the
clear button is a visible "×"; FIND ON PHONE hits get their BPM, key and cover after a moment. A song whose tempo
drifts gets its AVERAGE tempo on the steady grid (logcat `Beats: the tempo drifts; the average ...`): Aum Ganesha
~115 as djay (was 116.7); every song analysed before is analysed again once when loaded (`an older grid: again`).
## 18. (V44) The overview strips (whole song): no zoom window any more while dragging (Marko 14.10.2026: the
Phrasing tab already shows the zoomed waveform). A touch puts the play position under the finger, a drag scrubs
freely through the whole song (log `scrub [ChannelN] to ... s` on release); AUTO PHASE waits while dragging.
## 19. (V44) Dragging the ZOOMED waveform (Phrasing): the wave follows the finger, so zoomed out it runs fast through
the song and zoomed in slower (it was the other way round); at the deepest zooms it never crawls (at least 0.25 s per
100 px). Check at zoom 6, 3, 1, 0.25, 0.04: a full-width drag moves about width/100 x zoom seconds (0.25 at least).
## 20. (V45) TEMPO (Marko 14.10.2026): the screen fader's second mode is called TEMPO (label, toasts "VOLUME → TEMPO"),
as on the Mix Ultra. The console's tempo slider sets the tempo where it stands at its first move (no soft takeover), and
the screen's TEMPO fader follows it: move the console slider, the screen handle and BPM move with it (same direction?).
## 21. (V45) CONTROLLER CONNECTION in Settings: USB CABLE / BLUETOOTH, one or the other (Marko 14.10.2026; default
USB). USB: the app connects only over the cable (logcat DJMantraMIDI `Waiting for the controller on USB`, then
`Connected (usb)`), no Bluetooth scan. BLUETOOTH: only the Bluetooth scan (`Looking for the controller over Bluetooth`),
a plugged cable is ignored. Switching in Settings drops the link and connects on the other (toast, the pill USB / BT).
## 22. (V45) Faders and knobs over BLUETOOTH (r41: only buttons worked: the Mix Ultra sends the 14-bit MSB alone over
BLE, Mixxx waited for the LSB, "mis-matched LSB/MSB ... Ignoring both"): now every fader, EQ knob, crossfader and the
tempo slider work over Bluetooth too (7-bit steps there); no "mis-matched" warnings. Compare USB vs BT: same moves.
## 23. (V46) EQ knobs in dB (Marko 14.10.2026): under each HIGH / MID / LOW ring "−∞" at the left end (full kill) and
"0 dB" at the right end (no cut, the top); while cut, the caption shows the cut ("LOW −12 dB", in the knob's colour).
Turn LOW down step by step: −1 ... −6 ... −20 ... −∞; long press: back to "LOW" (0 dB). Nothing overlaps.
## 24. (V46) Recording time under the clock (Marko 14.10.2026): while recording, the clock has two rows in the matrix
font: the time of day on top, the recording's time under it (0:00, 0:01 ... 12:34, 1:02:45 after an hour), both red;
stop: back to one row. Start / stop by a long press on the clock or Menu -> RECORD MIX.
## 25. (V46) The menu (top left) has CONTROLLER: USB / CONTROLLER: BLUETOOTH, the same switch as Settings (lit = the
current one; a tap switches, toast "CONTROLLER → USB / BLUETOOTH"). Settings and the menu show the same choice.
## 26. (V46) Pad-mode buttons -> tabs again (r41 part 14 failed: the script's pulse was back at 0 before the screen
read it). HOT CUE / LOOP / FX / NEURAL MIX switch Songs / Phrasing / Mixer / AutoMix (`controller: page N`), SHIFT +
NEURAL MIX (= SAMPLER, 91 16) -> Keys; the same button twice after a screen tab tap works again. Also check BROWSE
turn / press and LOAD (same kind of message): if they fail too, say so.
## 27. (V47) TEMPO direction (r41 part 20 failed: opposite to the console). The screen's TEMPO handle now sits where
the console's tempo slider is: console bottom = screen bottom. Faster is DOWN on both (as Pioneer, Mixxx's default:
"+" % and the higher BPM are at the BOTTOM of the scale, "−" at the top). Move the console slider down: the handle goes
down, the BPM rises; drag the screen handle down: faster too.
## 28. (V48) GAIN in dB (Marko 14.10.2026): under the GAIN ring "−12" at the left end and "+12" at the right end
(Mixxx's pregain; a DJM's TRIM is −∞ .. +9 dB); straight up = 0 dB = just "GAIN"; turned, the caption shows the gain
("GAIN +4 dB", "GAIN −6 dB", yellow). Flipped to FILTER: no dB labels. Double tap: back to 0 dB.
## 29. (V48) The last 30 s of a song (Marko 14.10.2026): in the matrix, the deck's time left ("-0:30") and its
countdown turn RED from 30 s before the end; a new song (or jumping back) brings the usual colours back.
## 30. (V48) CUE held 1 s (Marko 14.10.2026): jumps to the very start of the song (toast "DECK 1 → START OF THE SONG",
log `cue held 1 s`), the deck paused there; a short CUE press works as before (back to the cue / preview).
## 31. (V49) TEMPO RANGE as a Pioneer CDJ (Marko 14.10.2026): under a TEMPO fader the label reads "TEMPO ±10" (the
default); a tap steps ±6 -> ±10 -> ±16 -> WIDE (±100) -> ±6 (toast "DECK 1 TEMPO RANGE → ±16 %"), the scale's BPM / %
follow, and the tempo playing stays the same where it fits. Settings -> TEMPO RANGE sets both decks. Remembered.
## 32. (V49) "No snapping anywhere, always ramping" (Marko 14.10.2026): every long press is 1 s (knob / fader resets,
GAIN <-> FILTER, VOLUME <-> TEMPO, FX chooser); every reset GLIDES there, easing in and out (~0.5 s): EQ to the top, GAIN
to 0 dB, crossfader to the middle, volume to the top, TEMPO to 0 %. TEMPO RESET as a CDJ: a tap on the 0 % mark of a
TEMPO fader glides the tempo to 0 % (toast "DECK 1 TEMPO → 0 %"). The AUTOMIX glide eases in and out too.
## 33. (V49) AUTO GAIN as djay (Settings, on by default): each song's peak is measured (logcat `Auto gain: <song> peak
-x dB, gain +y dB`) and played at -1 dBFS (+-12 dB at most): quiet and loud masters come out at the same peak. Songs
analysed before are measured again once. OFF in Settings: as before. The GAIN knob stays at 0 dB (it adds on top).
## 34. (V49) AutoMix PREVIOUS / NEXT (Marko 14.10.2026): with AutoMix running, NEXT: the next song (already waiting)
is mixed in on beat 1 of the playing deck's NEXT PHRASE (toast "NEXT: <song> ON THE NEXT PHRASE", log `AutoMix: next`);
PREVIOUS: the song before is loaded onto the waiting deck and mixed in the same way; after it, the order goes on from
there. Also: AutoMix's bass swap now returns the EQ to 0 dB (the top), not the middle (a cut).
## 35. (V49) Battery line (Marko 14.10.2026): a thin vertical line between the clock and the CPU / GPU meter, filled
from the bottom with the battery %, green; red at 10 % or less (checked once a minute).
## 36. (V50) Thumbnails: BPM and key moved to the BOTTOM LEFT of the cover, in the same dark box and white bold text as
the length at the bottom right (Marko 14.10.2026); nothing on the top left any more.
## 37. (V50) Controller BROWSE / LOAD / BACK (r41: no reaction; the script's set-and-reset pulse was read as 0): BROWSE
turn moves the highlighted song (log `controller: browse N`), press opens a folder / loads, LOAD A / B loads onto that
deck, BACK goes up; the list's view button reads LIST / GRID (no empty boxes).
## 38. (V50) PLAYLISTS (Marko 14.10.2026): the Songs tab's top level has FOLDERS | PLAYLISTS. PLAYLISTS lists the app's
playlists (NAME, CREATED, SONGS, LENGTH: sortable, filterable, LIST / GRID like folders); M3U / M3U8 files in
Music/Playlists are taken in once (logcat `Playlists: imported ...`, djay's exported ones). A playlist opens like a
folder (its songs in its order; sort / filter / grid / length / % as a folder); ← back to the playlists. A LONG PRESS
(1 s) on any song (folder, playlist, phone search, list or grid) opens ADD TO PLAYLIST: every playlist (with its song
count), a NEW PLAYLIST name + CREATE, inside a playlist REMOVE FROM <it>, CANCEL; toasts ADDED TO / ALREADY IN /
REMOVED FROM. The playlists survive a restart.
## 39. (V50) FULL / DOCK button in the song list's top bar (both views): FULL lays the list over the decks (top bar,
crossfader and tabs stay), DOCK puts it back (toasts).
## 40. (V51) The app's ROOT FOLDER (Marko 14.10.2026): /sdcard/djmantra_app (made at the start). The Songs tab opens IN
it; ← shows djmantra_app, Music, Download. Playlists: djmantra_app/Playlists/<name>.m3u8 (songs in
djmantra_app/Playlists/<name>/) are taken in under PLAYLISTS (Music/Playlists too). Recordings go to
djmantra_app/Recordings.
## 41. (V52) DOWNLOAD QUEUES (Marko 14.10.2026; Menu -> DOWNLOAD QUEUES): SPOTIFY / TIDAL / YOUTUBE MUSIC, each a text
file in /sdcard/djmantra_app/Download_Queue/ (Spotify.txt, TIDAL.txt, YouTube_Music.txt), a line per song
"artist | title | link" (the service's search link). The page lists the queue (title, artist), OPEN opens the link
(the service's app or browser), × removes the line; the file path is shown (copy / share it from the Files app).
Songs are added from the BEATSTATS charts (next; for now check with a file pushed by adb).
## 42. (V52) BEATSTATS (Marko 14.10.2026). Settings -> BEATSTATS: type the Groq API key once (hidden; stays on the
phone; Marko types it, never in a message or the repo). Menu -> BEATSTATS CHARTS -> OPEN BEATSTATS: beatstats.com in a
browser inside the app (pass the site's check once by hand; it reopens on the last page); open a chart (e.g. ORGANIC
HOUSE), READ CHART: the bar says "<chart>: N songs read" (logcat DJMantraBeatStats); CLOSE. The app's BEATSTATS page
shows the chart (rank, title, artist, label), the charts read as buttons to flip between; per song SPOT / TIDAL / YT M
open that service's search for artist + title (YouTube MUSIC, not videos); + adds it to a download queue (Spotify /
TIDAL / YouTube Music) -> Menu -> DOWNLOAD QUEUES shows it, in djmantra_app/Download_Queue/<service>.txt.
## 43. (V53) The app's ROOT moved INTO Music (Marko 14.10.2026: "one master folder of this app"): /sdcard/Music/djmantra_app
with Playlists/<name>/ + Playlists/<name>.m3u8, Library/, Recordings/, Test/, Download_Queue/, BeatStats/. The Songs tab
opens in it; ← shows djmantra_app, Music, Download.
## 44. (V54) TIDAL PLAYLISTS both ways (Marko 14.10.2026; official TIDAL API v2, OAuth PKCE with the public Client ID,
redirect http://localhost:8765/tidal-callback caught by the app; tokens only in the app's private storage). Songs ->
PLAYLISTS -> TIDAL: LOG IN TO TIDAL opens the browser, log in, "TIDAL is connected", back to the app: the TIDAL
playlists list (name, count). TAKE IN: the playlist appears among the app's playlists with the songs found on the phone
(Music, by file name), the rest go into the TIDAL download queue (status line: "N on the phone, M into the queue").
Inside one of the app's playlists -> TIDAL -> SEND "<name>" TO TIDAL: a new TIDAL playlist of that name with the songs
TIDAL finds ("found/total"); needs the playlists.write permission (if TIDAL refuses: the status says so; then the
developer dashboard has to allow it, Marko). logcat DJMantraTidal. LOG OUT clears the tokens.
## 45. (V55) BEATSTATS WITHOUT AI (Marko 14.10.2026: the exact record by its unique code). The Groq key is gone from
Settings. Menu -> BEATSTATS CHARTS -> OPEN BEATSTATS -> a chart (e.g. ORGANIC HOUSE top tracks) -> READ CHART: the bar
says "<chart>: N songs read, finding ISRCs ...", then "ISRC k/N", then "N songs, k ISRCs" (rows read from the page
itself: rank, title, artists, the Beatport track ID; each ISRC from its Beatport page). CLOSE: the chart shows
"artist · ISRC <code>" per song. SPOT opens Spotify's search for isrc:<ISRC> (the one recording); TIDAL opens the exact
TIDAL track (tidal.com/browse/track/<id>) when logged in to TIDAL, else TIDAL's search; YT M searches artist + title.
+ puts the same exact link in the queue file. logcat DJMantraBeatStats (report if the ISRCs stay 0: Beatport blocked).
## 46. (V56) AUTOMIX TAB = THE SONG LIST + THE DJ ROBOT (Marko 14.10.2026: "it looks same as songs with only just few
additional buttons"). The AutoMix tab shows the same song list as the Songs tab (the same folder / playlist, sorting,
filter, LIST / GRID, FULL / DOCK). Its top bar has AUTOMIX ▾: a tap drops down the AutoMix settings (what it plays:
the folder / playlist open, in the order shown; MIX OVER 4-64 BEATS; START / STOP AUTOMIX; PREVIOUS / NEXT; the
status), ▴ or a tap beside it collapses it; START collapses it by itself. While it runs: the playing song's row is
lit "PLAYING", the next one "NEXT", a status line at the bottom ("AUTOMIX Playing ..., mix in N s"); a song tapped in
the list is mixed in next on the next phrase (toast "NEXT: ... ON THE NEXT PHRASE"). The Songs tab has no AUTOMIX
button and loads songs as before.
## 47. (V57) SUNO PLAYLISTS (Marko 14.10.2026: paste Suno links, the app remembers them). Songs -> PLAYLISTS -> SUNO:
the playlist name (Suno, or the playlist open), a box for the links: copy a Suno song link (Suno app: Share -> Copy
link, e.g. https://suno.com/song/<id> or https://suno.com/s/<code>), PASTE (several links at once work), ADD TO SUNO.
"N links remembered, saving the songs ...", then "Saved: <title>" per song, then "N songs in Suno"; toast. The list
under it shows the remembered links (title · playlist). × closes: PLAYLISTS shows "Suno" with "SUNO PLAYLIST · N songs";
inside it the songs (files in /sdcard/Music/djmantra_app/Suno/<title>.mp3) load, play, analyse and mix as any song,
AutoMix too. The same link again: not saved twice. logcat DJMantraSuno (report "no audio on ..." errors: a private song).
## 48. (V58) SUNO WITH THE OWNER'S LOGIN. Tested in the cloud on Marko's 4 links: Suno gives the AUDIO only to the
logged-in owner ("audio_url": ".../api/forbidden", "is_public": false), so ADD now REMEMBERS each link with its TITLE
("Remembered: 1 svemir, one day porcupine tree (GET it ...)"). Each such song has GET: Suno's song page in a browser
inside the app: log in to Suno ONCE (the login stays; Google's login may refuse an in-app browser: then e-mail /
Discord / Apple), the song's ... menu -> Download -> MP3; the bar says "Saved: <title>.mp3 (N KB)"; DONE: the panel
finds it, the song is in the Suno playlist (GET disappears, the row turns white). ALSO: a song downloaded with the
Suno app or Chrome into Download is found by its title when the SUNO panel opens (copied into djmantra_app/Suno).
Report what the Download does in the in-app browser (saved or nothing): logcat DJMantraSuno.
## 49. (V58) TIDAL, NEW APP (Marko's own developer app "DJ Mantra", Client ID JjTAmDVohiV7GgOL, built in). Settings ->
TIDAL: "Client ID JjTA...OL"; the key files in Music/djmantra_app/Keys, Download, Documents as buttons: a tap reads ONLY
the Client ID from it (toast "TIDAL KEY FILE LOADED: LOG IN AGAIN"; the secret is never read). Songs -> PLAYLISTS ->
TIDAL -> LOG IN (PKCE) -> his TIDAL playlists; TAKE IN one; SYNC ALL PLAYLISTS TO TIDAL: every app playlist to TIDAL
(one after another, "found/total" each, then "N playlists synced to TIDAL"); SYNC again: no duplicate playlists, only
missing songs added. Check on TIDAL (app or web) that the playlists are there with the songs.
## 50. (V59) The AUTOMIX button in the AutoMix tab's top bar: "AUTOMIX" with a small drawn arrow pointing down
(was an empty box); in the dropdown, the collapse button shows a drawn arrow pointing up; PREVIOUS SONG / NEXT SONG
in words (no glyph boxes anywhere in the AutoMix dropdown).
