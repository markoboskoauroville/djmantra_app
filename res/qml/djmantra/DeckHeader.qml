import "."
import ".." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's header: cover, the 8-beat phrase LEDs, artist and key, title, the
// counter. No overview: the interface has no waveforms (Marko, 12.10.2026).
// Deck 1 reads left to right, deck 2 mirrored (cover at the outer edge).
Item {
    id: root

    property string group
    property bool mirrored: false
    property color deckColor: Style.deck1
    property var player: Mixxx.PlayerManager.getPlayer(group)
    signal coverTapped()

    implicitHeight: 74

    Mixxx.ControlProxy { id: keyControl; group: root.group; key: "key" }

    Rectangle {
        id: cover

        width: 52
        height: 52
        radius: 8
        color: Style.tile
        border.color: Style.line
        anchors.top: parent.top
        anchors.left: root.mirrored ? undefined : parent.left
        anchors.right: root.mirrored ? parent.right : undefined
        clip: true

        Image {
            anchors.fill: parent
            source: root.player && root.player.coverArtUrl ? root.player.coverArtUrl : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }

        Text {
            anchors.centerIn: parent
            visible: !(root.player && root.player.isLoaded)
            text: "♫"
            color: Style.subText
            font.pixelSize: 26
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.coverTapped()
        }
    }

    PhraseLeds {
        id: leds

        objectName: root.group === "[Channel1]" ? "leds1" : "leds2"
        anchors.top: parent.top
        anchors.topMargin: 3
        anchors.left: root.mirrored ? undefined : cover.right
        anchors.right: root.mirrored ? cover.left : undefined
        anchors.leftMargin: root.mirrored ? 0 : 6
        anchors.rightMargin: root.mirrored ? 6 : 0
        group: root.group
        deckColor: root.deckColor
        // 4 over 4, as tall as the cover's upper half
        square: 8
    }

    Column {
        id: lines

        anchors.top: parent.top
        anchors.left: root.mirrored ? parent.left : leds.right
        anchors.right: root.mirrored ? leds.left : parent.right
        anchors.leftMargin: root.mirrored ? 0 : 8
        anchors.rightMargin: root.mirrored ? 8 : 0
        spacing: 1

        Item {
            width: parent.width
            height: 18

            Text {
                id: key

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: root.mirrored ? undefined : parent.right
                anchors.left: root.mirrored ? parent.left : undefined
                text: Mixxx.DJMantra.camelot(keyControl.value)
                color: root.deckColor
                font.pixelSize: 13
                font.bold: true
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: root.mirrored ? key.right : parent.left
                anchors.right: root.mirrored ? parent.right : key.left
                anchors.leftMargin: root.mirrored ? 6 : 0
                anchors.rightMargin: root.mirrored ? 0 : 6
                horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
                text: root.player ? root.player.artist : ""
                color: Style.subText
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }

        Text {
            width: parent.width
            horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
            text: root.player && root.player.isLoaded ? root.player.title : qsTr("Tap the cover for a song")
            color: Style.text
            font.pixelSize: 16
            elide: Text.ElideRight
        }

        Counter {
            width: parent.width
            height: 20
            group: root.group
            horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
        }
    }

}
