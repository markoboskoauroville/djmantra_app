# DJ Mantra app

Android DJ app built on the Mixxx 2.5.6 engine. Plan and status: `docs/ANDROID_PORT.md`.
Phone testing: `testing/README.md` (`/test-on-phone`, results in `testing/results/`).

## Standing rules

- **Asking Marko for something: one action at a time.** Write it on its own line as
  `!!!!! MARKO: <the one action>` (five exclamation marks in front), then wait for him to confirm "done" before giving the next one.
  Never a list of steps for him (owner's rule 9.10.2026).

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
- **All device tests run through the local session "Claude Code local"** (Marko's Mac, Remote
  Control, session `session_01NtxDMqMEkD435RBFuqFs1s`; owner's rule 9.10.2026). It has the
  phones on USB: Nothing Phone 2, Pixel 7 and a Pixel 7 emulator. The app must work on all
  three; where a feature cannot, make it work where it can. Protocol in `testing/README.md`:
  1. Check the session first (`get_session`: `connection_status` connected). If it is not
     connected, ask Marko to start Remote Control on the Mac; do not run device tests anywhere else.
  2. Send it the exact commands (`send_message`); it runs them, commits the results to
     `testing/results/` on the development branch and messages back (`SendMessage` to "DJ APP cloud").
  3. Read the results, fix in the cloud, push, wait for the release, send the next round. Develop
     on without Marko; he is needed only for the last phase (USB-C audio out, Mix Ultra, USB stick).
