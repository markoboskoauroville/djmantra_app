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
        id: wheel

        anchors.top: bar.bottom
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(parent.width - 24, parent.height - bar.height - 220)
        height: width

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            const c = width / 2;
            const any = root.code1 !== "" || root.code2 !== "";
            for (const major of [true, false]) {
                const r1 = major ? c - 2 : c * 0.66;
                const r2 = major ? c * 0.66 : c * 0.34;
                for (let n = 1; n <= 12; ++n) {
                    const code = n + (major ? "B" : "A");
                    const mid = -Math.PI / 2 + (n % 12) * Math.PI / 6;
                    const lit = !any || root.matches(code);
                    const hue = (((n - 1) * 30 + 150) % 360) / 360;
                    ctx.fillStyle = Qt.hsva(hue, major ? 0.6 : 0.8, lit ? (major ? 0.92 : 0.8) : 0.25, 1);
                    ctx.beginPath();
                    ctx.arc(c, c, r1, mid - Math.PI / 12, mid + Math.PI / 12, false);
                    ctx.arc(c, c, r2, mid + Math.PI / 12, mid - Math.PI / 12, true);
                    ctx.closePath();
                    ctx.fill();
                    ctx.strokeStyle = "#111215";
                    ctx.lineWidth = 2;
                    ctx.stroke();
                    if (code === root.code1 || code === root.code2) {
                        ctx.strokeStyle = code === root.code1 ? "#3ad13a" : "#e04ad0";
                        ctx.lineWidth = 4;
                        ctx.stroke();
                    }
                    const lr = (r1 + r2) / 2;
                    ctx.fillStyle = lit ? "#111215" : "#8a8d90";
                    ctx.font = "bold " + Math.round((r1 - r2) * 0.32) + "px sans-serif";
                    ctx.textAlign = "center";
                    ctx.textBaseline = "middle";
                    ctx.fillText(code, c + lr * Math.cos(mid), c + lr * Math.sin(mid));
                }
            }
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
