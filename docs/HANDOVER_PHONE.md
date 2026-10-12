# Handover: the phone tester (13.10.2026, after round 29)

Session: "termux-tools-70" / session_01MCmmCT2kk49c9sPPKrkUk8, Claude Code in Termux (PRoot Ubuntu) on Marko's
Nothing Phone (2a) (A142, Android 16, 1084x2412). Role: phone tests only, from the coordinator's `testing/ROUND<N>.md`
(docs/COORDINATION.md); never app code. Results: `testing/results/<UTC>_r<N>_phone/REPORT.md`, one-line message
per part to session_01Ggm1Tq5a9mnY3wAZMMS9jh.

## 1. Rounds 22-29 (12-13.10.2026)

| Round | Build | Result | Fails / notes |
|---|---|---|---|
| 22 | 94fc08d | 8 pass, 2 fail | router listed only "Phone default"; CPU ~80 % of a core; genre 1 of 5 (Deezer found nothing) |
| 23 | 7478fb6 | 6 pass, 2 fail | CUE after play-from-cue did not return (fixed 302ab51); CPU Phrasing 76 % / Mixer 75 % |
| 24 | f6eae00 | 3 pass, 2 fail | a CUE preview made AUTO PHASE jump the on-air deck; CPU 62-66 % on every tab; genre now English |
| 25 | 763c684 | 4 pass, 1 fail | CPU Phrasing 65 %, Mixer 58 % (Songs 39 %, Keys 43 % fine); every aligned PLAY moved the cue (fixed in 26) |
| 26 | a5fd87c / 1845c1d | parts 0-3 pass | CUE stays put; parts 4-6 void (Marko used the app); clock colon made static on his request |
| 27 | V1 a972af4 | 0-2, 4 pass; 3 fail | scrub zoom window invisible; AUTO PHASE jumped the other deck during a scrub (both said fixed in V2, not retested); stopped by Marko |
| 28 | V4 b455a83 | 0 pass | stopped by Marko ("stop testing") |
| 29 | V7 208a91a | 4 pass (0-3), 0 fail | 150 songs analysed in 5:53; sections of all 150 in the app; counter resets at each section |

Round 29 files: `testing/sections/app_analysis_v7.txt` (150 lines bpm / key / first_beat_s / const),
`app_beats_v7.txt` (55 `Beats:` lines, no file names yet), `app_sections_v7.txt` (150 songs, mean 19.9 sections).
Section research: `testing/sections/README.md`, manifest `modules/section-detection.md`.

## 2. The phone setup (stays as it is: Marko asked for it "set up forever")
- **adb without Wi-Fi**: the phone's own loopback, `adb connect 127.0.0.1:5555` (after `adb tcpip 5555` once per
  reboot, over Wireless debugging or USB). `adb devices` → `127.0.0.1:5555 device`.
- **Shizuku** (moe.shizuku.privileged.api) as the second path; `rish` via app_process with
  `RISH_APPLICATION_ID=com.termux` (the rish script refuses inside PRoot).
- **phonelink** (`~/termux-tools/phonelink`, repo termux-tools): `status | sh | exec-out | install | watch`; a runit
  service `$PREFIX/var/service/phonelink` keeps adb and Shizuku alive (log `~/.cache/phonelink.log`). After a reboot
  it needs one Wireless debugging start; then it takes over.
- **Test helpers** (in `~/tmp`, not in git): `r29.sh` (tap by the app's `Quick: map` lines, newest wins; `shot`, `q`,
  `clr`, `rot`), `rep.sh` (append to REPORT.md, commit, push: **it stages only the round folder; add other files
  by hand and check `git show --stat`**), `buildwait.sh N` (background waiter: ROUND<N>.md + green Android run),
  `buildwait2.sh BASE` (only a build after commit BASE).
- `gh run download` needs `TMPDIR=~/tmp` (else it exits 0 with an empty folder).
- **Songs**: 150 test songs in `/sdcard/Music/Drive150` (6 renamed: `"` → `'`, `?` removed; Android storage refuses
  them), the same in the private repo markoboskoauroville/djmantra-test-songs and in `~/tmp/drive/mp3`.
- Section bench offline: `~/tmp/sections` (numpy / scipy / matplotlib from apt), `~/tmp/bench2`.

## 3. Habits that matter on this phone
- Never lock rotation: Marko frees it again within a minute. Test in the orientation he leaves; re-read the tap map
  after every rotation.
- Wake the phone (`input keyevent KEYCODE_WAKEUP`, `wm dismiss-keyguard`, `cmd statusbar collapse`), keep it awake
  for a round (`svc power stayon true`), switch that off at the end.
- If another app comes to the front or the phone turns while testing, Marko is using it: stop and say so.
- Escape / "stop testing" = stop at once, write the stop into the report, push, tell the coordinator.

## 4. Open problems
- CPU on the Phrasing tab ~65 % of a core (render thread ~21 %); Mixer ~58 % (round 25).
- Scrub zoom window and AUTO PHASE during a scrub: fixed in V2 per the builder, not retested on the phone.
- Untested on the phone: the audio router with a second output and channels 3-4 (needs the Mix Ultra or Bluetooth
  plugged in), the Mix Ultra controller mapping, CPU on V7, genre match on V7.
- V7 finds ~20 sections per song (every ~8 bars); decide whether the counter resets on all of them or only the
  major ones.
- `Beats:` log lines carry no file name (fixed in f9e2d7a, from V8).
- The release build is not debuggable: `run-as` cannot read the app's database; analysis comes through log lines.
- Donnie Horoscope v23 (astro_dos, report reports/v23.md on branch phone-reports) is half tested and paused:
  zoom / drag / double tap / planet tap PASS, pinch not possible over adb (SELinux), no reset after a 180° turn,
  the app is portrait only; icon, Donnie page, regression, web page, web app, manual shots and the report are
  still to do.

## 5. How to resume
1. `~/termux-tools/phonelink status` → adb `127.0.0.1:5555`, shizuku `up`.
2. `cd ~/djmantra_app && git pull` on `ccr-eb7d1019-d3kubd`; read docs/COORDINATION.md and the newest ROUND file.
3. `~/tmp/buildwait.sh <N>` in the background; when READY: download, install, run the round, push per part.
