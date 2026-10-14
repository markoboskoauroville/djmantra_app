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

    // CONCENTRIC (Marko 14.10.2026, SSL / big analog mixers): a second control on the same knob,
    // its own outer ring of dots; the inner knob is the active one. The EQ: inner = the mode's
    // band, outer = the other mode's (EQ <-> NEURAL MIX); a mode switch swaps them. On the
    // screen only the inner knob turns; the outer one from the controller (SHIFT).
    property string outerKey: ""
    property string outerLabel: ""
    property color outerColor: Style.line
    readonly property bool concentric: outerKey !== ""
    // V73 (Marko 14.10.2026: "two concentric pots ... outside again we have two concentric lights
    // which align to each other"): from the centre out the inner cap, the outer knob ring round
    // it, then the inner control's LED ring and, outermost, the outer control's, dot for dot
    readonly property real ledSize: Math.max(2.5, dial.width * 0.055)
    readonly property real outerLedRadius: dial.width / 2 - ledSize / 2
    readonly property real innerLedRadius: outerLedRadius - ledSize * 1.55
    readonly property real outerKnobSize: 2 * (innerLedRadius - ledSize / 2 - dial.width * 0.035)
    readonly property real innerKnobSize: outerKnobSize * 0.6
    readonly property real outerTurn: concentric ? Math.min(1, outerControl.parameter) : 0
    readonly property real outerDb: outerControl.value > 0.0005 ? 20 * Math.log(outerControl.value) / Math.LN10 : -999
    function dbString(db) {
        if (db <= -998)
            return "\u2212\u221E";
        if (Math.abs(db) < 0.5)
            return "0";
        return (db < 0 ? "\u2212" : "+") + Math.round(Math.abs(db));
    }

    function reset() {
        resetRamp.stop();
        resetRamp.target = knobArea.outer ? outerControl : control;
        resetRamp.to = knobArea.outer ? root.defaultParameter : root.defaultParameter * root.span;
        resetRamp.start();
        Toasts.show(root.toastPrefix + (knobArea.outer ? root.outerLabel : root.label) + " \u2192 " + root.resetText);
        Mixxx.DJMantra.log("knob reset " + root.group + "," + root.key);
    }
    readonly property int dots: 15
    // Marko 14.10.2026 (the cut-only EQ is like no other mixer's): the ends of the ring
    // labelled in dB, "−∞" (full kill) on the left, "0" (no cut) on the right, and the cut
    // in the caption while there is one ("LOW −12 dB"); the value is the linear gain
    // "cut" (the EQ: 0 dB at the top) or "gain" (GAIN: -inf .. 0 dB at the top, the lower half
    // of Mixxx's pregain, V60), "" none
    property string dbMode: ""
    // the part of the control's range the knob turns through: GAIN is cut only (Marko
    // 14.10.2026: each song normalized to a 0 dBFS peak, "If I put the gain to the zero, it means
    // it's a maximum volume of the song"), so its full turn is pregain's lower half, -inf .. 0 dB
    readonly property real span: dbMode === "gain" ? 0.5 : 1
    // the knob's own position, 0 .. 1
    readonly property real turn: Math.min(1, control.parameter / span)
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
    readonly property bool dbShown: dbMode !== "" && dbValue < -0.5

    implicitWidth: 52
    implicitHeight: 64

    Mixxx.ControlProxy {
        id: control

        group: root.group
        key: root.key
    }

    Mixxx.ControlProxy {
        id: outerControl

        group: root.group
        key: root.concentric ? root.outerKey : root.key
    }

    // Marko 14.10.2026: "always ramping is the rule ... no snapping anywhere": a reset glides
    // there, easing in and out
    NumberAnimation {
        id: resetRamp

        target: control
        property: "parameter"
        duration: 450
        easing.type: Easing.InOutCubic
    }

    Item {
        id: dial

        // the ring and the knob inside it
        width: Math.min(root.width, root.height - caption.height - (root.concentric ? topCaption.height + 2 : 0) - 1)
        height: width
        anchors.top: root.concentric ? topCaption.bottom : parent.top
        anchors.topMargin: root.concentric ? 1 : 0
        anchors.horizontalCenter: parent.horizontalCenter

        Repeater {
            model: root.dots

            delegate: Rectangle {
                required property int index

                // -135° .. +135°, 0° straight up
                readonly property real angle: (-135 + 270 * index / (root.dots - 1)) * Math.PI / 180
                readonly property real ringRadius: root.concentric ? root.innerLedRadius : dial.width / 2 - size / 2
                readonly property real size: root.concentric ? root.ledSize : Math.max(3, dial.width * 0.085)
                readonly property bool lit: root.turn > 0.001 && (index + 0.5) / root.dots <= root.turn + 0.5 / root.dots

                z: 2 // over the concentric outer knob
                width: size
                height: size
                radius: size / 2
                x: dial.width / 2 + ringRadius * Math.sin(angle) - size / 2
                y: dial.height / 2 - ringRadius * Math.cos(angle) - size / 2
                color: lit ? root.arcColor : Style.tile
                opacity: lit ? 1 : 0.9
            }
        }

        Repeater {
            // the outer ring: the concentric second control
            model: root.concentric ? root.dots : 0

            delegate: Rectangle {
                required property int index

                readonly property real angle: (-135 + 270 * index / (root.dots - 1)) * Math.PI / 180
                readonly property real size: root.ledSize
                readonly property real ringRadius: root.outerLedRadius
                readonly property bool lit: root.outerTurn > 0.001 && (index + 0.5) / root.dots <= root.outerTurn + 0.5 / root.dots

                width: size
                height: size
                radius: size / 2
                x: dial.width / 2 + ringRadius * Math.sin(angle) - size / 2
                y: dial.height / 2 - ringRadius * Math.cos(angle) - size / 2
                color: lit ? root.outerColor : Style.tile
                opacity: lit ? 1 : 0.6
            }
        }

        Rectangle {
            // the outer knob (concentric, as the reference: a larger dark ring round the inner
            // cap) with its own pointer
            id: outerKnob

            visible: root.concentric
            anchors.centerIn: parent
            width: root.outerKnobSize
            height: width
            radius: width / 2
            color: "#070a17"
            border.color: Style.line
            border.width: 1

            Rectangle {
                // the outer knob's pointer, on its ring: -135° (0) .. +135° (1)
                width: 2.5
                height: (root.outerKnobSize - root.innerKnobSize) / 2 - 3
                radius: 1.25
                color: root.outerColor
                x: outerKnob.width / 2 - width / 2
                y: 2
                transform: Rotation {
                    origin.x: 1.25
                    origin.y: outerKnob.height / 2 - 2
                    angle: -135 + 270 * root.outerTurn
                }
            }
        }

        Rectangle {
            id: body

            z: 3
            anchors.centerIn: parent
            width: root.concentric ? root.innerKnobSize : dial.width * 0.62
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
                rotation: -135 + 270 * root.turn
            }
        }
    }

    Text {
        // concentric: the outer ring's value on top (Marko 14.10.2026: "at the top ... the number
        // of outside ring, at the bottom number of inside ring")
        id: topCaption

        visible: root.concentric
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width: root.width
        height: root.concentric ? 11 : 0
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 7
        text: root.outerLabel + " " + root.dbString(root.outerDb)
        color: root.outerColor
        font.pixelSize: 9
        font.bold: true
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
        text: root.concentric ? root.label + " " + root.dbText
              : root.dbShown ? root.label + " " + root.dbText + " dB" : root.label
        color: root.concentric || root.dbShown ? (root.dbMode === "gain" ? Style.accent : root.arcColor) : Style.subText
        font.pixelSize: root.concentric ? 9 : 11
        font.bold: true
    }

    Repeater {
        // the dB at the ring's two ends (below its first and last dot); not on the concentric
        // knobs (V73: they ran into the captions; the values are above and below)
        model: root.dbMode !== "" && !root.concentric ? [[-135, "\u2212\u221E"], [135, "0 dB"]] : []

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

        interval: 1000 // Marko 14.10.2026: every long press is 1 s
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
        // the touch began on the outer ring (concentric): it turns the outer control
        property bool outer: false

        anchors.fill: dial
        preventStealing: true
        onPressed: (mouse) => {
            resetRamp.stop();
            startY = mouse.y;
            // fingers turn only the inner knob (Marko 14.10.2026); the outer ring is turned from
            // the controller (SHIFT + the knob)
            outer = false;
            startParameter = outer ? root.outerTurn : root.turn;
            flipped = false;
            hold.restart();
        }
        onPositionChanged: (mouse) => {
            if (Math.abs(mouse.y - startY) > 8)
                hold.stop();
            if (flipped)
                return;
            const p = startParameter + (startY - mouse.y) / 254; // ~2 px per MIDI step
            if (outer)
                outerControl.parameter = Math.max(0, Math.min(1, p));
            else
                control.parameter = Math.max(0, Math.min(1, p)) * root.span;
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
