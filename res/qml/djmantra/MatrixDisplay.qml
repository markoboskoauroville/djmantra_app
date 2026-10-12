import "."
import QtQuick 2.12

// The dot-matrix display, as on the OXI E16 (Marko, 12.10.2026): dark glass,
// a grid of square pixels, a 5x7 pixel font, lit dots in the accent colours,
// the unlit ones faintly visible. The ONLY phrase lights of the interface.
// Per deck, deck 1 over deck 2 so both compare at a glance:
//   time left in the song | bar.beat (deck colour) | countdown to the next phrase /
//   section change (yellow)
//   the running lights: one LED cell per beat of the phrase (8 / 16 / 32),
//   bars set apart, beat 1 yellow. A tap on a cell launches that beat
//   (cellTapped); an armed cell blinks until its start.
Canvas {
    id: root

    // [{remaining, barBeat, countdown, beat, cells, armed, color}] per deck
    property var decks: []
    property bool blink: false
    readonly property int cellRows: 4
    readonly property int deckRows: 1 + 7 + 1 + cellRows // gap, numbers, gap, cells
    readonly property int totalRows: 2 * deckRows + 1
    readonly property real pitch: height / totalRows
    readonly property int columns: Math.floor(width / pitch)
    readonly property color glass: "#04060e"
    readonly property color unlit: "#121735"
    signal cellTapped(int deck, int cell)

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

    // where the cells are: the first column of each and their width; a
    // 1-dot gap between beats, 3 between bars (4 beats)
    function cellColumns(n) {
        const bars = Math.ceil(n / 4);
        const gaps = (n - bars) * 1 + (bars - 1) * 3;
        const width = Math.max(1, Math.floor((columns - 2 - gaps) / n));
        const used = n * width + gaps;
        let x = Math.floor((columns - used) / 2);
        const starts = [];
        for (let c = 0; c < n; ++c) {
            starts.push(x);
            x += width + ((c + 1) % 4 === 0 ? 3 : 1);
        }
        return {starts: starts, width: width};
    }

    function anyArmed() {
        for (const deck of decks) {
            if (deck.armed >= 0)
                return true;
        }
        return false;
    }

    Timer {
        // an armed cell blinks
        interval: 250
        repeat: true
        running: root.anyArmed()
        onTriggered: {
            root.blink = !root.blink;
            root.requestPaint();
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            const d = Math.min(1, Math.floor(mouse.y / (root.deckRows * root.pitch)));
            if (d >= root.decks.length)
                return;
            const layout = root.cellColumns(root.decks[d].cells);
            const x = mouse.x / root.pitch;
            // the nearest cell, so the gaps tap too
            let best = 0;
            for (let c = 1; c < layout.starts.length; ++c) {
                if (Math.abs(x - (layout.starts[c] + layout.width / 2)) <
                        Math.abs(x - (layout.starts[best] + layout.width / 2)))
                    best = c;
            }
            root.cellTapped(d, best);
        }
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
        const dimCell = "#252d5c";
        const dimFirst = "#4a4326";
        for (let d = 0; d < Math.min(2, decks.length); ++d) {
            const deck = decks[d];
            const top = d * deckRows;
            text(deck.remaining, 1, top + 1, String(Style.text));
            text(deck.barBeat, Math.floor((columns - textWidth(deck.barBeat)) / 2), top + 1, deck.color);
            text(deck.countdown, columns - 1 - textWidth(deck.countdown), top + 1, String(Style.accent));
            const layout = cellColumns(deck.cells);
            const cellTop = top + 1 + 7 + 1;
            for (let c = 0; c < deck.cells; ++c) {
                const armed = c === deck.armed;
                const on = c === deck.beat || (armed && blink);
                const fill = on ? (c === 0 || armed ? String(Style.accent) : deck.color)
                        : (c === 0 ? dimFirst : dimCell);
                for (let dx = 0; dx < layout.width; ++dx)
                    for (let dy = 0; dy < cellRows; ++dy)
                        lit[(layout.starts[c] + dx) + "," + (cellTop + dy)] = fill;
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
