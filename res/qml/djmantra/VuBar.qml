import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A VU meter: green, orange near the top, red when it clips
Rectangle {
    id: root

    property string group

    color: "#18191b"
    radius: 2

    Mixxx.ControlProxy {
        id: level

        group: root.group
        key: "vu_meter"
    }

    Mixxx.ControlProxy {
        id: peak

        group: root.group
        key: "peak_indicator"
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height * Math.max(0, Math.min(1, level.value))
        radius: 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: peak.value > 0 ? "#ff304a" : "#ff8a00" }
            GradientStop { position: 0.35; color: "#3ad13a" }
            GradientStop { position: 1.0; color: "#1f8f1f" }
        }
    }
}
