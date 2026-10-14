import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A knob with an LED ring, as the OXI E16 (Marko, 12.10.2026): a circle of
// separate dots round the knob, lit from the start up to the value in the
// knob's colour, the others dim. With the cut-only EQs a full ring is 0 dB and
// it empties as the cut deepens. Drag up / down to turn. Marko 14.10.2026: a long press
// (0.5 s, finger still) resets the knob; a knob whose long press does something else
// (GAIN -> FILTER) resets on a double tap instead. Every reset shows a toast.
Item {
    id: root

    property string group
    property string key
    // in the tap map of the phone tests (round 19: the Mixer tab had none)
    objectName: "knob_" + group + "," + key
    property string label
    property color arcColor: Style.accent
    // the knob's rest position (EQs: the top = 0 dB, cut only)
    property real defaultParameter: 0.5
    // the long press has its own action (longPressed): then a double tap resets
    property bool longPressAction: false
    // the toast: "<toastPrefix><label> → <resetText>", e.g. "DECK 1  LOW → 0 dB"
    property string toastPrefix: ""
    property string resetText: "0 dB"

    function reset() {
        control.parameter = root.defaultParameter;
        Toasts.show(root.toastPrefix + root.label + " \u2192 " + root.resetText);
        Mixxx.DJMantra.log("knob reset " + root.group + "," + root.key);
    }
    readonly property int dots: 15
    // Marko 14.10.2026 (the cut-only EQ is like no other mixer's): the ends of the ring
    // labelled in dB, "−∞" (full kill) on the left, "0" (no cut) on the right, and the cut
    // in the caption while there is one ("LOW −12 dB"); the value is the linear gain
    // "cut" (the EQ: 0 dB at the top) or "gain" (GAIN: -12 dB .. 0 at the middle .. +12 dB,
    // Mixxx's pregain; a DJM's TRIM is -inf .. +9 dB), "" none
    property string dbMode: ""
    property bool dbScale: dbMode !== ""
    readonly property real dbValue: control.value > 0.0005 ? 20 * Math.log(control.value) / Math.LN10 : -999
    readonly property string dbText: {
        if (dbValue <= -998)
            return "\u2212\u221E";
        if (Math.abs(dbValue) < 0.5)
            return "0";
        const n = Math.round(Math.abs(dbValue));
        return (dbValue < 0 ? "\u2212" : "+") + n;
    }
    // the caption shows the value while it is away from the rest position
    readonly property bool dbShown: dbMode === "gain" ? Math.abs(dbValue) >= 0.5
                                  : dbMode === "cut" && dbValue < -0.5

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
        width: root.width
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 8
        text: root.dbShown ? root.label + " " + root.dbText + " dB" : root.label
        color: root.dbShown ? (root.dbMode === "gain" ? Style.accent : root.arcColor) : Style.subText
        font.pixelSize: 11
        font.bold: true
    }

    Repeater {
        // the dB at the ring's two ends (below its first and last dot)
        model: root.dbMode === "cut" ? [[-135, "\u2212\u221E"], [135, "0 dB"]]
             : root.dbMode === "gain" ? [[-135, "\u221212"], [135, "+12"]] : []

        delegate: Text {
            required property var modelData
            readonly property real angle: modelData[0] * Math.PI / 180
            readonly property real ringRadius: dial.width / 2 - dial.width * 0.0425

            x: dial.x + dial.width / 2 + ringRadius * Math.sin(angle) - width / 2
            y: dial.y + dial.height / 2 - ringRadius * Math.cos(angle) + 3
            text: modelData[1]
            color: Style.subText
            font.pixelSize: 9
            font.bold: true
        }
    }

    // a long press (0.5 s, finger still): the GAIN knob flips to FILTER (MixerPage)
    signal longPressed()

    Timer {
        id: hold

        interval: 500
        onTriggered: {
            knobArea.flipped = true;
            if (root.longPressAction)
                root.longPressed();
            else
                root.reset();
        }
    }

    MouseArea {
        id: knobArea

        property real startY: 0
        property real startParameter: 0
        property bool flipped: false

        anchors.fill: dial
        preventStealing: true
        onPressed: (mouse) => {
            startY = mouse.y;
            startParameter = control.parameter;
            flipped = false;
            hold.restart();
        }
        onPositionChanged: (mouse) => {
            if (Math.abs(mouse.y - startY) > 8)
                hold.stop();
            if (flipped)
                return;
            const p = startParameter + (startY - mouse.y) / 254; // ~2 px per MIDI step
            control.parameter = Math.max(0, Math.min(1, p));
        }
        onReleased: hold.stop()
        onCanceled: hold.stop()
        onDoubleClicked: {
            hold.stop();
            if (root.longPressAction)
                root.reset();
        }
    }
}
