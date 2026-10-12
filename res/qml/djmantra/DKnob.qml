import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A knob: drag up / down to turn, double tap for the middle (centre detent)
Item {
    id: root

    property string group
    property string key
    property string label
    property color arcColor: Style.orange

    implicitWidth: 52
    implicitHeight: 64

    Mixxx.ControlProxy {
        id: control

        group: root.group
        key: root.key
    }

    Rectangle {
        id: body

        width: Math.min(root.width, root.height - caption.height)
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        radius: width / 2
        color: Style.tile
        border.color: root.arcColor
        border.width: 2

        Rectangle {
            // the pointer: -135° (0) .. +135° (1)
            width: 3
            height: body.height / 2 - 4
            radius: 1.5
            color: Style.text
            x: body.width / 2 - width / 2
            y: 4
            transformOrigin: Item.Bottom
            rotation: -135 + 270 * control.parameter
        }
    }

    Text {
        id: caption

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        color: Style.subText
        font.pixelSize: 11
        font.bold: true
    }

    MouseArea {
        property real startY: 0
        property real startParameter: 0

        anchors.fill: body
        preventStealing: true
        onPressed: (mouse) => {
            startY = mouse.y;
            startParameter = control.parameter;
        }
        onPositionChanged: (mouse) => {
            const p = startParameter + (startY - mouse.y) / 150;
            control.parameter = Math.max(0, Math.min(1, p));
        }
        onDoubleClicked: control.parameter = 0.5
    }
}
