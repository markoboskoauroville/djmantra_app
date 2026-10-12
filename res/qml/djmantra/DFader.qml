import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A fader: vertical (volume) or horizontal (crossfader); the handle follows
// the finger from where it is touched
Item {
    id: root

    property string group
    property string key
    property bool vertical: true
    property color barColor: Style.accent

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
        color: "#3a3c41"
    }

    Rectangle {
        id: handle

        width: root.vertical ? Math.min(root.width, 44) : 26
        height: root.vertical ? 26 : Math.min(root.height, 44)
        radius: 5
        color: "#b8bbbf"
        border.color: root.barColor
        border.width: 2
        x: root.vertical ? (root.width - width) / 2 : control.parameter * (root.width - width)
        y: root.vertical ? (1 - control.parameter) * (root.height - height) : (root.height - height) / 2
    }

    MouseArea {
        property real startPos: 0
        property real startParameter: 0

        anchors.fill: parent
        preventStealing: true
        onPressed: (mouse) => {
            startPos = root.vertical ? mouse.y : mouse.x;
            startParameter = control.parameter;
        }
        onPositionChanged: (mouse) => {
            const span = root.vertical ? root.height - handle.height : root.width - handle.width;
            const delta = root.vertical ? startPos - mouse.y : mouse.x - startPos;
            control.parameter = Math.max(0, Math.min(1, startParameter + delta / span));
        }
        onDoubleClicked: control.parameter = root.vertical ? 1 : 0.5
    }
}
