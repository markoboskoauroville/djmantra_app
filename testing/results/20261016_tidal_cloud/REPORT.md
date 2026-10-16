# TIDAL from the cloud session with Marko's refresh token (16.10.2026)

Marko: "please test TIDAL_REFRESH_TOKEN ... generate few playlists on Tidal and also to stream that playlists through
the DJ player". The token (environment variable, never printed or stored in the repo) was used by a script that makes
the same requests as `TidalBridge.java` (same endpoints, same order).

## Login
- Refresh with the app's client ID `JjTAmDVohiV7GgOL` (DJ Mantra): **OK**. Scopes: `playlists.read search.read
  playlists.write user.read`. TIDAL did not rotate the refresh token. Account country HR, developer tier THIRD_PARTY.

## Reading playlists (`playlists()`)
- **OK**: all of Marko's playlists listed with their counts (Nu Disco Chart (generated) 108, ECSTATIC DEMO 26, the
  morning / evening sets, the world sets ...).

## Making playlists (`pushPlaylist()`): two bugs found and fixed in V86
1. **Rate limit**: TIDAL answers **429** after ~7 quick requests. The app took that as an error: a whole push failed
   ("TIDAL 429"), and in the ISRC lookup a 429 meant "not found" -> the song was searched instead -> another version
   -> a double on the next sync (seen: re-sending the same 10 songs added 1). **Fix**: a 429 waits (Retry-After or 1,
   2, 4 ... s, 6 tries).
2. **Wrong songs from search**: the fallback (no ISRC, or an ISRC TIDAL does not have) took the search's **first**
   track: DSF "Not Like You" -> The Bangles; Lizzo "About Damn Time (Purple Disco Machine Remix)" -> RUFUS DU SOL "Treat
   You Better (PDM Remix)"; Incognito "Always There" -> Greta Van Fleet; "Ai No Corrida" -> a Uniting Nations cover;
   "Baby Got Back" -> a karaoke version. **Fix** (`bestTrack`): search with the artists included; the track must have
   the same title (without its version) and one of the wanted artists, no karaoke / cover / tribute / instrumental
   unless asked; the wanted version's words win; none -> nothing added (logged "not on TIDAL"). The Java code, run on the
   real search answers, picks the same tracks as the tested script (all 8 right, Jakatta's PROFF version: none).

Result after the fixes (test playlists on Marko's TIDAL, "DJ Mantra TEST - ...", to delete when done):

| playlist | songs | on TIDAL | by ISRC | by search | not on TIDAL | sent again |
|---|---|---|---|---|---|---|
| DJ Mantra TEST - Organic House Top 10 (the BeatStats chart, ISRCs from Beatport) | 10 | 9 | 8 | 1 (DSF: its ISRC is not on TIDAL HR) | 1 (Jakatta - American Dream (PROFF Extended Interpretation)) | 0 added |
| DJ Mantra TEST - Disco classics (artist + title only) | 6 | 6 (Chaka Khan / Rufus, Lizzo PDM Remix, Quincy Jones, Diana Ross, Incognito, Sir Mix-A-Lot: each the right one) | 0 | 6 | 0 | 0 added |

Read back as the app imports a playlist (`playlistTracks()`): the songs in order with artists, titles, ISRCs. **OK.**

## Streaming through the DJ player
- TIDAL's official stream endpoint `GET /v2/trackManifests/{id}` (the one TIDAL's own player SDK uses) answers for this
  app with **`trackPresentation: PREVIEW`, `previewReason: FULL_REQUIRES_SUBSCRIPTION`**: a **30-second preview**
  (measured 29.98 s, AAC 44.1 kHz stereo, no DRM), for a track whose `availability` is `STREAM, DJ`.
- So the official API gives this app (developer tier THIRD_PARTY, client DJ Mantra) only previews. Full tracks need
  TIDAL's entitlement for the account / the app: Marko's TIDAL subscription as TIDAL sees it, and for DJ apps TIDAL's
  DJ partnership (the "DJ" availability is what TIDAL licenses to DJ partners such as djay, Serato, rekordbox). The
  scope `entitlements.read` (registered on the portal) was not granted to this token, and `/userEntitlements/me` is 404,
  so which of the two it is could not be read.
- Not built: playing 30-second previews in a deck, or getting full streams any other way (no unofficial APIs, no
  stream downloading; CLAUDE.md).

## The whole round, BeatStats -> TIDAL (V86 additions, tested 16.10.2026 evening)

Matching now ported from the Playlist Generator (`markoboskoauroville/generated_playlists`, tools/tidal_playlist.py):
a search ladder from exact to loose, the same song in another version when the named one is missing ("the song
matters, not the exact version"), never another song. The app's Java `findTrack`, run against live TIDAL, on 12 songs:
all right (Jakatta -> "American Dream (PROFF Interpretation)", Chic -> "Le Freak (Dimitri From Paris Remix)",
Justice "D.A.N.C.E.", Tensnake "Coma Cat", Sgt Slick's own "Gimme! Gimme! Gimme!", the Lizzo PDM remix ...).

Two real BeatStats charts through the round (the same requests as `TidalBridge.pushChart`), on Marko's TIDAL (UNLISTED):

| chart | songs | found | exact version | other version of the song | not on TIDAL | TIDAL |
|---|---|---|---|---|---|---|
| BeatStats - ORGANIC HOUSE Top tracks (the saved chart page) | 100 | 100 (all by search: Beatport refused the cloud after its first pages) | 93 | 7 | 0 | https://tidal.com/playlist/b091d272-b039-42a6-a825-02576a7d452d (99 tracks: the chart has "Who Loves The Sun" and its DSF Remix, TIDAL only the first) |
| BeatStats - TRANCE (RAW / DEEP / HYPNOTIC) (the June screen capture's songs) | 96 | 89 | - | - | 7 (one is an OCR error, "Not Tonight Panels") | https://tidal.com/playlist/ace5e62c-8da9-4547-a278-fc8361b16051 |

Beatport: 10/10 ISRCs through a browser engine this morning; tonight, after 100 quick requests from the datacenter
address, Beatport stopped answering the cloud at all. The search fallback alone found 100/100 there, so the round does
not depend on the ISRCs.
