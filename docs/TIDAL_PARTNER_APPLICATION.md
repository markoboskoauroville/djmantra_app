# TIDAL DJ integration request: DJ Mantra

A draft application for Marko to send to TIDAL. Send it from the TIDAL developer account where the app is already
registered (developer.tidal.com, Client ID `Etv8AkwIcduV4SYO`), through its support / contact form or TIDAL's
partnership contact. Fill in the lines in [brackets] before sending.

---

**Subject:** DJ integration request: DJ Mantra (Android DJ app), access to full-track playback for DJ mixing

Hello TIDAL team,

I am the developer of **DJ Mantra**, a DJ app for Android phones and tablets. I would like to ask for access to
TIDAL's DJ integration: the same full-track streaming for DJ mixing that TIDAL offers through djay, Serato, rekordbox,
Hercules DJUCED and edjing for subscribers with the DJ Extension.

**The app**
- DJ Mantra is built on **Mixxx 2.5.6**, the open-source DJ engine (GPL). It adds a touch interface for phones, an
  Android audio engine (Oboe / AAudio, USB audio interfaces) and support for the **Hercules DJControl Mix Ultra** over
  USB and Bluetooth.
- Its features: two decks with beat grids, key detection (Camelot), BPM sync and tempo blend, phrase-aware mixing, a
  cut-only 3-band EQ and stem EQ (drums / vocals / harmonies), a Pioneer-style filter and beat effects, AutoMix, and
  playlists.
- It is developed and tested on a Google Pixel 7. The project page is https://djmantra.pages.dev.
- The app is registered on the TIDAL developer platform (Client ID `Etv8AkwIcduV4SYO`). It signs users in with OAuth
  2.0 PKCE and keeps no client secret in the app.

**What we ask for**
1. Full-track playback of TIDAL tracks on DJ Mantra's decks for **subscribers with the TIDAL DJ Extension**, under
   the terms that apply to TIDAL's DJ partners.
2. The technical way to do this: decoded audio (PCM) that the DJ engine can process in real time (tempo, key lock,
   EQ, filter, crossfade, two decks at once), or TIDAL's recommended DJ playback component, and the analysis it may
   keep (beat grid, key, waveform).
3. The rules for offline use or caching in DJ sets, if TIDAL allows it.

**How TIDAL content will be protected**
- No downloading or exporting of TIDAL audio. Streams are only played on the decks.
- **Recording is switched off** whenever a TIDAL track is loaded, as djay does with streamed tracks.
- TIDAL tracks are shown with TIDAL's branding and attribution, and are never mixed up with the user's own files.
- Only TIDAL's official SDK or API is used, with no unofficial clients. We will follow TIDAL's developer terms and any
  partner requirements, and pass any review TIDAL asks for.

**What we are building now with TIDAL's public API**
- Sign-in (OAuth PKCE), the user's playlists, and matching TIDAL tracks to the user's own music files by ISRC, for
  exact recordings.

I would be glad to show a test build, walk through the integration with your team, and adjust it to your
requirements.

Kind regards,

[Your full name]
DJ Mantra, [country]
[email] · https://djmantra.pages.dev
