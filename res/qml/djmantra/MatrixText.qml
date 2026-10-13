import "."
import QtQuick 2.12

// Text in the matrix display style of the clock (ClockDisplay.qml, PixelFont.qml):
// 5x7 dots per character, lit in the accent colour on dark dots; repainted only when
// the text changes (Marko 13.10.2026: the playlist's statistics in this font).
Canvas {
    id: root

    property string text: ""
    property color litColor: Style.accent
    readonly property int columns: Math.max(1, text.length) * 6 + 1
    readonly property int rows: 7 + 2
    readonly property real pitch: height / rows

    implicitHeight: 20
    width: columns * pitch
    onTextChanged: requestPaint()
    onLitColorChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const p = pitch;
        const dot = p * 0.8;
        const lit = {};
        for (let i = 0; i < text.length; ++i) {
            const g = PixelFont.glyph(text[i]);
            for (let gx = 0; gx < 5; ++gx)
                for (let gy = 0; gy < 7; ++gy)
                    if (g[gy] & (16 >> gx))
                        lit[(1 + i * 6 + gx) + "," + (1 + gy)] = true;
        }
        for (let y = 0; y < rows; ++y)
            for (let x = 0; x < columns; ++x) {
                ctx.fillStyle = lit[x + "," + y] ? String(root.litColor) : "#121735";
                ctx.fillRect(x * p + (p - dot) / 2, y * p + (p - dot) / 2, dot, dot);
            }
    }
}
