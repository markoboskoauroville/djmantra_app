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
        maxRate: 20
    }

    Mixxx.ControlProxy {
        id: peak

        group: root.group
        key: "peak_indicator"
        maxRate: 20
    }

    // Round 26 (Marko's screenshot: a black spike in the meter): a rounded
    // gradient rectangle of changing height drew broken on the phone's GPU.
    // Now the gradient bar keeps its full height, and a clip shows its
    // bottom part, as high as the level
    Item {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.round(parent.height * Math.max(0, Math.min(1, level.value)))
        visible: height > 0
        clip: true

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: root.height
            gradient: Gradient {
                GradientStop { position: 0.0; color: peak.value > 0 ? Style.fire : Style.air }
                GradientStop { position: 0.35; color: Style.earth }
                GradientStop { position: 1.0; color: "#3f8a45" }
            }
        }
    }
}
