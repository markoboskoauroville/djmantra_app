import "."
import QtQuick 2.12

// A dot-matrix display, as on the OXI E16 (Marko, 12.10.2026): dark glass, a
// grid of square pixels, a 5x7 pixel font, lit dots in the accent colours,
// the unlit ones faintly visible. Its job (Marko): per deck, the current bar
// and beat (17.3), ONE countdown to the next phrase / section change (-6.2)
// and the running lights of the 8-beat phrase, LED cells of the same pixels.
// Deck 1 over deck 2, so the numbers and the lights compare at a glance.
Canvas {
    id: root

    // [{number, barBeat, countdown, beat (0..7, -1: none), color}] per deck
    property var decks: []
    readonly property int cellRows: 3 // a phrase LED cell is 3 dots high
    readonly property int deckRows: 1 + 7 + 1 + cellRows // gap, numbers, gap, cells
    readonly property int totalRows: 2 * deckRows + 1
    readonly property real pitch: height / totalRows
    readonly property int columns: Math.floor(width / pitch)
    readonly property color glass: "#04060e"
    readonly property color unlit: "#121735"

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

    onDecksChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const p = pitch;
        const dot = p * 0.78;
        ctx.fillStyle = glass;
        ctx.fillRect(0, 0, width, height);
        const lit = {}; // "x,y" -> colour
        const text = (string, column, top, color) => {
            for (let i = 0; i < string.length; ++i) {
                const g = glyph(string[i]);
                for (let gx = 0; gx < 5; ++gx) {
                    const x = column + i * 6 + gx;
                    if (x < 0 || x >= columns)
                        continue;
                    for (let gy = 0; gy < 7; ++gy) {
                        if (g[gy] & (16 >> gx))
                            lit[x + "," + (top + gy)] = color;
                    }
                }
            }
        };
        const textWidth = (string) => string.length * 6 - 1;
        // the phrase cells under the numbers, across the whole width
        const cellGap = 2;
        const cellRoom = columns - 2;
        const cellWidth = Math.max(2, Math.floor((cellRoom - 7 * cellGap) / 8));
        const firstCell = Math.floor((columns - (8 * cellWidth + 7 * cellGap)) / 2);
        const dimCell = "#252d5c";
        const dimFirst = "#4a4326";
        for (let d = 0; d < Math.min(2, decks.length); ++d) {
            const deck = decks[d];
            const top = d * deckRows;
            // deck number and bar.beat, left; the countdown, right
            text(String(deck.number), 1, top + 1, Qt.darker(deck.color, 1.6));
            text(deck.barBeat, 8, top + 1, deck.color);
            text(deck.countdown, columns - 1 - textWidth(deck.countdown), top + 1, Style.accent);
            // the running lights, under the numbers
            const cellTop = top + 1 + 7 + 1;
            for (let c = 0; c < 8; ++c) {
                const on = c === deck.beat;
                const fill = on ? (c === 0 ? Style.accent : deck.color) : (c === 0 ? dimFirst : dimCell);
                for (let dx = 0; dx < cellWidth; ++dx)
                    for (let dy = 0; dy < cellRows; ++dy)
                        lit[(firstCell + c * (cellWidth + cellGap) + dx) + "," + (cellTop + dy)] = fill;
            }
        }
        for (let y = 0; y < totalRows; ++y) {
            for (let x = 0; x < columns; ++x) {
                const c = lit[x + "," + y];
                ctx.fillStyle = c !== undefined ? c : unlit;
                ctx.fillRect(x * p + (p - dot) / 2, y * p + (p - dot) / 2, dot, dot);
            }
        }
    }
}
