# Phone test: CI run #30 (`a865b67`), M5 milestone, 2026-10-08 15:38

| | Pixel 7 emulator | real Pixel 7 |
|---|---|---|
| Device | AVD Pixel_7_API_35, Android 15 (API 35), arm64-v8a, Google Play image | Pixel 7, Android 16 (API 36), arm64-v8a, adb over Wi-Fi (USB-C free) |
| APK | `djmantra-arm64-v8a.apk` from run 37783697815, signed "DJ Mantra test builds" | same |
| Install (fresh, no permissions pre-granted) | pass | pass |
| `am start -W` | Status ok, WaitTime 372 ms (to the splash) | Status ok, WaitTime 903 ms (to the splash) |
| Still running after 10 s | **no** | **no** |
| Crash | `FATAL EXCEPTION: qtMainLoopThread` → `UnsatisfiedLinkError: dlopen failed: library "libomp.so" not found: needed by …/lib/arm64/libmixxx_arm64-v8a.so` | the same |
| Logcat (`-s DJMantraMIDI:* DJMantraStorage:* AndroidRuntime:E`) | [logcat_djmantra.txt](2026-10-08_run30_emulator/logcat_djmantra.txt): only the crash, no DJMantraMIDI / DJMantraStorage line | [logcat_djmantra.txt](2026-10-08_run30_pixel7/logcat_djmantra.txt): the same |
| Screen | [after_launch](2026-10-08_run30_emulator/01_after_launch.png): home screen | [after_launch](2026-10-08_run30_pixel7/after_launch.png): home screen |

## Why it still crashes

- `llvm-readelf -d libmixxx_arm64-v8a.so` (run #30) still lists **`NEEDED libomp.so`**, and the APK's `lib/arm64-v8a/` (97 files) has no libomp.so. 4934eb2 ("link OpenMP statically for real") did not take effect.
- The only OpenMP symbol libmixxx still imports is **`omp_get_thread_num`** (undefined dynamic symbol). Everything else from OpenMP is resolved, so one object (most likely Sleef's DFT, used by Rubber Band; built in vcpkg with `-fopenmp`) still links the NDK's shared libomp.
- **The CI check found it but did not fail the run.** Run #30's "Check native libraries" step printed
  `MISSING: libmixxx_arm64-v8a.so needs libomp.so (not in the APK, not an Android system library)` (and the same for x86_64),
  but the step is `tools/android/check_apk_libs.sh … | tee -a "$GITHUB_STEP_SUMMARY"`: without `set -o pipefail` the pipe's status is `tee`'s, so the job stays green.

## Checklist of the milestone

| check | emulator | Pixel 7 | notes |
|---|---|---|---|
| App starts and stays running (no libomp crash) | **fail** | **fail** | see above |
| Launch time (tap → first screen) | not tested | not tested | only the splash: 372 / 903 ms to the crash |
| "All files access" prompt | not tested | not tested | the crash comes first |
| Bluetooth ("Nearby devices") prompt | not tested | not tested | |
| USB stick through the folder picker (SAF), Folders, offline dots, unplug while playing | – | not tested | needs a running app |
| Mix Ultra over USB-C / Bluetooth, DJMantraMIDI lines, LEDs, mapping, jog, tempo direction | – | not tested | needs a running app; Android itself sees the controller (USB MIDI type 1 and BLE advertising, earlier report) |
| Beat lights per pad mode (camera photos) | – | not tested | needs a playing deck |

Nothing was asked of Marko this round: every hand step depends on the app starting.

## Problems found / suggestions for the cloud session

1. **libomp.so still NEEDED** (only for `omp_get_thread_num`). The surest fix: package the NDK's own `libomp.so` for each ABI
   (`$NDK/toolchains/llvm/prebuilt/linux-x86_64/lib/clang/<ver>/lib/linux/<arch>/libomp.so` → `QT_ANDROID_EXTRA_LIBS` / the
   APK's `lib/<abi>/`). Or link the static `libomp.a` from the same folder into libmixxx (`-Wl,--whole-archive`-free, just
   the archive before `-lc++_shared`) and make sure no target links `libomp.so` by path (check `CMakeFiles/mixxx.dir/link.txt`
   for `libomp.so`), or build Sleef without OpenMP (`SLEEFDFT_ENABLE_OPENMP=OFF` in the vcpkg port/triplet; Rubber Band does
   not need Sleef's threads).
2. **Make the CI check fail the job**: add `set -o pipefail` to that step (or `shell: bash` with `-eo pipefail`, or run the
   script first and `tee` its saved output). Then a red run shows the problem before a phone test.
3. Once it starts, the next local test runs the whole M5 list (it is ready: controller, stick and camera are set up).
