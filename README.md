# djmantra_app

DJ Mantra Android app (Kotlin + Jetpack Compose).

## Project

| | |
|---|---|
| Package | `com.djmantra.app` |
| Min SDK | 26 (Android 8.0) |
| Target / compile SDK | 35 |
| UI | Jetpack Compose, Material 3 |
| Build | Gradle 8.11.1 (wrapper), AGP 8.7.3, Kotlin 2.1.0 |
| JDK | 17+ |

## Build & test

```bash
./gradlew assembleDebug              # build the APK -> app/build/outputs/apk/debug/
./gradlew testDebugUnitTest          # JVM unit + Robolectric tests (no emulator)
./gradlew lintDebug                  # Android lint
./gradlew connectedDebugAndroidTest  # instrumented tests (needs a device/emulator)
```

Requires the Android SDK (`ANDROID_HOME` or `local.properties` with `sdk.dir=...`).

## Testing strategy

1. **Unit + Robolectric tests** (`app/src/test`): run anywhere with a JDK, including the cloud dev container.
2. **Instrumented tests on a Pixel 7 emulator** (`app/src/androidTest`): run in GitHub Actions
   (`.github/workflows/android.yml`, API 34, `pixel_7` profile) on every push and PR.
3. **Hands-on checks**: open the project in Android Studio on your own machine and run it on a
   Pixel 7 AVD (Device Manager → Pixel 7 → API 34+).

CI also uploads the debug APK as the `app-debug-apk` artifact, so you can sideload a build onto
a phone or emulator without building locally.
