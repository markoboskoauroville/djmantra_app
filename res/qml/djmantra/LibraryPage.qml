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

    function load(path, group) {
        Mixxx.DJMantra.log("load " + group + " " + path);
        Mixxx.PlayerManager.getPlayer(group).loadTrackFromLocation(path, false);
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
                spacing: 6
                visible: !row.modelData.isFolder

                DButton {
                    visible: root.deck === "" || root.deck === "[Channel1]"
                    width: 60
                    height: 40
                    text: "LOAD 1"
                    fontSize: 12
                    onColor: Style.deck1
                    onClicked: root.load(row.modelData.path, "[Channel1]")
                }

                DButton {
                    visible: root.deck === "" || root.deck === "[Channel2]"
                    width: 60
                    height: 40
                    text: "LOAD 2"
                    fontSize: 12
                    onColor: Style.deck2
                    onClicked: root.load(row.modelData.path, "[Channel2]")
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
}
