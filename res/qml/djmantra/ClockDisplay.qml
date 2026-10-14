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
    // Marko 14.10.2026: while recording, two rows: the clock on top, the recording's
    // time under it (m:ss, h:mm:ss after an hour), both red
    property string recTime: ""
    readonly property var lines: recording ? [time, recTime] : [time]
    readonly property int longest: Math.max(5, ...lines.map(l => l.length))
    readonly property int columns: longest * 6 + 1
    readonly property int rows: lines.length * 8 + 1
    readonly property real pitch: Math.min(width / columns, height / rows)

    readonly property bool recording: Recorder.recording
    onRecordingChanged: {
        recTime = "0:00";
        requestPaint();
    }
    onRecTimeChanged: requestPaint()

    implicitHeight: 24
    implicitWidth: columns * height / rows

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

    Timer {
        // the recording's time, twice a second (it turns on the whole second)
        interval: 500
        repeat: true
        running: root.recording
        onTriggered: {
            const t = Math.max(0, Math.floor((Date.now() - Recorder.startedAt) / 1000));
            const two = (n) => (n < 10 ? "0" : "") + n;
            const h = Math.floor(t / 3600), m = Math.floor(t / 60) % 60, sec = t % 60;
            root.recTime = h > 0 ? h + ":" + two(m) + ":" + two(sec) : m + ":" + two(sec);
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
        for (let line = 0; line < lines.length; ++line) {
            const text = lines[line];
            // each row centred
            const left = 1 + Math.floor((columns - 1 - text.length * 6) / 2);
            const top = 1 + line * 8;
            for (let i = 0; i < text.length; ++i) {
                const g = PixelFont.glyph(text[i]);
                for (let gx = 0; gx < 5; ++gx)
                    for (let gy = 0; gy < 7; ++gy)
                        if (g[gy] & (16 >> gx))
                            lit[(left + i * 6 + gx) + "," + (top + gy)] = true;
            }
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
