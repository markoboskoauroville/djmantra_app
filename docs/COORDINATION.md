# Who does what (Marko, 12.10.2026: "you are the master brain, coordinate everything")

Three Claude sessions work on DJ Mantra. The cloud session coordinates; the other two do only what needs
their machine. Messages: `send_message` to the session id; results always as files pushed to git.

| Session | Where | Id | Does |
|---|---|---|---|
| **Builder / coordinator** ("DJ mantra app testing") | cloud | session_01Ggm1Tq5a9mnY3wAZMMS9jh | all code, algorithms, offline analysis of the 150 test songs, CI, ROUND files, decisions |
| **Phone tester** ("dj amp local") | Marko's phone, Termux + adb | session_01MCmmCT2kk49c9sPPKrkUk8 | installs builds, runs `testing/ROUND<N>.md`, pushes `testing/results/`; the 150 songs in /sdcard/Music/Drive150 |
| **Mac helper** ("record box local") | Marko's Mac | session_01BRyLJvNCpeYAXPPDoXdwWp | rekordbox 7.2.19 and Engine DJ exports, KeyFinder, All-In-One (allin1); pushes small JSON only, never audio |

Rules: one owner per task; the helpers never change app code; Marko is asked only for what needs a person, one
action at a time (`!!!!!MARKO!!!!!>`); audio only in the private repo markoboskoauroville/djmantra-test-songs.

## Task board (12.10.2026, 22:xx UTC)
- Phone testing: STOPPED (Marko) until the version with the new BPM / key / grid / section analysis (V6).
- Mac: (A) rekordbox phrases DONE (testing/sections/rekordbox.json); rekordbox raw USBANLZ + XML + Engine DJ
  databases -> djmantra-test-songs (in progress); (B) KeyFinder keys -> testing/sections/keyfinder_keys.json;
  (C) allin1 sections -> testing/sections/allin1/.
- Cloud: sections learned from rekordbox (testing/sections/learn, F1 0.61 within a bar vs 0.40); grid phase
  (off-beat in ~27 % of songs, ~60 ms early) - Essentia beat tracker benchmark running; key: QM 55-58 % vs
  rekordbox, waiting for KeyFinder / Essentia numbers; BPM: live-drum songs need a beat map, not one tempo.
- Then: V6 with all four, CI green, ROUND29 to the phone tester.
