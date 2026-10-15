#version 450

// DJ Mantra's scrolling waveform. V77 (Marko 15.10.2026: "Look at the icon itself ... how
// waveform looks like. It looks very sharp, high resolution. It's not muddy, it's not blurry ...
// redesign our waveform to look like one in the icon itself. That's our role model"): as the
// launcher icon (tools/android/gen_launcher_icon.py), separate solid BARS with black gaps
// between them, each bar ONE pure band colour (bass red, mids green / cyan, highs blue /
// violet, bass + highs magenta; src/waveform/djaycolor.h spectralColor holds the same), its
// height the peak of the points it covers, mirrored round the middle, edges sharp to the
// device pixel. Bars belong to the track (they scroll with it, no shimmer); a bar is at least
// ~3 px, so zoomed out it takes the peak of several points. Before V77: per-pixel colours
// fitted to djay's average (beige / grey), a translucent body with fading edges, and the
// texture read between its points (each point blended with its neighbours): muddy.
// No names of GLSL built-ins (index, distance ...): round 31, the phone's GPU
// drew nothing.

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;

    vec2 framebufferSize;
    vec4 axesColor;

    int waveformLength;
    int textureSize;
    int textureStride;

    float levelGain;   // the bands / the song's loudest level (the colour's inputs)
    float heightGain;  // how far the loud parts reach (1 = the lane's edge)
    float lowGain;
    float midGain;
    float highGain;
    float firstVisualIndex;
    float lastVisualIndex;
    int fineLength;    // the real wave's points (0 = not built yet), qml/finewave.h
    int fineStride;
    float pixelRatio;  // device pixels per item pixel (sharp edges, V77)
    int waveStyle;     // 0 = 3-BAND (sections, V78), 1 = ICON (one colour per bar, V77)
};

layout(binding = 1) uniform sampler2D waveformTexture;
// the real wave: R = min, G = max (0.5 = silence), mono, scaled to the song's peak
layout(binding = 2) uniform sampler2D fineTexture;

vec2 fineAt(float point) {
    point = clamp(point, 0.0, float(fineLength - 1));
    float row = floor(point / float(fineStride));
    float col = point - row * float(fineStride);
    float rows = ceil(float(fineLength) / float(fineStride));
    vec4 t = texture(fineTexture, vec2((col + 0.5) / float(fineStride), (row + 0.5) / rows));
    return vec2(t.r, t.g) * 2.0 - 1.0;
}

vec4 getWaveformData(float position) {
    vec2 uv_data;
    uv_data.y = floor(position / float(textureStride));
    uv_data.x = floor(position - uv_data.y * float(textureStride));
    // the point's own texel, at its centre (V77: the corner read blended four points)
    return texture(waveformTexture, (uv_data + 0.5) / float(textureStride));
}

// the icon's colours (V77): each band a pure hue, the bar's hue their energy-weighted mix on
// the colour circle (bass 2°, mids 150°, highs 240°: bass + highs = magenta), full saturation;
// quiet bars darker, silence grey. src/waveform/djaycolor.h spectralColor is the same.
vec3 hueRgb(float h) {
    vec3 k = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return k;
}

vec3 spectralColor(vec3 b) {
    // the highs are stored compressed (^0.632) and every kick's click peaks in them: weighed
    // down, so kicks read red as in the icon
    vec3 w = b * b * vec3(1.0, 1.0, 0.45);
    float total = w.x + w.y + w.z;
    float loudest = max(b.x, max(b.y, b.z));
    if (total < 1e-6) {
        return vec3(0.19);
    }
    const float TAU = 6.2831853;
    vec2 v = w.x * vec2(cos(TAU * 2.0 / 360.0), sin(TAU * 2.0 / 360.0))
            + w.y * vec2(cos(TAU * 150.0 / 360.0), sin(TAU * 150.0 / 360.0))
            + w.z * vec2(cos(TAU * 240.0 / 360.0), sin(TAU * 240.0 / 360.0));
    float hue = fract(atan(v.y, v.x) / TAU + 1.0);
    // only the icon's colours (r42 V77: bass + mids blended to amber / yellow-green): a hue
    // between red-orange (14°) and green (135°) goes to the nearer of the two
    if (hue > 14.0 / 360.0 && hue < 135.0 / 360.0) {
        hue = hue < 74.5 / 360.0 ? 14.0 / 360.0 : 135.0 / 360.0;
    }
    // one band leads: pure; an even mix: a little paler (still no grey)
    float lead = length(v) / total;
    vec3 c = mix(vec3(1.0), hueRgb(hue), 0.78 + 0.22 * lead);
    c *= 0.62 + 0.38 * clamp(loudest, 0.0, 1.0);
    float voiced = clamp((loudest - 0.02) / 0.06, 0.0, 1.0);
    return mix(vec3(0.19), c, voiced);
}

void main(void) {
    vec2 uv = qt_TexCoord0.st;
    vec4 outputColor = vec4(0.0);

    float units = max(lastVisualIndex - firstVisualIndex, 1.0);
    float pxPerUnit = framebufferSize.x / units;           // item pixels per waveform point
    float pr = max(pixelRatio, 1.0);
    // a bar is at least ~3 px: a power of two of points (stable while zooming in steps)
    float barUnits = exp2(ceil(log2(max(1.0, 3.0 / max(pxPerUnit, 1e-4)))));
    float visual = firstVisualIndex + uv.x * units;
    float bar = floor(visual / barUnits);
    float barPx = barUnits * pxPerUnit;
    float inBar = (visual / barUnits - bar) * barPx;      // px from the bar's left edge
    float gap = clamp(barPx * 0.35, 1.0, 2.0);
    float body = barPx - gap;
    // the bar's coverage of this device pixel across (sharp, no shimmer while scrolling)
    float across = clamp(min(inBar, body - inBar) * pr + 0.5, 0.0, 1.0);
    float first = bar * barUnits;
    if (across > 0.0 && first >= 0.0 && first * 2.0 <= float(waveformLength) - 2.0) {
        // the bar's window: its own points, at least ~145 ms (64 points) round its middle, so a
        // bar does not flicker with every transient and a section reads as one texture;
        // 16 reads at most. Per band its peak and its mean square.
        float win = waveStyle == 0 ? max(barUnits, 64.0) : barUnits; // ICON: the bar's own points (V77)
        float from = first + 0.5 * barUnits - 0.5 * win;
        float pointStep = max(1.0, win / 16.0);
        vec4 peak = vec4(0.0);
        vec4 square = vec4(0.0);
        float count = 0.0;
        for (int k = 0; k < 16; ++k) {
            float point = from + float(k) * pointStep;
            if (point >= from + win) {
                break;
            }
            if (point < 0.0 || point * 2.0 > float(waveformLength) - 2.0) {
                continue;
            }
            // both channels as one (mono)
            vec4 d = (getWaveformData(point * 2.0) + getWaveformData(point * 2.0 + 1.0)) * 0.5;
            peak = max(peak, d);
            square += d * d;
            count += 1.0;
        }
        vec3 gains = vec3(lowGain, midGain, highGain) * levelGain;
        vec3 peakBands = peak.xyz * gains;
        vec3 level = mix(sqrt(square.xyz / max(count, 1.0)), peak.xyz, 0.5) * gains;
        float dist = abs(uv.y - 0.5) * 2.0;
        float halfHeight = framebufferSize.y * 0.5;
        float px = 1.0 / (halfHeight * pr);                 // one device pixel, in lane units
        if (waveStyle == 0) {
            // 3-BAND (V78, Marko 15.10.2026: "sections must be clearly seen through the waveform
            // shape and colors ... even when the vertical lines make mistake"; rekordbox's 3-band
            // view in the icon's colours): three nested layers per bar, BASS red at the back,
            // MIDS green in front of it, HIGHS blue-violet in front. Each colour is a band, never
            // a mix, so the colours do not change bar to bar; a section is a texture of its own:
            // a breakdown loses its red, a drop is wide red, vocals widen the green, hats the blue.
            // Each band's reach measured on 5 test songs (90th percentile of a band / the song's
            // loudest: bass 0.5, mids 0.35, highs 0.6, the highs stored compressed ^0.632): bass
            // fills most of the lane, mids a ring round the core, highs the core. Linear: a quieter
            // section is visibly smaller.
            vec3 reach = vec3(2.4, 2.6, 0.5);
            vec3 h = min(vec3(1.0), level * reach * heightGain);
            h = max(h, vec3(peakBands.x > 0.02 ? px : 0.0, 0.0, 0.0));
            vec3 cover = clamp((h - vec3(dist)) * halfHeight * pr + 0.5, 0.0, 1.0);
            vec3 colour = vec3(0.0);
            float a = 0.0;
            // back to front: bass, mids, highs
            colour = mix(colour, vec3(0.93, 0.13, 0.17), cover.x);
            a = max(a, cover.x);
            colour = mix(colour, vec3(0.16, 0.86, 0.55), cover.y);
            a = max(a, cover.y);
            colour = mix(colour, vec3(0.45, 0.48, 1.0), cover.z);
            a = max(a, cover.z);
            // silence: grey hairline
            if (max(level.x, max(level.y, level.z)) < 0.02) {
                colour = vec3(0.19);
            }
            a *= across;
            outputColor = vec4(colour * a, a);
        } else {
            // ICON (V77): one colour per bar from the bands' mix, as tall as the bar's peak
            float loud = max(peakBands.x, max(peakBands.y, peakBands.z));
            vec3 colour = spectralColor(peakBands);
            // the icon's full body (V77; round 37's x^1.7 made thin needles between the hits)
            float outer = min(1.0, pow(clamp(loud, 0.0, 1.5), 1.2) * heightGain);
            outer = max(outer, loud > 0.02 ? px : 0.0);
            float down = clamp((outer - dist) * halfHeight * pr + 0.5, 0.0, 1.0);
            float a = across * down;
            outputColor = vec4(colour * a, a);
        }
    }

    // Marko 13.10.2026 (djay's deep zoom; round 39 pairs): zoomed in, djay draws the
    // signed wave itself as ONE thin line (no fill, not mirrored): each kick a few clear
    // sine cycles. From ~12 points of the real wave per pixel down: the min..max of the
    // points a pixel covers (a line once a pixel holds one point or less, interpolated)
    if (fineLength > 0) {
        float pointsPerPixel = float(fineLength) / max(framebufferSize.x, 1.0);
        float weight = clamp((12.0 - pointsPerPixel) / 4.0, 0.0, 1.0);
        if (weight > 0.0) {
            float point = uv.x * float(fineLength);
            float lo;
            float hi;
            if (pointsPerPixel >= 1.0) {
                vec2 a = fineAt(floor(point - 0.5 * pointsPerPixel));
                vec2 b = fineAt(floor(point));
                vec2 c = fineAt(floor(point + 0.5 * pointsPerPixel));
                lo = min(a.x, min(b.x, c.x));
                hi = max(a.y, max(b.y, c.y));
            } else {
                // between two points: the line through their middles
                float p0 = floor(point - 0.5);
                vec2 a = fineAt(p0);
                vec2 b = fineAt(p0 + 1.0);
                float v = mix(0.5 * (a.x + a.y), 0.5 * (b.x + b.y), point - 0.5 - p0);
                lo = v;
                hi = v;
            }
            // half the lane's height for the song's peak (djay's line is low)
            float scale = 0.5 * heightGain;
            float y = (0.5 - uv.y) * 2.0;
            float px = 2.0 / max(framebufferSize.y, 1.0);
            vec4 waveColor = vec4(0.0);
            if (y >= lo * scale - 1.2 * px && y <= hi * scale + 1.2 * px) {
                float idx2 = floor(firstVisualIndex + uv.x * (lastVisualIndex - firstVisualIndex)) * 2.0;
                vec4 d = (getWaveformData(idx2) + getWaveformData(idx2 + 1.0)) * 0.5;
                waveColor = vec4(spectralColor(d.xyz * levelGain), 1.0);
            }
            outputColor = mix(outputColor, waveColor, weight);
        }
    }

    // the middle line, thin
    if (outputColor.a == 0.0 && abs(framebufferSize.y / 2.0 - gl_FragCoord.y) <= 0.5) {
        outputColor = vec4(axesColor.rgb * axesColor.a, axesColor.a);
    }
    fragColor = outputColor * qt_Opacity;
}
