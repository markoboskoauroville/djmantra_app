import "."
import ".." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's header: cover, artist and key, title, the total and the remaining
// time (Marko, 12.10.2026, the redesign: elapsed time and bar.beat are in the
// matrix display, the phrase lights only there).
// Deck 1 reads left to right, deck 2 mirrored (cover at the outer edge).
Item {
    id: root

    property string group
    property bool mirrored: false
    property color deckColor: Style.deck1
    property var player: Mixxx.PlayerManager.getPlayer(group)
    signal coverTapped()

    implicitHeight: 56

    Mixxx.ControlProxy { id: keyControl; group: root.group; key: "key" }
    Mixxx.ControlProxy { id: position; group: root.group; key: "playposition" }
    Mixxx.ControlProxy { id: duration; group: root.group; key: "duration" }

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

    Column {
        id: lines

        anchors.top: parent.top
        anchors.left: root.mirrored ? parent.left : cover.right
        anchors.right: root.mirrored ? cover.left : parent.right
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

        Text {
            // total and remaining
            width: parent.width
            horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
            text: root.player && root.player.isLoaded
                    ? Mixxx.DJMantra.formatTime(duration.value) + "   -" +
                      Mixxx.DJMantra.formatTime(Math.max(0, duration.value * (1 - Math.max(0, position.value))))
                    : ""
            color: Style.text
            font.pixelSize: 14
            font.bold: true
        }
    }

}
