#pragma once

#include <QColor>
#include <algorithm>
#include <cmath>

/// DJ Mantra: djay Pro's waveform colour (Marko 13.10.2026: "djay algorithm waveform
/// shape is paramount"). Fitted to djay Pro 5's own colour per 256 samples
/// (.djayMetadata, 24 of Marko's songs, testing/djay/) from the bands Mixxx's
/// waveform analysis stores (Bessel 4: low < 600 Hz < mid < 4 kHz < high; peak per
/// point; high ^0.632), each divided by the song's loudest "all" (99.5th percentile):
///   rgb = bias + sum over band of weight * sqrt(band)
/// Error 21 / 255 per channel against djay (a flat mean colour: 43). Silence fades to
/// djay's dark grey. res/shaders/djaysignal_qml.frag holds the same numbers.
namespace djmantra {

inline QColor djayColor(double low, double mid, double high, double all) {
    const auto v = [](double x) {
        return std::sqrt(std::clamp(x, 0.0, 1.5));
    };
    const double l = v(low);
    const double m = v(mid);
    const double h = v(high);
    const double a = v(all);
    double r = 0.1597 + 1.0319 * l - 0.4393 * m - 0.3169 * h + 0.1319 * a;
    double g = 0.2188 - 0.0081 * l + 1.3782 * m - 0.5140 * h - 0.0914 * a;
    double b = 0.0633 - 0.0594 * l + 0.5831 * m + 0.6696 * h - 0.2709 * a;
    // silence: djay's dark grey (0x3186)
    const double voiced = std::clamp((all - 0.02) / 0.06, 0.0, 1.0);
    constexpr double kGrey = 0.19;
    r = kGrey + (r - kGrey) * voiced;
    g = kGrey + (g - kGrey) * voiced;
    b = kGrey + (b - kGrey) * voiced;
    return QColor::fromRgbF(static_cast<float>(std::clamp(r, 0.0, 1.0)),
            static_cast<float>(std::clamp(g, 0.0, 1.0)),
            static_cast<float>(std::clamp(b, 0.0, 1.0)));
}

} // namespace djmantra
