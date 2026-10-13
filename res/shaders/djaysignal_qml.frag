#version 450

// DJ Mantra: djay Pro's scrolling waveform (Marko 13.10.2026: "djay algorithm
// waveform shape is paramount"; testing/djay/). One colour per point from the
// bands (src/waveform/djaycolor.h holds the same fitted numbers), a waveform
// mirrored around the middle: a bright body and a darker outline, so every hit
// reads as a coloured arrow (kicks red, hats blue / violet), on djay's #141414.
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
    return texture(waveformTexture, uv_data / float(textureStride));
}

vec3 djayColor(vec4 x) {
    // x.w = max(low, mid, high), as src/waveform/djaycolor.h
    vec4 v = sqrt(clamp(x, 0.0, 1.5));
    vec3 c = vec3(0.1561, 0.2868, 0.0815)
            + v.x * vec3(1.1164, 0.3428, -0.0047)
            + v.y * vec3(-0.5454, 1.3173, 0.3851)
            + v.z * vec3(-0.2553, -0.1464, 0.8582)
            + v.w * vec3(0.0784, -0.8494, -0.4954);
    float voiced = clamp((x.w - 0.02) / 0.06, 0.0, 1.0);
    return clamp(mix(vec3(0.19), c, voiced), 0.0, 1.0);
}

void main(void) {
    vec2 uv = qt_TexCoord0.st;
    vec4 outputColor = vec4(0.0);

    float idx = floor(firstVisualIndex + uv.x * (lastVisualIndex - firstVisualIndex)) * 2.0;
    if (idx >= 0.0 && idx <= float(waveformLength) - 2.0) {
        // both channels as one (djay draws mono)
        vec4 data = (getWaveformData(idx) + getWaveformData(idx + 1.0)) * 0.5;
        // round 32: the texture's alpha ("all") reached the phone as nothing (the
        // wave had no height); as the classic shader, everything from the 3 bands
        data.w = max(data.x, max(data.y, data.z));
        vec4 x = data * levelGain;
        x.x *= lowGain;
        x.y *= midGain;
        x.z *= highGain;
        vec3 colour = djayColor(x);
        // round 34 (Marko's djay screenshots of the zoomed lane): djay's lane colours
        // are far more saturated than the fitted average (red kicks, beige mids,
        // blue / violet highs, green breaks): 1.4x the distance from the grey (1.8
        // turned the drops green / cyan in the cloud preview)
        float luma = dot(colour, vec3(0.299, 0.587, 0.114));
        colour = clamp(vec3(luma) + (colour - vec3(luma)) * 1.4, 0.0, 1.0);
        colour = min(colour * 1.2, vec3(1.0));
        // round 35 (djay vs the app at the same bars): djay's kicks are crimson, not
        // orange, and its hats pink / lilac, not blue: less green where red leads,
        // some red where the highs lead
        float redness = clamp((colour.r - max(colour.g, colour.b)) * 2.0, 0.0, 1.0);
        colour.g *= 1.0 - 0.45 * redness;
        colour.b = min(1.0, colour.b + 0.12 * redness);
        float highness = clamp((x.z - max(x.x, x.y)) * 3.0, 0.0, 1.0);
        colour.r = min(1.0, colour.r + 0.15 * highness); // round 37: 0.35 was too magenta
        float dist = abs(uv.y - 0.5) * 2.0;
        // round 37 (djay vs V17): djay shows one distinct burst per hit with a thin
        // line between; an expansion (x^1.7) keeps the hits and thins what is between
        float outer = min(1.0, pow(clamp(x.w, 0.0, 1.5), 1.7) * heightGain);
        float core = outer * 0.55;
        if (dist <= core) {
            // round 35: djay's wave is translucent even in its body
            outputColor = vec4(colour * 0.85, 0.85);
        } else if (dist <= outer) {
            // the outer part fades out (djay: translucent streaks, no dark outline)
            float fade = 1.0 - (dist - core) / max(outer - core, 0.0001);
            float a = 0.15 + 0.55 * fade;
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
                d.w = max(d.x, max(d.y, d.z));
                vec3 c = djayColor(d * levelGain);
                float l = dot(c, vec3(0.299, 0.587, 0.114));
                c = clamp(vec3(l) + (c - vec3(l)) * 1.4, 0.0, 1.0);
                waveColor = vec4(c, 1.0);
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
