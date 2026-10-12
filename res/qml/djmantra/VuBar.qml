import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A VU meter: green, orange near the top, red when it clips
Rectangle {
    id: root

    property string group

    color: Style.surface
    radius: 2

    Mixxx.ControlProxy {
        id: level

        group: root.group
        key: "vu_meter"
        maxRate: 30
    }

    Mixxx.ControlProxy {
        id: peak

        group: root.group
        key: "peak_indicator"
        maxRate: 30
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height * Math.max(0, Math.min(1, level.value))
        radius: 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: peak.value > 0 ? Style.fire : Style.air }
            GradientStop { position: 0.35; color: Style.earth }
            GradientStop { position: 1.0; color: "#3f8a45" }
        }
    }
}
