import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A button. With group and key it drives a control: "toggle" flips it on
// each tap, "push" holds it at 1 while pressed (CUE, hot cues)
Rectangle {
    id: root

    property string text
    property string group
    property string key
    property string mode: "toggle" // or "push", or "" (only clicked())
    property color litColor: Style.accent
    property bool lit: root.key !== "" && control.value > 0
    property int fontSize: 15
    // "play" / "pause" / "next" (to the next section): drawn, not a font
    // glyph (round 13: the pause sign was tofu; round 16b: JUMP's too)
    property string icon: ""
    signal clicked()
    signal pressedDown()

    radius: Style.radius
    color: area.pressed ? Style.pressed : (lit ? litColor : Style.tile)
    border.color: lit ? litColor : Style.surface
    border.width: 1

    Mixxx.ControlProxy {
        id: control

        group: root.group !== "" ? root.group : "[Master]"
        key: root.key !== "" ? root.key : "enabled"
    }

    Row {
        anchors.centerIn: parent
        visible: root.icon === "pause"
        spacing: root.height * 0.14

        Repeater {
            model: 2

            delegate: Rectangle {
                width: root.height * 0.13
                height: root.height * 0.42
                radius: 2
                color: root.lit ? Style.inkOnAccent : Style.text
            }
        }
    }

    Canvas {
        // "next": a triangle against a bar, ▸|
        id: nextIcon

        anchors.centerIn: parent
        width: root.height * 0.42
        height: width
        visible: root.icon === "next"
        onVisibleChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            ctx.fillStyle = String(root.lit ? Style.inkOnAccent : Style.text);
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.lineTo(width * 0.72, height / 2);
            ctx.lineTo(0, height);
            ctx.closePath();
            ctx.fill();
            ctx.fillRect(width * 0.78, 0, width * 0.22, height);
        }
    }

    Text {
        anchors.fill: parent
        anchors.margins: 4
        visible: root.icon !== "pause" && root.icon !== "next"
        text: root.icon === "play" ? "\u25B6" : root.text
        color: root.lit ? Style.inkOnAccent : Style.text
        font.pixelSize: root.fontSize
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.Fit
        minimumPixelSize: 8
    }

    MouseArea {
        id: area

        anchors.fill: parent
        onPressed: {
            root.pressedDown();
            if (root.key !== "" && root.mode === "push") {
                control.value = 1;
                Mixxx.DJMantra.log("button " + root.group + "," + root.key + " pressed");
            }
            // toggles act on the press (round 17: Marko's PLAY "doesn't work":
            // a finger that slides a little lost the click); the log names the
            // new value (round 19: the proxy's value lagged behind)
            if (root.key !== "" && root.mode === "toggle") {
                const next = control.value > 0 ? 0 : 1;
                control.value = next;
                Mixxx.DJMantra.log("button " + root.group + "," + root.key + " " + next);
            }
        }
        onReleased: {
            if (root.key !== "" && root.mode === "push")
                control.value = 0;
        }
        onCanceled: {
            if (root.key !== "" && root.mode === "push")
                control.value = 0;
        }
        onClicked: root.clicked()
    }
}
