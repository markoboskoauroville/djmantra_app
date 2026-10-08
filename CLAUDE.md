# DJ Mantra app

Android DJ app built on the Mixxx 2.5.6 engine. Plan and status: `docs/ANDROID_PORT.md`.
Phone testing: `testing/README.md` (`/test-on-phone`, results in `testing/results/`).

## Standing rules

- **Keep the public progress page current.** After every app milestone, update
  `assets/app/progress.json` in the website repo `markoboskoauroville/djmantra-ecstatic-dance`
  (djmantra.pages.dev): step `status` (done / doing / todo), `hr` and `en` text, and the `updated`
  date. Follow that repo's rules: log the request word for word in `momentaryupdates.md`, bump the
  version label in `index.html`, and push progress updates straight to that repo's `main` (approved by
  the owner 8.10.2026): a push to `main` deploys djmantra.pages.dev automatically (deploy.yml).
- Never commit credentials. The TIDAL **Client ID** (`Etv8AkwIcduV4SYO`) is public and may live in
  config; the TIDAL Client Secret and any tokens must never be committed or asked for. The app uses
  OAuth PKCE and does not need the secret.
- Desktop Linux must keep building and passing Mixxx's test suite (`.github/workflows/linux.yml`).
- No unofficial or reverse-engineered service APIs (Shazam wrappers, StreamRip-style downloaders).
