import "."
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

                    DButton { width: 56; height: 36; text: "−"; group: lane.modelData; key: "waveform_zoom_up"; mode: "push" }
                    DButton { width: 56; height: 36; text: "+"; group: lane.modelData; key: "waveform_zoom_down"; mode: "push" }
                }
            }
        }
    }
}
