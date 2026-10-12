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
                const g = PixelFont.glyph(string[i]);
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
