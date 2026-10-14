#pragma once

#include <QColor>
#include <algorithm>
#include <cmath>

/// DJ Mantra: djay Pro's waveform colour (Marko 13.10.2026: "djay algorithm waveform
/// shape is paramount"). Fitted to djay Pro 5's own colour per 256 samples
/// (.djayMetadata, 24 of Marko's songs, testing/djay/) from the bands exactly as Mixxx's
/// waveform analysis stores them (fidlib Bessel 4, -3 dB at 600 Hz / 4 kHz; peak per
/// point; low and mid linear, high ^0.632), each divided by the song's loudest band
/// (99th percentile of max(low, mid, high)); a = max(low, mid, high):
///   rgb = bias + sum of weight * sqrt(band)
/// Error 19.7 / 255 per channel against djay (a flat mean colour: 43). Round 32: the
/// first fit (other filter normalisation, "all" as the loudness) came out orange-brown
/// on the phone. Silence fades to djay's dark grey. res/shaders/djaysignal_qml.frag
/// holds the same numbers.
namespace djmantra {

inline QColor djayColor(double low, double mid, double high) {
    const auto v = [](double x) {
        return std::sqrt(std::clamp(x, 0.0, 1.5));
    };
    const double loudest = std::max({low, mid, high});
    const double l = v(low);
    const double m = v(mid);
    const double h = v(high);
    const double a = v(loudest);
    double r = 0.1561 + 1.1164 * l - 0.5454 * m - 0.2553 * h + 0.0784 * a;
    double g = 0.2868 + 0.3428 * l + 1.3173 * m - 0.1464 * h - 0.8494 * a;
    double b = 0.0815 - 0.0047 * l + 0.3851 * m + 0.8582 * h - 0.4954 * a;
    // silence: djay's dark grey (0x3186)
    const double voiced = std::clamp((loudest - 0.02) / 0.06, 0.0, 1.0);
    constexpr double kGrey = 0.19;
    r = kGrey + (r - kGrey) * voiced;
    g = kGrey + (g - kGrey) * voiced;
    b = kGrey + (b - kGrey) * voiced;
    return QColor::fromRgbF(static_cast<float>(std::clamp(r, 0.0, 1.0)),
            static_cast<float>(std::clamp(g, 0.0, 1.0)),
            static_cast<float>(std::clamp(b, 0.0, 1.0)));
}

} // namespace djmantra
