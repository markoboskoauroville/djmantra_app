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

/// V77 (Marko 15.10.2026: the waveform as sharp as the launcher icon's): the icon's colours.
/// Each band a pure hue (bass 2°, mids 150°, highs 240°), a bar's hue their energy-weighted mix
/// on the colour circle (bass + highs = magenta), nearly full saturation; quiet bars darker,
/// silence grey. Inputs as djayColor's (divided by the song's loudest band). The same as
/// spectralColor in res/shaders/djaysignal_qml.frag.
inline QColor spectralColor(double low, double mid, double high) {
    constexpr double kTau = 6.283185307179586;
    // the highs are stored compressed and every kick's click peaks in them: weighed down
    const double wl = low * low;
    const double wm = mid * mid;
    const double wh = high * high * 0.45;
    const double total = wl + wm + wh;
    const double loudest = std::max({low, mid, high});
    constexpr double kGrey = 0.19;
    if (total < 1e-6) {
        return QColor::fromRgbF(kGrey, kGrey, kGrey);
    }
    const auto dir = [](double degrees) {
        return degrees * kTau / 360.0;
    };
    const double x = wl * std::cos(dir(2)) + wm * std::cos(dir(150)) + wh * std::cos(dir(240));
    const double y = wl * std::sin(dir(2)) + wm * std::sin(dir(150)) + wh * std::sin(dir(240));
    double hue = std::atan2(y, x) / kTau;
    hue -= std::floor(hue);
    // only the icon's colours (r42: amber / yellow-green between bass and mids): a hue between
    // red-orange (14°) and green (135°) goes to the nearer of the two
    if (hue > 14.0 / 360.0 && hue < 135.0 / 360.0) {
        hue = hue < 74.5 / 360.0 ? 14.0 / 360.0 : 135.0 / 360.0;
    }
    const double lead = std::hypot(x, y) / total;
    const double saturation = 0.78 + 0.22 * lead;
    const double value = 0.62 + 0.38 * std::clamp(loudest, 0.0, 1.0);
    const double voiced = std::clamp((loudest - 0.02) / 0.06, 0.0, 1.0);
    // the hue's pure colour, paled by saturation, darkened by value, faded from grey
    double rgb[3];
    for (int i = 0; i < 3; ++i) {
        const double k = std::fmod(hue * 6.0 + (i == 0 ? 0.0 : i == 1 ? 4.0 : 2.0), 6.0);
        const double pure = std::clamp(std::abs(k - 3.0) - 1.0, 0.0, 1.0);
        const double c = (1.0 + (pure - 1.0) * saturation) * value;
        rgb[i] = kGrey + (c - kGrey) * voiced;
    }
    return QColor::fromRgbF(static_cast<float>(std::clamp(rgb[0], 0.0, 1.0)),
            static_cast<float>(std::clamp(rgb[1], 0.0, 1.0)),
            static_cast<float>(std::clamp(rgb[2], 0.0, 1.0)));
}

} // namespace djmantra
