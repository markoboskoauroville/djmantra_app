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
## 51. (V62) NORMALIZED, NO GAIN KNOB, FILTER AT THE BOTTOM (Marko 14.10.2026: "every file is normalized to -1 dB.
Gain slider doesn't make any sense ... I have faders for volume ... filter is at the bottom, the same as in my
controller"). Every song's peak is brought to -1 dBFS in the analysis, always (no switch in Settings; logcat "Auto gain:
... peak X dB, gain Y dB"; songs V60/V61 had moved to 0 dBFS return to -1 dBFS without a new analysis: "Normalized: -1
dBFS again for ..."). Two very different songs (a quiet and a loud master) peak at the same level on the VU meters with
the faders up. The Mixer tab, per deck from the top: HIGH, MID, LOW, FILTER (blue; middle = off, left low-pass, right
high-pass; long press = back to off, gliding, toast "DECK N  FILTER → OFF"), then FX and the headphone button. No GAIN
knob. The Mix Ultra's FILTER knob moves the screen's FILTER; SHIFT + HIGH does nothing (no gain).
## 53. (V63) EQ AND NEURAL MIX ARE SEPARATE KNOB SETS; SHIFT = THE OTHER SET (Marko 14.10.2026). The three bands (LOW /
MID / HIGH) and the three stems (DRUMS / VOCALS / HARMONIES) now have their own values and both act at once. Mixer tab:
cut MID to the bottom in EQ, switch to NEURAL MIX: the knobs show DRUMS / VOCALS / HARMONIES at 0 dB (full rings), the
MID cut stays audible; cut VOCALS; back to EQ: MID still cut, the vocal cut still audible. The Mix Ultra in EQ mode:
the EQ knobs turn LOW / MID / HIGH (the screen's knobs follow); hold the deck's SHIFT and turn its MID knob: VOCALS
moves instead (switch the screen to NEURAL MIX to see it), release SHIFT: MID again. In NEURAL MIX mode the opposite.
## 54. (V63) A NEW SONG STARTS FRESH: cut LOW, VOCALS and the FILTER on deck 1, then load another song onto deck 1 (tap a
song, the controller's LOAD, or AutoMix's next song): all six knobs back at 0 dB and the FILTER off (logcat "New song on
[Channel1] : EQ, Neural Mix and FILTER reset"). Deck 2 untouched.
## 55. (V64) CONCENTRIC EQ KNOBS (Marko 14.10.2026, SSL-style dual concentric pots). Mixer tab: each of HIGH / MID / LOW
is two knobs in one: the INNER cap with coloured dots = the active mode (EQ: HIGH / MID / LOW), the OUTER dark ring with
its own pointer and grey dots = the other mode (NEURAL MIX: HARMONY / VOCALS / DRUMS). A finger turns (and a long press
resets) ONLY the inner one, wherever the knob is touched (V66); the outer one only from the Mix Ultra (SHIFT + knob). Caption:
"MID −4 dB · VOCALS −9" while either is cut. Switch the EQ dropdown to NEURAL MIX: the two swap (inner = VOCALS etc.).
## 56. (V64) PICKUP ON THE MIX ULTRA (as Ableton Live's Pickup): EQ mode, MID knob down to the middle; hold SHIFT and
turn MID a little: VOCALS does NOT jump to the knob; turn it up to where VOCALS is (0 dB, the top): from there VOCALS
follows. Release SHIFT, turn: MID does not jump until the knob reaches MID's value. Same after the screen's knob or a new
song changed a value: the hardware knob first has to reach it.
## 57. (V65) CONCENTRIC KNOB COLOURS AND NUMBERS: EQ = the band colours (HIGH blue, MID green, LOW red), NEURAL MIX =
violet. In EQ mode the inner dots are the band colour, the outer ring's dots and pointer violet; in NEURAL MIX mode the
other way round. Two numbers per knob: on TOP the outer ring's ("VOCALS −9 dB", its colour), at the BOTTOM the inner
one's ("MID −4 dB"), always shown (0 dB too).
## 58. (V67) ROOM EQ (Marko 14.10.2026: an EQ on the output to correct the room, FabFilter style, analyzer, noise /
sweep, three places averaged). Menu -> ROOM EQ: a graph 20 Hz - 20 kHz (+12 .. -18 dB), ten yellow handles (31 Hz ..
16 kHz) on the yellow EQ curve. ON / OFF (the main output only: the recording is not corrected). ANALYZER: Android asks
for the microphone once (allow); the violet live spectrum moves with the sound (talk, clap). Drag a handle up / down:
the curve follows, toast "63 Hz −5.0 dB"; double tap a handle: 0 dB. PINK NOISE (or SWEEP), 5 s: MEASURE 1 plays the
test signal from the phone's speaker / the USB output while "LISTENING ...", then "PLACE 1 measured", a blue line; move,
MEASURE 2 (green), MEASURE 3 (red); the dashed white line is their average. APPLY THE AVERAGE: the handles move to the
inverse of the average, the highest band at 0 dB (cut only), ROOM EQ ON. FLAT: all at 0. Leave and come back, restart
the app: the curve and ON / OFF are kept. Play a song: ON vs OFF is audible (e.g. less boom where the room had it).
logcat DJMantraRoomEq.
## 59. (V68) ROOM CORRECTION (replaces part 58; docs/ROOM_CORRECTION.md). Menu -> ROOM CORRECTION. Pause the music.
AUTO CALIBRATE: Android asks for the microphone once; the sweep plays THROUGH THE APP'S MIXER (audible on the USB PA / the
phone and on Bluetooth if routed), "MEASURING PLACE 1 ...", then the place's line appears (blue), the white dashed
average and the yellow correction curve (31 points); toast "ROOM CORRECTION APPLIED". Move, AUTO CALIBRATE again (place 2,
green). Tap place 1: "LEFT OUT" (faint), AVERAGE & APPLY: the curve changes; tap again: back in. Long press a place:
measured again. BYPASS: lit = off (the curve dims), audible difference with music. MORE: TARGET CURVE (FLAT / HOUSE /
CLUB: grey target line), STRENGTH, TEST SIGNAL (SWEEP / PINK NOISE), LENGTH, TEST LEVEL, MICROPHONE (the phone; a USB
microphone appears when plugged), ANALYZER (violet live spectrum), CORRECTION GUIDE (reads, scrolls, ×). ROOM: NEW ROOM
"Test", measure one place there, switch back: each room keeps its places; restart the app: rooms, places and the
correction kept (Music/djmantra_app/Room_Correction/rooms.json). The recording (RECORD MIX) is NOT corrected. logcat
DJMantraRoomEq ("sweep 8 s, input 0, peak X dBFS").
## 60. (V69) ROOM FILES AND SAVE: Room_Corrections in Music/djmantra_app holds one file per room, "<room>.json", written
by itself after each measurement, APPLY, a band dragged, FLAT, a place in / out. SAVE (next to ROOM): toast "SAVED:
Room_Corrections/<room>.json". NEW ROOM "Test", measure and APPLY there; choose the first room again: ITS correction
curve comes back; choose "Test": its curve. RENAME: the file is renamed; DELETE: the file is gone (adb shell ls
/sdcard/Music/djmantra_app/Room_Corrections).
## 61. (V70) BEATSTATS AND THE SUNO LOGIN FIT THE SCREEN (Marko 14.10.2026: "not adapting to the user interface of
Android ... cropped"). Menu -> BEATSTATS CHARTS -> OPEN BEATSTATS: no status bar (clock, battery) over its bar, its
buttons BACK / READ CHART / CLOSE clear of the camera hole, the page ends above the bottom edge (nothing under the
navigation area). BACK goes back a page in the site (on the first page: closes). A swipe from the bottom edge shows the
navigation bar for a moment, then it hides again; coming back from another app: hidden again. The same for SUNO -> GET
(the Suno login browser): BACK / DONE on top, clear of the clock and the hole. Screenshot both (top 130 px, bottom 150 px).
## 62. (V71) SONGS TOP LEVEL BAR (r41 V70 phone bug): FOLDERS | PLAYLISTS no longer run over LIST / GRID; in PLAYLISTS the
filter field ("Find a song") has room next to SUNO, TIDAL and FIND ON PHONE (two lines). Screenshot the bar in FOLDERS and
in PLAYLISTS, list and grid.
## 63. (V72) ROOM CORRECTION FIXES (r41 V70 phone report 8d1c20c). (a) ALL 31 bands now act: drag the 4 kHz and 16 kHz
handles to -12 with music playing: audible, and the yellow curve shows it (Mixxx effect slots had 16 knobs: bands from
1 kHz up never reached the engine; now 32 on Android). (b) A measurement the microphone barely heard (peak below -40
dBFS) or that cannot be a room (63 Hz - 8 kHz spread > 40 dB, a 25 dB jump between thirds) is NOT kept: the status says
why ("raise the TEST LEVEL ..."), the place stays empty. Test: TEST LEVEL -30 dB with the speaker turned right down.
(c) Room_Corrections is not listed as a folder in Songs and its test WAVs get no genre lookup (logcat: no Deezer
lookup for sweep_*.wav). (d) Room name: type "Test Room" without Enter, NEW ROOM: the room is "Test Room", the keyboard
closes; RENAME the same. (e) DELETE asks first ("TAP AGAIN TO DELETE ..."), a second tap within 3 s deletes. (f)
Choosing a room resets the status line. Delete the bad "My room" from the V70 test first (or FLAT it).
## 64. (V73) CONCENTRIC KNOB REDESIGN (Marko 14.10.2026: text overlapped; two concentric pots with two concentric LED
rings outside). Mixer tab, each HIGH / MID / LOW: from the centre out the inner cap (white pointer), the dark outer knob
ring right round it (its own violet / band-colour pointer), then two LED rings outside both, dot for dot: the inner
knob's (closer) and the outer knob's (outermost). Above: the outer value small ("VOCALS −9"), below: the inner ("MID −4"),
no "−∞ / 0 dB" end labels on these knobs, NO text overlapping anything (screenshot the whole mixer column, both decks,
EQ and NEURAL MIX mode).
## 65. (V74) COMPACT LIST, as a macOS list view (Marko 14.10.2026: half the height, no "Folder" text, whole names). Songs
in LIST view: every row 30 px, one line. A folder: its icon and its WHOLE name across the row (no "Folder" under it, no
"..." while the name fits: "HOTEL KORINJAK {by ...}" now complete). A song: small cover, TITLE / ARTIST / LENGTH / BPM /
KEY under their headers. PLAYLISTS: name, CREATED, SONGS, LENGTH under theirs. Twice as many rows on screen as V73; tap,
long press (add to a playlist), the controller's BROWSE highlight and AutoMix's PLAYING / NEXT marks still work.
## 66. (V75) PLAYLISTS ARE FOLDERS (Marko 14.10.2026: one view). Songs: no FOLDERS | PLAYLISTS buttons any more. The top
level (← from djmantra_app) lists djmantra_app, Music, Download and "Playlists". Open Playlists: every playlist as a
folder (NAME, CREATED, SONGS, LENGTH; SUNO and TIDAL buttons there); open one: its songs as files (load, long press,
sort, filter as in any folder); ← goes back to Playlists, ← again to the top level. Adding a song to a playlist (long
press) and REMOVE FROM still work; a new playlist appears inside Playlists.
## 67. (V76) NO EMPTY FOLDERS OR PLAYLISTS (Marko 14.10.2026: "only playlists which contain some songs"): Songs hides
every folder without a song anywhere inside it (subfolders counted) and every playlist whose songs are all gone. Test:
make an empty folder in Music/djmantra_app (adb shell mkdir), and one with an empty subfolder only: neither shows; copy a
song into the subfolder, switch to another app and back: the folder shows. A playlist whose files were deleted: gone
from Playlists (and from "add to playlist").
## 68. (V76) BLUETOOTH CONNECT DOES NOT OPEN THE BROWSER (r41 V71 BUG 1): connect the Mix Ultra over Bluetooth (or USB):
the song list does NOT open by itself; 2 s later BROWSE turn / press work as before.
Tester note (r41 BUG 2): with the navigation bar shown the app makes room for it (the rule: nothing under the bars), so
the tab bar moves up ~15 px; read the positions again after the bars appear (a finger taps where it is drawn).
## 69. (V77) FX PADS ON THE MIX ULTRA (Marko 15.10.2026: "each effect to the pad ... activated by Shift FX ... more than 8
effects ... Shift Pad. Then we have 16 pads"). Each deck: SHIFT + FX (the controller's SLICER mode): the app goes to the
Mixer tab (log `controller: page 3`). Pads 1-8 = FLANGER, UNDERWATER, COOLDOWN, ECHO OUT, DJM DELAY, DJM ECHO, DJM TRANS,
DJM PAN on THAT deck: a tap switches it on (toast "DECK 1  DJM ECHO ON", the deck's FX button on screen shows the name
lit, the pad lights, no running beat light in this mode, you hear it); the same pad again: off (pad dark); another pad
switches to that effect. A pad HELD over half a second: the effect only while held, off on release. SHIFT + pad: effects
9-16, none yet: toast "FX PAD 9: NO EFFECT". Deck 2 the same, on deck 2 only. Log lines `FX pad: deck N pad M ...`.
Leave FX PADS (HOT CUE): pads as before, beat lights back.
## 70. (V77) "1 IS HERE" MOVED: the SLICER was its home; now NEURAL MIX mode, SHIFT + pad 5 sets beat 1 of the phrase at
the play position (pads 5-8 still jump to beats 5-8, pads 1-3 the kills).
## 71. (V77) SHIFT + FILTER = THE EFFECT'S DEPTH, WITH A POP-UP (Marko 15.10.2026). Switch an effect on (FX pad or the FX
button), hold SHIFT on that deck and turn its FILTER knob: a pop-up under the toast shows "DECK 1  DJM ECHO  ON • DEPTH"
and the DEPTH fader; it goes 2.5 s after the knob stops (log `FX depth pop-up: deck 1`). The depth does NOT jump to the
knob: a half-transparent GHOST handle shows where the knob is, the real handle stays until the knob reaches it (or
passes it), then they are one and move together (you hear the depth change only from then). Echo Out: left half 1
beat, right half 2 beats. Release SHIFT and turn FILTER: the filter does not jump either (its knob was moved for the
depth): ghost on the FILTER knob until it picks up. Deck 2 the same.
## 72. (V77) GHOSTS ON EVERY PICKUP KNOB (Mixer tab, controller connected): reset a deck's MID on the screen (long press)
with the controller's MID knob turned left; turn the controller knob a little: a half-transparent pointer and ring
marker at the knob's position, the real one stays; turn through the value: the ghost disappears and the knob follows.
The same with SHIFT (the outer, NEURAL MIX ring: ghost on the outer ring) and after loading a new song (EQs reset). A
ghost left alone fades after 3 s. Screenshot one ghost.
## 73. (V77) THE WAVEFORM AS SHARP AS THE ICON (Marko 15.10.2026: "ours is muddy and blurry ... redesign our waveform to look
like one in the icon itself"). Load a song on both decks, play. The scrolling waveform: separate SOLID bars with thin
black gaps (no haze, no translucent fringe, no soft edges), each bar one pure colour (kicks / bass RED, vocals and mids
GREEN / CYAN, hats BLUE / VIOLET, bass + hats MAGENTA), mirrored round the middle; the bars move with the song, no
shimmer or flicker while scrolling, at every zoom (pinch in and out). Deep zoom: the thin real-wave line as before. The
overview (the whole song) the same: 2 px bars, 1 px gaps, solid colours. The cloud preview (same song section, old vs
new shader): docs/ui-reference/waveform-v76-vs-v77.png. Screenshot both decks playing next to the app icon.
## 74. (V78) WAVEFORM: r42 notes 1 and 2 (tester, V77). (1) Only the icon's colours: no ORANGE-AMBER, YELLOW or
YELLOW-GREEN bars any more (bass + mids now go to red-orange or green); red, red-orange, green, cyan, blue, violet,
magenta only. Same songs as r42 06 / 07 for comparison. (2) Height: the loud bars reach about two thirds of each lane
(was a quarter to a third); quiet parts lower. Settings -> LANE HEIGHT "tall" still makes it taller. Screenshot as 06.
## 75. (V78) CONCENTRIC KNOBS WITHOUT LEDS, BIGGER (Marko 15.10.2026, on V77: "remove LEDs around knobs which are concentric
because now they are too small. So LEDs go only around the filter, that's all. And the other is completely removed and
the knobs are enlarged"). Mixer tab, both decks: HIGH/HARMONY, MID/VOCALS, LOW/DRUMS have NO dot rings (neither inner
nor outer); the outer knob fills the space the rings used (the knob as wide as the FILTER's ring), the inner cap 60 % of
it; each value shows by its pointer (white inner, coloured outer) and the numbers above / below. FILTER keeps its LED
ring. Ghosts (part 72) on these knobs: a half-transparent pointer plus a small hollow circle on the knob's rim (inner
cap's edge for the inner value, the outer knob's edge for the outer). Screenshot the mixer column, both modes.
## 76. (V78) SECTIONS SEEN IN THE WAVEFORM: 3-BAND (Marko 15.10.2026: "sections are not clearly seen in this new waveform
... I need to see a change in music, so sections must be clearly seen through the waveform shape and colors ... even
when the vertical lines make mistake"). New default WAVEFORM style 3-BAND (rekordbox's 3-band idea in the icon's
colours): every bar is three nested solid layers, BASS red at the back (widest), MIDS green round the core, HIGHS
blue-violet in the core; the colours never mix, so they do not change bar to bar, and each layer is smoothed over
~145 ms so a section reads as one texture. Check on the same songs as r42 06 / 07 and on a dance track with a
breakdown: a BREAKDOWN shows no red (green / blue only), a DROP wide red, vocals in = wider green, hats = the blue
core; the change is visible where the section line is, and would be without it. The overview lanes the same (2 px
bars, three layers): the song's structure readable at a glance. Settings -> WAVEFORM: 3-BAND (sections) / ICON (one
colour, V77); the choice applies when the tab opens again (log `Settings: waveform ...`). Cloud preview, same section
in V76 / V77 / V78 and the whole song: docs/ui-reference/waveform-sections-v76-v77-v78.png. Screenshot both styles.
