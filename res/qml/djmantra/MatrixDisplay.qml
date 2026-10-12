import "."
import QtQuick 2.12

// A dot-matrix display, as on the OXI E16 (Marko, 12.10.2026): dark glass, a
// grid of square pixels, a 5x7 pixel font, lit dots in the accent colours,
// the unlit ones faintly visible. Two halves (deck 1 left, deck 2 right),
// three lines each; a line is a list of {text, color} pieces. A piece with
// scroll: true runs through its room when it is too long (the titles).
Canvas {
    id: root

    property var left: []   // [[{text, color, scroll}], line 2, line 3]
    property var right: []
    property int scrollStep: 0
    readonly property int lines: 3
    readonly property int rowsPerLine: 8 // 7 dots + 1 gap
    readonly property real pitch: height / (lines * rowsPerLine + 1)
    readonly property int columns: Math.floor(width / pitch)

    // 5x7 font: 7 rows of 5 bits per glyph
    readonly property var glyphs: ({
        "0": [14,17,19,21,25,17,14], "1": [4,12,4,4,4,4,14], "2": [14,17,1,2,4,8,31],
        "3": [31,2,4,2,1,17,14], "4": [2,6,10,18,31,2,2], "5": [31,16,30,1,1,17,14],
        "6": [6,8,16,30,17,17,14], "7": [31,1,2,4,8,8,8], "8": [14,17,17,14,17,17,14],
        "9": [14,17,17,15,1,2,12], "A": [14,17,17,31,17,17,17], "B": [30,17,17,30,17,17,30],
        "C": [14,17,16,16,16,17,14], "D": [28,18,17,17,17,18,28], "E": [31,16,16,30,16,16,31],
        "F": [31,16,16,30,16,16,16], "G": [14,17,16,23,17,17,15], "H": [17,17,17,31,17,17,17],
        "I": [14,4,4,4,4,4,14], "J": [7,2,2,2,2,18,12], "K": [17,18,20,24,20,18,17],
        "L": [16,16,16,16,16,16,31], "M": [17,27,21,21,17,17,17], "N": [17,17,25,21,19,17,17],
        "O": [14,17,17,17,17,17,14], "P": [30,17,17,30,16,16,16], "Q": [14,17,17,17,21,18,13],
        "R": [30,17,17,30,20,18,17], "S": [15,16,16,14,1,1,30], "T": [31,4,4,4,4,4,4],
        "U": [17,17,17,17,17,17,14], "V": [17,17,17,17,17,10,4], "W": [17,17,17,21,21,21,10],
        "X": [17,17,10,4,10,17,17], "Y": [17,17,17,10,4,4,4], "Z": [31,1,2,4,8,16,31],
        " ": [0,0,0,0,0,0,0], ".": [0,0,0,0,0,12,12], "-": [0,0,0,31,0,0,0],
        ":": [0,12,12,0,12,12,0], "/": [0,1,2,4,8,16,0], "|": [4,4,4,4,4,4,4],
        ">": [8,4,2,1,2,4,8], "%": [24,25,2,4,8,19,3], "?": [14,17,1,2,4,0,4],
        "'": [4,4,0,0,0,0,0], "&": [12,18,20,8,21,18,13], "(": [2,4,8,8,8,4,2], ")": [8,4,2,2,2,4,8]
    })

    function glyph(c) {
        const g = glyphs[c.toUpperCase()];
        return g !== undefined ? g : glyphs["?"];
    }

    Timer {
        // the titles run through when they are longer than their room
        interval: 160
        repeat: true
        running: root.visible
        onTriggered: {
            root.scrollStep += 1;
            root.requestPaint();
        }
    }

    onLeftChanged: requestPaint()
    onRightChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const p = pitch;
        const dot = p * 0.78;
        // dark glass
        ctx.fillStyle = "#04060e";
        ctx.fillRect(0, 0, width, height);
        const rows = lines * rowsPerLine + 1;
        // the lit pixels of this frame: "x,y" -> colour
        const lit = {};
        const half = Math.floor(columns / 2);
        const drawHalf = (lineSet, startColumn, room) => {
            for (let l = 0; l < Math.min(lines, lineSet.length); ++l) {
                let column = startColumn + 1;
                const top = 1 + l * rowsPerLine;
                for (const piece of lineSet[l]) {
                    let text = String(piece.text);
                    let pieceRoom = startColumn + room - 1 - column;
                    let shift = 0;
                    const textColumns = text.length * 6;
                    if (piece.scroll && textColumns > pieceRoom) {
                        const loop = textColumns + 6;
                        shift = scrollStep % loop;
                        text = text + "   " + text;
                    }
                    for (let i = 0; i < text.length; ++i) {
                        const g = glyph(text[i]);
                        for (let gx = 0; gx < 5; ++gx) {
                            const x = column + i * 6 + gx - shift;
                            if (x < column || x >= startColumn + room - 1 || x < startColumn + 1)
                                continue;
                            for (let gy = 0; gy < 7; ++gy) {
                                if (g[gy] & (16 >> gx))
                                    lit[x + "," + (top + gy)] = piece.color;
                            }
                        }
                    }
                    column += piece.scroll ? pieceRoom + 1 : textColumns;
                }
            }
        };
        drawHalf(left, 0, half);
        drawHalf(right, half, columns - half);
        for (let y = 0; y < rows; ++y) {
            for (let x = 0; x < columns; ++x) {
                const c = lit[x + "," + y];
                ctx.fillStyle = c !== undefined ? c : "#121735";
                ctx.fillRect(x * p + (p - dot) / 2, y * p + (p - dot) / 2, dot, dot);
            }
        }
        // the gap between the halves
        ctx.fillStyle = "#04060e";
        ctx.fillRect(half * p - p * 0.2, 0, p * 0.4, height);
    }
}
