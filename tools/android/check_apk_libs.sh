#!/usr/bin/env bash
# Check that every shared library an APK's native code needs is there:
# each NEEDED entry of each lib/<abi>/*.so must be packaged in the APK or be
# an Android system library (one of the NDK sysroot's stubs for the minimum
# API level). Catches "dlopen failed: library ... not found" at build time.
#
#   check_apk_libs.sh <apk> <abi> <min-api> [<ndk-root>]
#
# READELF can be set to a readelf that reads the APK's architecture (default:
# the NDK's llvm-readelf). SYSROOT_LIBS can point to a directory of system
# library stubs instead of the NDK's (used by the self test).
set -euo pipefail

apk=${1:?apk}
abi=${2:?abi}
api=${3:?min api}
ndk=${4:-${ANDROID_NDK_ROOT:-}}

case "$abi" in
  arm64-v8a) triple=aarch64-linux-android ;;
  armeabi-v7a) triple=arm-linux-androideabi ;;
  x86_64) triple=x86_64-linux-android ;;
  x86) triple=i686-linux-android ;;
  *) echo "unknown ABI $abi" >&2; exit 2 ;;
esac

if [ -z "${READELF:-}" ]; then
  READELF=$(ls "$ndk"/toolchains/llvm/prebuilt/*/bin/llvm-readelf 2>/dev/null | head -1)
fi
[ -x "${READELF:-}" ] || command -v "${READELF:-}" > /dev/null || { echo "no readelf (set READELF or pass the NDK)" >&2; exit 2; }
if [ -z "${SYSROOT_LIBS:-}" ]; then
  SYSROOT_LIBS=$(ls -d "$ndk"/toolchains/llvm/prebuilt/*/sysroot/usr/lib/"$triple"/"$api" 2>/dev/null | head -1)
fi
[ -d "${SYSROOT_LIBS:-}" ] || { echo "no system library stubs for $triple API $api" >&2; exit 2; }

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
unzip -q -o "$apk" "lib/$abi/*" -d "$work" || { echo "no lib/$abi in $apk" >&2; exit 1; }

declare -A available
for f in "$work/lib/$abi"/*.so; do available[$(basename "$f")]=apk; done
for f in "$SYSROOT_LIBS"/*.so; do available[$(basename "$f")]=system; done

missing=0
for f in "$work/lib/$abi"/*.so; do
  while read -r needed; do
    [ -n "$needed" ] || continue
    if [ -z "${available[$needed]:-}" ]; then
      echo "MISSING: $(basename "$f") needs $needed (not in the APK, not an Android system library)"
      missing=1
    fi
  done < <("$READELF" -d "$f" | sed -n 's/.*(NEEDED).*\[\(.*\)\].*/\1/p')
done

if [ "$missing" -ne 0 ]; then
  echo "The APK would crash at launch. Package the libraries above or link them statically." >&2
  exit 1
fi
echo "OK: every NEEDED library of lib/$abi/*.so is in the APK or a system library (API $api)"
