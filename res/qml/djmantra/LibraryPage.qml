import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The phone's music folders (Music, Download): a tap on a folder opens it,
// LOAD 1 / LOAD 2 put a song on a deck. Back closes the folder or the page.
Rectangle {
    id: root

    property string folder: ""
    property string deck: "" // a deck's cover was tapped: its LOAD button only
    property var entries: folder === "" ? Mixxx.DJMantra.musicRoots() : Mixxx.DJMantra.listFolder(folder)
    signal done()

    color: Style.background

    function back() {
        if (folder === "") {
            done();
            return;
        }
        const roots = Mixxx.DJMantra.musicRoots();
        for (let i = 0; i < roots.length; ++i) {
            if (roots[i].path === folder) {
                folder = "";
                return;
            }
        }
        folder = folder.substring(0, folder.lastIndexOf("/"));
    }

    property string pendingPath: ""
    property string pendingGroup: ""

    function load(path, group, sure) {
        if (!sure && Mixxx.DJMantra.isPlaying(group)) {
            // a playing deck is not replaced without asking (round 13)
            pendingPath = path;
            pendingGroup = group;
            return;
        }
        Mixxx.DJMantra.log("load " + group + " " + path);
        Mixxx.PlayerManager.getPlayer(group).loadTrackFromLocation(path, false);
        Mixxx.DJMantra.noteLoaded(group);
        pendingPath = "";
        done();
    }

    Rectangle {
        id: bar

        width: parent.width
        height: 52
        color: Style.surface

        DButton {
            id: backButton

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 52
            height: 40
            text: "\u2190"
            fontSize: 22
            onClicked: root.back()
        }

        Text {
            anchors.left: backButton.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: root.folder === "" ? qsTr("Folders") : root.folder.substring(root.folder.lastIndexOf("/") + 1)
            color: Style.text
            font.pixelSize: 20
            font.bold: true
            elide: Text.ElideMiddle
        }
    }

    ListView {
        id: list

        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true
        model: root.entries

        delegate: Rectangle {
            id: row

            required property var modelData

            width: list.width
            height: 56
            color: rowArea.pressed ? Style.pressed : "transparent"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.right: buttons.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: (row.modelData.isFolder ? "\uD83D\uDCC1  " : "\u266B  ") + row.modelData.name
                color: Style.text
                font.pixelSize: 15
                elide: Text.ElideRight
            }

            MouseArea {
                id: rowArea

                anchors.fill: parent
                enabled: row.modelData.isFolder
                onClicked: root.folder = row.modelData.path
            }

            Row {
                id: buttons

                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                visible: !row.modelData.isFolder

                // One LOAD: onto the deck whose cover was tapped, or the free deck
                // (Marko, 12.10.2026: "load to free deck", as djay)
                DButton {
                    width: 72
                    height: 40
                    text: root.deck === "[Channel1]" ? "LOAD 1" : root.deck === "[Channel2]" ? "LOAD 2" : "LOAD"
                    fontSize: 12
                    onColor: Style.accent
                    onClicked: root.load(row.modelData.path,
                            root.deck !== "" ? root.deck : Mixxx.DJMantra.freeDeck(), false)
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#26272b"
            }
        }
    }

    Rectangle {
        // "Deck 2 is playing": load anyway?
        anchors.fill: parent
        visible: root.pendingPath !== ""
        color: "#b0000000"

        MouseArea {
            anchors.fill: parent
        }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 48
            height: confirmColumn.height + 32
            radius: 12
            color: Style.surface

            Column {
                id: confirmColumn

                anchors.centerIn: parent
                width: parent.width - 32
                spacing: 12

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    color: Style.text
                    font.pixelSize: 16
                    text: qsTr("Deck %1 is playing. Load the new song anyway?")
                            .arg(root.pendingGroup === "[Channel2]" ? 2 : 1)
                }

                Row {
                    anchors.right: parent.right
                    spacing: 10

                    DButton {
                        width: 110
                        height: 44
                        text: qsTr("CANCEL")
                        onClicked: root.pendingPath = ""
                    }

                    DButton {
                        width: 110
                        height: 44
                        text: qsTr("LOAD")
                        lit: true
                        onClicked: root.load(root.pendingPath, root.pendingGroup, true)
                    }
                }
            }
        }
    }
}
