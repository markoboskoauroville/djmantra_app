# Android packaging

- `vcpkg.json`, `triplets/`: native dependencies for arm64-android and x64-android (API 29).
- `djmantra-test.keystore`: signing key for **test builds only** (CI artifacts, emulator and phone
  testing). Store and key password `android`, alias `djmantra-test`, like Android's own debug key.
  It is deliberately public, so CI builds are reproducible and a new test build installs over the old
  one. Release builds for other DJs must be signed with a separate, private key that is never
  committed.
