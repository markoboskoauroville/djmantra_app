import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The Camelot wheel: B (major) outside, A (minor) inside, 12 on top. The
// decks' keys are ringed (deck 1 green, deck 2 magenta); the keys that mix
// with them are lit, the others dimmed.
Rectangle {
    id: root

    signal done()

    color: Style.background

    Mixxx.ControlProxy { id: key1; group: "[Channel1]"; key: "key" }
    Mixxx.ControlProxy { id: key2; group: "[Channel2]"; key: "key" }

    readonly property string code1: Mixxx.DJMantra.camelot(key1.value)
    readonly property string code2: Mixxx.DJMantra.camelot(key2.value)

    function matches(code) {
        for (const deck of [code1, code2]) {
            if (deck === "")
                continue;
            const n = parseInt(deck);
            const m = parseInt(code);
            const sameRing = deck.slice(-1) === code.slice(-1);
            const delta = (m - n + 12) % 12;
            if ((sameRing && (delta === 0 || delta === 1 || delta === 11)) || (!sameRing && delta === 0))
                return true;
        }
        return false;
    }

    Rectangle {
        id: bar

        width: parent.width
        height: 52
        color: Style.surface

        DButton {
            id: backButton

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 52
            height: 40
            text: "←"
            fontSize: 22
            onClicked: root.done()
        }

        Text {
            anchors.left: backButton.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Camelot wheel")
            color: Style.text
            font.pixelSize: 20
            font.bold: true
        }
    }

    Canvas {
        // Drawn like the chart wheel of Marko's horoscope app (astro_dos,
        // modern/Wheel.kt): a true circle on a dark disc, each key a thinly
        // tinted ring sector between two radii, thin boundary lines, the code
        // centred in its band in the key's colour, the bold outer circle
        id: wheel

        anchors.top: bar.bottom
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 24, parent.height - bar.height - 220)
        height: width

        readonly property color wheelBg: "#141821"
        readonly property color line: "#5d6584"
        readonly property color ink: "#eef0ff"
        readonly property color sub: "#9aa3c7"

        function keyColor(n, alpha) {
            // 12 hues round the wheel, the same for A and B of one number
            return Qt.hsla((((n - 1) * 30 + 150) % 360) / 360, 0.72, 0.62, alpha);
        }

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const cx = width / 2;
            const cy = height / 2;
            const r = width / 2 - 3;
            const rB = r * 0.70;   // B (major) between rB and r
            const rA = r * 0.42;   // A (minor) between rA and rB
            const thin = Math.max(1, r * 0.006);
            const bold = Math.max(2, r * 0.012);
            const any = root.code1 !== "" || root.code2 !== "";

            ctx.fillStyle = wheelBg;
            ctx.beginPath();
            ctx.arc(cx, cy, r, 0, 2 * Math.PI, false);
            ctx.fill();

            const sector = (r1, r2, start, sweep) => {
                ctx.beginPath();
                ctx.arc(cx, cy, r2, start, start + sweep, false);
                ctx.arc(cx, cy, r1, start + sweep, start, true);
                ctx.closePath();
            };

            for (const major of [true, false]) {
                const r1 = major ? rB : rA;
                const r2 = major ? r : rB;
                for (let n = 1; n <= 12; ++n) {
                    const code = n + (major ? "B" : "A");
                    // 12 on top, clockwise; the sector spans 30 degrees round its middle
                    const mid = -Math.PI / 2 + (n % 12) * Math.PI / 6;
                    const isDeck = code === root.code1 || code === root.code2;
                    const lit = any && root.matches(code);
                    const alpha = isDeck ? 0.85 : lit ? 0.45 : any ? 0.08 : 0.22;
                    sector(r1, r2, mid - Math.PI / 12, Math.PI / 6);
                    ctx.fillStyle = keyColor(n, alpha);
                    ctx.fill();
                    if (isDeck) {
                        ctx.strokeStyle = code === root.code1 ? "#3ad13a" : "#e04ad0";
                        ctx.lineWidth = bold * 1.4;
                        ctx.stroke();
                    }
                    // the code and its musical name, centred in the band
                    const lr = (r1 + r2) / 2;
                    const x = cx + lr * Math.cos(mid);
                    const y = cy + lr * Math.sin(mid);
                    const size = (r2 - r1) * 0.34;
                    ctx.textAlign = "center";
                    ctx.textBaseline = "middle";
                    ctx.font = "bold " + Math.round(size) + "px sans-serif";
                    ctx.fillStyle = isDeck ? "#111215" : (any && !lit ? sub : keyColor(n, 1));
                    ctx.fillText(code, x, y - size * 0.35);
                    ctx.font = Math.round(size * 0.62) + "px sans-serif";
                    ctx.fillStyle = isDeck ? "#111215" : sub;
                    ctx.fillText(Mixxx.DJMantra.musicalName(code), x, y + size * 0.55);
                }
            }
            // thin boundary lines across both rings
            ctx.strokeStyle = line;
            ctx.lineWidth = thin;
            for (let k = 0; k < 12; ++k) {
                const a = -Math.PI / 2 + k * Math.PI / 6 + Math.PI / 12;
                ctx.beginPath();
                ctx.moveTo(cx + rA * Math.cos(a), cy + rA * Math.sin(a));
                ctx.lineTo(cx + r * Math.cos(a), cy + r * Math.sin(a));
                ctx.stroke();
            }
            // the rings: bold outside, thin between and inside
            for (const [radius, width] of [[r, bold], [rB, thin * 1.4], [rA, thin]]) {
                ctx.beginPath();
                ctx.arc(cx, cy, radius, 0, 2 * Math.PI, false);
                ctx.strokeStyle = radius === r ? ink : line;
                ctx.lineWidth = width;
                ctx.stroke();
            }
            // in the middle: the decks' keys and how they match
            ctx.textAlign = "center";
            ctx.textBaseline = "middle";
            const fs = rA * 0.2;
            ctx.font = "bold " + Math.round(fs) + "px sans-serif";
            ctx.fillStyle = "#3ad13a";
            ctx.fillText("1  " + (root.code1 !== "" ? root.code1 : "-"), cx, cy - fs * 1.2);
            ctx.fillStyle = "#e04ad0";
            ctx.fillText("2  " + (root.code2 !== "" ? root.code2 : "-"), cx, cy);
            const m = Mixxx.DJMantra.keyMatch(key1.value, key2.value);
            ctx.font = Math.round(fs * 0.75) + "px sans-serif";
            ctx.fillStyle = m.color;
            ctx.fillText(m.why, cx, cy + fs * 1.2);
        }

        Connections {
            target: root
            function onCode1Changed() { wheel.requestPaint(); }
            function onCode2Changed() { wheel.requestPaint(); }
        }
    }

    Column {
        anchors.top: wheel.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 8

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#3ad13a"
            font.pixelSize: 14
            text: qsTr("Same key, ±1, A ↔ B with the same number: compatible. +1 lifts the energy, -1 calms it.")
        }
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#ffb300"
            font.pixelSize: 14
            text: qsTr("+2, +7, diagonal (8A > 9B): risky, an energy boost on a short blend.")
        }
        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#e53935"
            font.pixelSize: 14
            text: qsTr("Anything else clashes: blend on the drums only.")
        }
    }
}
