# Round 42: V90 on the Pixel 7 (one-button TIDAL login)

Tester: a local Claude Code session on the Pixel 7 (Termux / proot, adb over wireless debugging to 127.0.0.1;
paired 17.10.2026, rescan the connect port with ONE python process, see docs/HANDOVER.md). Screenshots to
Download/djm_v90/. Report each part as soon as it is done (PASS / FAIL, what you saw, logcat lines).

0. Install https://github.com/markoboskoauroville/djmantra_app/releases/download/android-latest/DJMantra-V90-9d65517-phone-arm64.apk; versionName V90; starts; crash grep
   (`FATAL|AndroidRuntime|SIGSEGV|qml.*error|TypeError|ReferenceError`) empty.
1. Settings: the TIDAL block is ONE line "TIDAL: not logged in" (or "logged in as ...") and ONE button LOGIN TO TIDAL
   (or LOG OUT OF TIDAL). No key-file or token-file rows.
2. If logged in: LOG OUT OF TIDAL -> "TIDAL: not logged in", the button says LOGIN TO TIDAL.
3. LOGIN TO TIDAL: logcat `DJMantraTidal ... login in org.mozilla.firefox`; Firefox opens TIDAL's login. Marko logs in
   (a person's step). The page says "TIDAL is connected. Go back to DJ Mantra.", and DJ Mantra comes back to the front
   by itself; Settings shows "TIDAL: logged in as <name>" and LOG OUT OF TIDAL; toast "TIDAL: LOGGED IN".
4. Force-stop the app (`am force-stop com.djmantra.app`), start it: Settings still says logged in as <name>.
5. Songs -> TIDAL playlists list loads (Marko's playlists); import one: the songs on the phone go into the app
   playlist, the rest into QUEUES -> TIDAL.
6. BeatStats: a saved chart -> TIDAL PLAYLIST: the status line reports the count on TIDAL; the playlist appears on TIDAL.
7. LOG OUT OF TIDAL, then LOGIN TO TIDAL again: works the same.
8. Crash grep empty for the whole run; `adb logcat -d -s DJMantraTidal:* | tail -40` (no token is ever logged).
Send "round 42 finished".
