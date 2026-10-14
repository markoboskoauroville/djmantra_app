# Handover from the Mac session "record box local" (12–13.10.2026)

Session `session_01BRyLJvNCpeYAXPPDoXdwWp` on Marko's MacBook, working for the cloud builder. Songs: the 150 Sunrise
Dance mp3 already on the Mac in `~/Downloads/GDriveDownloader/❂ Sunrise Dance ❂` (146 distinct titles); the
`djmantra-test-songs` clone was never needed.

## Done (commits on ccr-eb7d1019-d3kubd)

| | What | Commit |
|---|---|---|
| rekordbox phrases | rekordbox 7.2.19, Phrase + BPM/Grid + Key on; the 150 songs imported into the Collection (the Analyze button had done nothing because the tracks were only in Explorer) and analysed. `testing/sections/rekordbox.json`: 165 songs with phrases (the 150 + rekordbox demo tracks). | 6c1937b |
| rekordbox BPM/key | `~/Music/rekordbox.xml` exported (199 tracks). `rekordbox_xml.py` output was byte-identical to the file already on the branch, so not recommitted. | 726f9f8 (earlier) |
| KeyFinder | libkeyfinder 2.2.8 (Homebrew; there is no keyfinder-cli formula) + a small tool of our own, `testing/sections/keyfinder/kf.cpp` + `run.py`. `testing/sections/keyfinder_keys.json`: 150 of 150. | 1ea4e9d |
| djay Pro | Reverse engineered `.djayMetadata` (binary plist): colours RGB565 per 256 samples, uint8 amplitudes, neural beat grid, key (libkeyfinder order), mix-in/out ranges. `testing/djay/README.md` (the findings), `testing/sections/djay.json` (146 titles), `testing/djay/decoded_150.tar.gz`, screenshots, renders, decoder. | 7717018 |

## Not done, and how to resume

1. **A: rekordbox raw ANLZ + XML + Engine DJ databases → djmantra-test-songs (main).** The Mac's permission check
   refused this send from Claude (personal library data) even with Marko's approval in the chat. It is one script,
   for Marko to run himself: `! ~/Developer/claude-code/tools/send_dj_reference.sh` (sparse clone, picks the 150
   ANLZ folders by the PPTH file name, copies `~/Music/rekordbox.xml` and `~/Music/Engine Library/Database2/*.db`,
   commits, pushes, prints `SENT <sha>`). Engine DJ must be closed (it was).
2. **C: allin1.** Venv `~/.venvs/allin1` (Python 3.11) has torch 2.14.1 and allin1 from PyPI. madmom from GitHub was
   refused by the permission check; PyPI madmom 0.16.1 fails on 3.11 (`cannot import name 'MutableSequence' from
   'collections'`). To resume, Marko runs `! ~/.venvs/allin1/bin/pip install --no-build-isolation
   git+https://github.com/CPJKU/madmom`, then `~/.venvs/allin1/bin/allin1 --out-dir ~/Music/allin1 <the 150 mp3>`
   and the JSON goes to `testing/sections/allin1/`.
3. **djay screenshots of more songs.** Only Nanana was captured (deck A playing at 1:14, both decks at the start /
   0:29). Loading That Girl by script failed: search worked, but the drag and Cmd+← (`musicLibrary.load1` in
   Marko's key bindings) did not load it. The decoded data covers That Girl and every other song; `render.py` draws any
   of them.

## Where things are on the Mac

- rekordbox analysis: `~/Library/Pioneer/rekordbox/share/PIONEER/USBANLZ/` (PPTH holds only `?/<file name>`).
- djay analysis: `~/Library/Group Containers/VJXTL73S8G.com.algoriddim.userdata/Library/Application Support/Algoriddim/Metadata/`.
- Engine DJ: `~/Music/Engine Library/Database2/` (m.db 17 MB + six small dbs).
- The requests word for word: `~/Developer/claude-code/momentaryupdates.md` (hub).
