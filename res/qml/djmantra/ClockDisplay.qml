import "."
import QtQuick 2.12

// A real-time clock in the matrix display style, top of the screen (Marko,
// 12.10.2026: "real-time clock in the matrix display style"): HH:MM, 24 hours,
// the colon static (Marko: "2 dots ... should be static, not blinking");
// checked once a second, repainted only when the minute turns. The hidden easter egg
// (Marko 14.10.2026): a long press on the clock starts / stops recording the mix, and
// the clock is red while it records.
Canvas {
    id: root

    property string time: "--:--"
    readonly property int columns: 5 * 6 + 1
    readonly property int rows: 7 + 2
    readonly property real pitch: Math.min(width / columns, height / rows)

    readonly property bool recording: Recorder.recording
    onRecordingChanged: requestPaint()

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
            const time = two(now.getHours()) + ":" + two(now.getMinutes());
            if (time !== root.time) {
                root.time = time;
                root.requestPaint();
            }
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
            const g = PixelFont.glyph(time[i]);
            for (let gx = 0; gx < 5; ++gx)
                for (let gy = 0; gy < 7; ++gy)
                    if (g[gy] & (16 >> gx))
                        lit[(1 + i * 6 + gx) + "," + (1 + gy)] = true;
        }
        for (let y = 0; y < rows; ++y)
            for (let x = 0; x < columns; ++x) {
                ctx.fillStyle = lit[x + "," + y] ? String(root.recording ? Style.red : Style.accent) : "#121735";
                ctx.fillRect(x * p + (p - dot) / 2, y * p + (p - dot) / 2, dot, dot);
            }
    }

    MouseArea {
        anchors.fill: parent
        pressAndHoldInterval: 600
        onPressAndHold: Recorder.toggle()
    }
}
