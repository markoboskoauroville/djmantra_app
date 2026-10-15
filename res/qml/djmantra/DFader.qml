import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A fader: vertical (volume) or horizontal (crossfader); the handle follows
// the finger from where it is touched; a long press (0.5 s, finger still) resets it with
// a toast (Marko 14.10.2026: double taps are not practical)
Item {
    id: root

    property string group
    property string key
    // in the tap map of the phone tests (round 19: the Mixer tab had none)
    objectName: "fader_" + group + "," + key
    property bool vertical: true
    property color barColor: Style.accent
    property string label: "CROSSFADER"
    property real defaultParameter: vertical ? 1 : 0.5
    property string resetText: qsTr("middle")
    // the controller's ghost (Ghost.qml, V77): a half-transparent handle where the Mix Ultra's
    // knob is while it has not picked up this value
    property string ghostName: ""
    property Ghost ghost: Ghost { name: root.ghostName }

    // Marko 14.10.2026: "always ramping is the rule ... no snapping anywhere": a reset glides
    // there, easing in and out
    NumberAnimation {
        id: resetRamp

        target: control
        property: "parameter"
        duration: 450
        easing.type: Easing.InOutCubic
    }

    Timer {
        id: hold

        interval: 1000 // every long press is 1 s
        onTriggered: {
            area.held = true;
            resetRamp.stop();
            resetRamp.to = root.defaultParameter;
            resetRamp.start();
            Toasts.show(root.label + " \u2192 " + root.resetText);
            Mixxx.DJMantra.log("fader reset " + root.group + "," + root.key);
        }
    }

    Mixxx.ControlProxy {
        id: control

        group: root.group
        key: root.key
    }

    Rectangle {
        // the track
        anchors.centerIn: parent
        width: root.vertical ? 4 : root.width - 8
        height: root.vertical ? root.height - 8 : 4
        radius: 2
        color: Style.line
    }

    Rectangle {
        // the GHOST handle: the controller's knob, not picked up yet
        visible: root.ghost.position >= 0
        width: handle.width
        height: handle.height
        radius: 5
        color: "transparent"
        border.color: root.barColor
        border.width: 2
        opacity: 0.75
        x: root.vertical ? handle.x : Math.max(0, root.ghost.position) * (root.width - width)
        y: root.vertical ? (1 - Math.max(0, root.ghost.position)) * (root.height - height) : handle.y

        Rectangle {
            anchors.fill: parent
            anchors.margins: 3
            radius: 3
            color: root.barColor
            opacity: 0.5
        }
    }

    Rectangle {
        id: handle

        width: root.vertical ? Math.min(root.width, 44) : 26
        height: root.vertical ? 26 : Math.min(root.height, 44)
        radius: 5
        color: Style.text
        border.color: root.barColor
        border.width: 2
        x: root.vertical ? (root.width - width) / 2 : control.parameter * (root.width - width)
        y: root.vertical ? (1 - control.parameter) * (root.height - height) : (root.height - height) / 2
    }

    MouseArea {
        id: area

        property real startPos: 0
        property real startParameter: 0
        property bool held: false

        anchors.fill: parent
        preventStealing: true
        onPressed: (mouse) => {
            resetRamp.stop();
            startPos = root.vertical ? mouse.y : mouse.x;
            startParameter = control.parameter;
            held = false;
            hold.restart();
        }
        onReleased: hold.stop()
        onCanceled: hold.stop()
        onPositionChanged: (mouse) => {
            if (Math.abs((root.vertical ? mouse.y : mouse.x) - startPos) > 8)
                hold.stop();
            if (held)
                return;
            const span = root.vertical ? root.height - handle.height : root.width - handle.width;
            const delta = root.vertical ? startPos - mouse.y : mouse.x - startPos;
            control.parameter = Math.max(0, Math.min(1, startParameter + delta / span));
        }
    }
}
