import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Camelot: the playing deck's key > the other deck's key, green (same, +-1,
// A<->B), amber (+-2, +7, diagonal) or red (clash). A tap shows the wheel.
Rectangle {
    id: root

    signal wheelRequested()

    readonly property bool fromDeck2: play2.value > 0 && play1.value <= 0
    readonly property var match: Mixxx.DJMantra.keyMatch(fromDeck2 ? key2.value : key1.value,
            fromDeck2 ? key1.value : key2.value)

    implicitWidth: 118
    implicitHeight: 34
    radius: Style.radius
    color: Style.tile
    border.color: match.color
    border.width: 1.5

    Mixxx.ControlProxy { id: key1; group: "[Channel1]"; key: "key" }
    Mixxx.ControlProxy { id: key2; group: "[Channel2]"; key: "key" }
    Mixxx.ControlProxy { id: play1; group: "[Channel1]"; key: "play" }
    Mixxx.ControlProxy { id: play2; group: "[Channel2]"; key: "play" }

    Column {
        anchors.centerIn: parent

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.match.text
            color: Style.text
            font.pixelSize: 13
            font.bold: true
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.match.why
            color: root.match.color
            font.pixelSize: 10
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.wheelRequested()
    }
}
