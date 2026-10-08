---
description: Log in to TIDAL with the user's account, check what the API returns for DJ Mantra, and push the report
---

You are running on the user's own computer. Goal: a real check of TIDAL's official API with the
user's account, so the cloud session can build the app's TIDAL code against real responses.

1. `git fetch origin claude/admiring-feynman-hym3vp && git checkout claude/admiring-feynman-hym3vp && git pull`
2. Run `python3 tools/tidal/tidal_check.py`. It opens the TIDAL login in the browser; tell the user
   to log in and approve DJ-Mantra. (Port 8765 must be free; the redirect
   `http://localhost:8765/tidal-callback` is registered for the app.) It prints `REPORT=<path>`.
3. Read the report and the raw JSON files next to it. Summarize for the user in a few lines:
   country, number of playlists, whether tracks have genres / BPM / key / popularity, whether play
   counts (trackStatistics) are readable, how many genres TIDAL lists.
4. Privacy check before committing: the files must contain no access token, no `code=` value and no
   e-mail address. Show the user the list of playlist names that will be committed (the repository
   is public) and ask if that is OK; if not, delete `playlists.json` and remove the names from the
   report.
5. `git add testing/results/ && git commit -m "TIDAL API check: <one-line outcome>"` and
   `git push origin claude/admiring-feynman-hym3vp`. Tell the user to say
   "read the latest TIDAL results" in the cloud session.

Never ask for, print or store the TIDAL Client Secret or any token.
