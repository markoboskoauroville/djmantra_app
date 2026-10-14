# BeatStats samples (from Marko's Mac, 30.6.2026)

- `BEATSTATS.COM-SCREEN-CAPTURE.pdf`: a chart page as a full-page screen capture (TRANCE (RAW / DEEP / HYPNOTIC)).
- `screencapture-beatstats-tracks-home-list-...pdf`: the tracks home list.
- `..._extracted.json`: what the vision model read from the first PDF: {chart_title, entries[]}.

The live site answers plain HTTP with 403 (Cloudflare) and holds a remote-controlled Firefox on "Just a moment...". His old pipeline: screen capture -> vision OCR (Gemini) -> Spotify search (`charts`, repo markoboskoauroville/charts).
