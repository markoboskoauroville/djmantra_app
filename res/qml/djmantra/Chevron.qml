import QtQuick 2.12

// a small arrow drawn, not a font glyph: ▾ / ▴ were empty boxes in the phone's font (r41, V58)
Canvas {
    id: root

    property bool up: false
    property color color: "white"

    width: 12
    height: 8
    onUpChanged: requestPaint()
    onColorChanged: requestPaint()
    onPaint: {
        const g = getContext("2d");
        g.reset();
        g.fillStyle = root.color;
        g.beginPath();
        if (root.up) {
            g.moveTo(0, height);
            g.lineTo(width, height);
            g.lineTo(width / 2, 0);
        } else {
            g.moveTo(0, 0);
            g.lineTo(width, 0);
            g.lineTo(width / 2, height);
        }
        g.closePath();
        g.fill();
    }
}
