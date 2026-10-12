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
    property color onColor: Style.accent
    property bool lit: root.key !== "" && control.value > 0
    property int fontSize: 15
    // "play" / "pause": drawn, not a font glyph (round 13: the pause sign was tofu)
    property string icon: ""
    signal clicked()

    radius: Style.radius
    color: area.pressed ? Style.pressed : (lit ? onColor : Style.tile)
    border.color: lit ? onColor : "#3a3c41"
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
                color: root.lit ? "#111215" : Style.text
            }
        }
    }

    Text {
        anchors.fill: parent
        anchors.margins: 4
        visible: root.icon !== "pause"
        text: root.icon === "play" ? "\u25B6" : root.text
        color: root.lit ? "#111215" : Style.text
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
            if (root.key !== "" && root.mode === "push")
                control.value = 1;
        }
        onReleased: {
            if (root.key !== "" && root.mode === "push")
                control.value = 0;
        }
        onCanceled: {
            if (root.key !== "" && root.mode === "push")
                control.value = 0;
        }
        onClicked: {
            if (root.key !== "" && root.mode === "toggle")
                control.value = control.value > 0 ? 0 : 1;
            root.clicked();
        }
    }
}
