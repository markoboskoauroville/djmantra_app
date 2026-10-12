import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Both decks' scrolling waveforms side by side, always at the same zoom, so
// they compare like one track (Marko, 12.10.2026: one zoom for both decks)
Item {
    id: root

    Mixxx.ControlProxy { id: zoom1; group: "[Channel1]"; key: "waveform_zoom" }
    Mixxx.ControlProxy { id: zoom2; group: "[Channel2]"; key: "waveform_zoom" }

    // waveform_zoom 1 (close) .. 10 (far); both decks get the same value
    function setZoom(value) {
        const z = Math.min(10, Math.max(1, value));
        zoom1.value = z;
        zoom2.value = z;
    }

    Component.onCompleted: setZoom(zoom1.value > 0 ? zoom1.value : 3)

    Row {
        id: lanes

        anchors.top: parent.top
        anchors.bottom: zoomRow.top
        anchors.bottomMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 2

        VerticalWaveform {
            width: (lanes.width - 2) / 2
            height: lanes.height
            group: "[Channel1]"
        }

        VerticalWaveform {
            width: (lanes.width - 2) / 2
            height: lanes.height
            group: "[Channel2]"
        }
    }

    Row {
        id: zoomRow

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 40
        spacing: 12

        DButton {
            objectName: "zoomOut"
            width: 64
            height: 36
            text: "−"
            onClicked: root.setZoom(Math.max(1, zoom1.value) + 1)
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("ZOOM")
            color: Style.subText
            font.pixelSize: 12
            font.bold: true
        }

        DButton {
            objectName: "zoomIn"
            width: 64
            height: 36
            text: "+"
            onClicked: root.setZoom(Math.max(1, zoom1.value) - 1)
        }
    }
}
