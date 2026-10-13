#version 450

// DJ Mantra: djay Pro's scrolling waveform (Marko 13.10.2026: "djay algorithm
// waveform shape is paramount"; testing/djay/). One colour per point from the
// bands (src/waveform/djaycolor.h holds the same fitted numbers), a waveform
// mirrored around the middle: a bright body and a darker outline, so every hit
// reads as a coloured arrow (kicks red, hats blue / violet), on djay's #141414.

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
};

layout(binding = 1) uniform sampler2D waveformTexture;

vec4 getWaveformData(float index) {
    vec2 uv_data;
    uv_data.y = floor(index / float(textureStride));
    uv_data.x = floor(index - uv_data.y * float(textureStride));
    return texture(waveformTexture, uv_data / float(textureStride));
}

vec3 djayColor(vec4 x) {
    vec4 v = sqrt(clamp(x, 0.0, 1.5));
    vec3 c = vec3(0.1597, 0.2188, 0.0633)
            + v.x * vec3(1.0319, -0.0081, -0.0594)
            + v.y * vec3(-0.4393, 1.3782, 0.5831)
            + v.z * vec3(-0.3169, -0.5140, 0.6696)
            + v.w * vec3(0.1319, -0.0914, -0.2709);
    float voiced = clamp((x.w - 0.02) / 0.06, 0.0, 1.0);
    return clamp(mix(vec3(0.19), c, voiced), 0.0, 1.0);
}

void main(void) {
    vec2 uv = qt_TexCoord0.st;
    vec4 outputColor = vec4(0.0);

    float index = floor(firstVisualIndex + uv.x * (lastVisualIndex - firstVisualIndex)) * 2.0;
    if (index >= 0.0 && index <= float(waveformLength) - 2.0) {
        // both channels as one (djay draws mono)
        vec4 data = (getWaveformData(index) + getWaveformData(index + 1.0)) * 0.5;
        vec4 x = data * levelGain;
        x.x *= lowGain;
        x.y *= midGain;
        x.z *= highGain;
        vec3 colour = djayColor(x);
        float distance = abs(uv.y - 0.5) * 2.0;
        float outer = x.w * heightGain;
        float inner = outer * 0.6;
        if (distance <= inner) {
            outputColor = vec4(min(colour * 1.15, vec3(1.0)), 1.0);
        } else if (distance <= outer) {
            outputColor = vec4(colour * 0.6, 1.0);
        }
    }
    // the middle line, thin
    if (outputColor.a == 0.0 && abs(framebufferSize.y / 2.0 - gl_FragCoord.y) <= 0.5) {
        outputColor = vec4(axesColor.rgb * axesColor.a, axesColor.a);
    }
    fragColor = outputColor * qt_Opacity;
}
