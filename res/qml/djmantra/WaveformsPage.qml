import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Both decks' scrolling waveforms side by side, with zoom
Item {
    id: root

    Row {
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: ["[Channel1]", "[Channel2]"]

            delegate: Item {
                id: lane

                required property string modelData

                width: (root.width - 2) / 2
                height: root.height

                VerticalWaveform {
                    anchors.top: parent.top
                    anchors.bottom: zoom.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    group: lane.modelData
                }

                Row {
                    id: zoom

                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: 40
                    spacing: 8

                    // waveform_zoom 1 (close) .. 10 (far); the _up/_down buttons are
                    // handled by the classic interface only (round 13: no effect)
                    DButton {
                        width: 56
                        height: 36
                        text: "−"
                        onClicked: zoomLevel.value = Math.min(10, Math.max(1, zoomLevel.value) + 1)
                    }
                    DButton {
                        width: 56
                        height: 36
                        text: "+"
                        onClicked: zoomLevel.value = Math.max(1, Math.max(1, zoomLevel.value) - 1)
                    }

                    Mixxx.ControlProxy {
                        id: zoomLevel

                        group: lane.modelData
                        key: "waveform_zoom"
                    }
                }
            }
        }
    }
}
