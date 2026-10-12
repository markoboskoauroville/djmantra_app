import "."
import QtQuick 2.12

// A real-time clock in the matrix display style, top of the screen (Marko,
// 12.10.2026: "real-time clock in the matrix display style"): HH:MM, 24 hours,
// the colon blinking each second. Repainted once a second, nothing more.
Canvas {
    id: root

    property string time: "--:--"
    property bool colon: true
    readonly property int columns: 5 * 6 + 1
    readonly property int rows: 7 + 2
    readonly property real pitch: Math.min(width / columns, height / rows)

    implicitHeight: 24
    implicitWidth: columns * implicitHeight / rows

    Timer {
        interval: 1000
        repeat: true
        running: true
        triggeredOnStart: true
        onTriggered: {
            const now = new Date();
            const two = (n) => (n < 10 ? "0" : "") + n;
            root.time = two(now.getHours()) + ":" + two(now.getMinutes());
            root.colon = now.getSeconds() % 2 === 0;
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
        for (let i = 0; i < time.length; ++i) {
            if (time[i] === ":" && !colon)
                continue;
            const g = PixelFont.glyph(time[i]);
            for (let gx = 0; gx < 5; ++gx)
                for (let gy = 0; gy < 7; ++gy)
                    if (g[gy] & (16 >> gx))
                        lit[(1 + i * 6 + gx) + "," + (1 + gy)] = true;
        }
        for (let y = 0; y < rows; ++y)
            for (let x = 0; x < columns; ++x) {
                ctx.fillStyle = lit[x + "," + y] ? String(Style.accent) : "#121735";
                ctx.fillRect(x * p + (p - dot) / 2, y * p + (p - dot) / 2, dot, dot);
            }
    }
}
