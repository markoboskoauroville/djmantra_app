import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A knob with an LED ring, as the OXI E16 (Marko, 12.10.2026): a circle of
// separate dots round the knob, lit from the start up to the value in the
// knob's colour, the others dim. With the cut-only EQs a full ring is 0 dB and
// it empties as the cut deepens. Drag up / down to turn; a double tap: its
// rest position.
Item {
    id: root

    property string group
    property string key
    // in the tap map of the phone tests (round 19: the Mixer tab had none)
    objectName: "knob_" + group + "," + key
    property string label
    property color arcColor: Style.accent
    // a double tap: the knob's rest position (EQs: the top = 0 dB, cut only)
    property real defaultParameter: 0.5
    readonly property int dots: 15

    implicitWidth: 52
    implicitHeight: 64

    Mixxx.ControlProxy {
        id: control

        group: root.group
        key: root.key
    }

    Item {
        id: dial

        // the ring and the knob inside it
        width: Math.min(root.width, root.height - caption.height - 1)
        height: width
        anchors.horizontalCenter: parent.horizontalCenter

        Repeater {
            model: root.dots

            delegate: Rectangle {
                required property int index

                // -135° .. +135°, 0° straight up
                readonly property real angle: (-135 + 270 * index / (root.dots - 1)) * Math.PI / 180
                readonly property real ringRadius: dial.width / 2 - size / 2
                readonly property real size: Math.max(3, dial.width * 0.085)
                readonly property bool lit: control.parameter > 0.001 && (index + 0.5) / root.dots <= control.parameter + 0.5 / root.dots

                width: size
                height: size
                radius: size / 2
                x: dial.width / 2 + ringRadius * Math.sin(angle) - size / 2
                y: dial.height / 2 - ringRadius * Math.cos(angle) - size / 2
                color: lit ? root.arcColor : Style.tile
                opacity: lit ? 1 : 0.9
            }
        }

        Rectangle {
            id: body

            anchors.centerIn: parent
            width: dial.width * 0.62
            height: width
            radius: width / 2
            color: Style.tile
            border.color: Style.line
            border.width: 1

            Rectangle {
                // the pointer: -135° (0) .. +135° (1)
                width: 2.5
                height: body.height / 2 - 3
                radius: 1.25
                color: Style.text
                x: body.width / 2 - width / 2
                y: 3
                transformOrigin: Item.Bottom
                rotation: -135 + 270 * control.parameter
            }
        }
    }

    Text {
        id: caption

        anchors.top: dial.bottom
        anchors.topMargin: 1
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        color: Style.subText
        font.pixelSize: 11
        font.bold: true
    }

    MouseArea {
        property real startY: 0
        property real startParameter: 0

        anchors.fill: dial
        preventStealing: true
        onPressed: (mouse) => {
            startY = mouse.y;
            startParameter = control.parameter;
        }
        onPositionChanged: (mouse) => {
            const p = startParameter + (startY - mouse.y) / 254; // ~2 px per MIDI step
            control.parameter = Math.max(0, Math.min(1, p));
        }
        onDoubleClicked: control.parameter = root.defaultParameter
    }
}
