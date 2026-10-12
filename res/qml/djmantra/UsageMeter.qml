import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A small dot-matrix display, top right (Marko, 12.10.2026: "GPU and CPU ...
// percentage of using while I'm running the app"): the app's CPU use (share
// of all the phone's cores) and the GPU's load, once a second. Green below
// 50 %, yellow below 80 %, red above. "--": the phone does not tell an app its
// GPU load.
Canvas {
    id: root

    property int cpu: -1
    property int gpu: -1
    readonly property int columns: 7 * 6 + 1
    readonly property int rows: 2 * 8 + 1
    readonly property real pitch: Math.min(width / columns, height / rows)

    implicitHeight: 34
    implicitWidth: columns * implicitHeight / rows

    function level(value) {
        return value < 0 ? String(Style.subText) : value < 50 ? "#4cd964" : value < 80 ? String(Style.accent) : "#ff3b30";
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        triggeredOnStart: true
        onTriggered: {
            const u = Mixxx.DJMantra.usage();
            root.cpu = u.cpu;
            root.gpu = u.gpu;
            root.requestPaint();
        }
    }

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const p = pitch;
        const dot = p * 0.8;
        ctx.fillStyle = "#04060e";
        ctx.fillRect(0, 0, width, height);
        const lit = {};
        const text = (string, column, top, color) => {
            for (let i = 0; i < string.length; ++i) {
                const g = PixelFont.glyph(string[i]);
                for (let gx = 0; gx < 5; ++gx)
                    for (let gy = 0; gy < 7; ++gy)
                        if (g[gy] & (16 >> gx))
                            lit[(column + i * 6 + gx) + "," + (top + gy)] = color;
            }
        };
        const value = (v) => v < 0 ? "--" : String(Math.min(999, v)) + "%";
        const line = (label, v, top) => {
            text(label, 1, top, String(Style.subText));
            const s = value(v);
            text(s, columns - s.length * 6, top, level(v));
        };
        line("CPU", cpu, 1);
        line("GPU", gpu, 9);
        for (let y = 0; y < rows; ++y)
            for (let x = 0; x < columns; ++x) {
                const c = lit[x + "," + y];
                ctx.fillStyle = c !== undefined ? c : "#121735";
                ctx.fillRect(x * p + (p - dot) / 2, y * p + (p - dot) / 2, dot, dot);
            }
    }
}
