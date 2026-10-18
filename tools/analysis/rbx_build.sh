#!/bin/sh
# Builds tools/analysis/rbx_analyse (the rbxport port, offline) with qm-dsp's FFT. Usage: rbx_build.sh OUT
set -e
R=$(cd "$(dirname "$0")/../.." && pwd)
Q="$R/lib/qm-dsp"
g++ -O2 -std=c++17 -o "${1:-rbx_analyse}" -Dkiss_fft_scalar=double -I"$R/src" -I"$Q" -I"$Q/ext/kissfft" \
  "$R/tools/analysis/rbx_analyse.cpp" "$R/src/analyzer/rbxanalysis.cpp" "$R/src/analyzer/sectionnames.cpp" "$Q/dsp/transforms/FFT.cpp" \
  "$Q/maths/MathUtilities.cpp" -x c "$Q/ext/kissfft/kiss_fft.c" "$Q/ext/kissfft/tools/kiss_fftr.c" -lm
