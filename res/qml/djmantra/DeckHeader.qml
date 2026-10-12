import "."
import ".." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's header (round 18): cover, artist, title, key and genre. The times
// and the phrase are in the matrix display.
// Deck 1 reads left to right, deck 2 mirrored (cover at the outer edge).
Item {
    id: root

    property string group
    property bool mirrored: false
    property color deckColor: Style.deck1
    property var player: Mixxx.PlayerManager.getPlayer(group)
    signal coverTapped()

    implicitHeight: 58

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

    Column {
        // artist, title, genre (with the key); no times (round 18: the time
        // left counts down in the matrix)
        id: lines

        anchors.top: parent.top
        anchors.left: root.mirrored ? parent.left : cover.right
        anchors.right: root.mirrored ? cover.left : parent.right
        anchors.leftMargin: root.mirrored ? 0 : 8
        anchors.rightMargin: root.mirrored ? 8 : 0
        spacing: 1

        Text {
            width: parent.width
            horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
            text: root.player && root.player.isLoaded ? root.player.artist : ""
            color: Style.subText
            font.pixelSize: 13
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            horizontalAlignment: root.mirrored ? Text.AlignRight : Text.AlignLeft
            text: root.player && root.player.isLoaded ? root.player.title : qsTr("Tap the cover for a song")
            color: Style.text
            font.pixelSize: 16
            font.bold: true
            elide: Text.ElideRight
        }

        Row {
            // the key in the deck's colour, then the genre (looked up online
            // when the file has none); deck 2 mirrored, so each key stays
            // on its own deck's side
            anchors.left: root.mirrored ? undefined : parent.left
            anchors.right: root.mirrored ? parent.right : undefined
            layoutDirection: root.mirrored ? Qt.RightToLeft : Qt.LeftToRight
            spacing: 6

            Text {
                text: Mixxx.DJMantra.camelot(keyControl.value)
                color: root.deckColor
                font.pixelSize: 13
                font.bold: true
            }

            Text {
                width: Math.min(implicitWidth, lines.width - 40)
                text: root.player && root.player.isLoaded ? root.player.genre : ""
                color: Style.accent
                font.pixelSize: 13
                elide: Text.ElideRight
            }
        }
    }
}
