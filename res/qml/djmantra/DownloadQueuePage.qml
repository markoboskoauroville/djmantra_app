import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// DOWNLOAD QUEUES (Marko 14.10.2026: "download queue is actually just a playlist ... for Spotify,
// Tidal, or YouTube Music ... export the text file with the lists of all the links"): one
// text file per service in djmantra_app/Download_Queue, a line per song "artist | title |
// link". Songs come in from the BEATSTATS charts (+ QUEUE); here: read, open, remove.
Rectangle {
    id: root

    signal done()

    readonly property var services: ["Spotify", "TIDAL", "YouTube Music"]
    property string service: Mixxx.DJMantra.setting("queue_service", "Spotify")
    property int version: 0
    readonly property var items: version >= 0 ? Mixxx.DJMantra.downloadQueue(service) : []

    color: Style.background

    MouseArea {
        anchors.fill: parent
    }

    Column {
        id: head

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        spacing: 8

        Item {
            width: parent.width
            height: 44

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("DOWNLOAD QUEUES")
                color: Style.accent
                font.pixelSize: 18
                font.bold: true
            }

            CloseButton {
                objectName: "queueClose"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.done()
            }
        }

        Row {
            spacing: 6

            Repeater {
                model: root.services

                delegate: DButton {
                    required property string modelData

                    objectName: "queue_" + modelData
                    width: (head.width - 12) / 3
                    height: 44
                    mode: ""
                    text: modelData.toUpperCase()
                    fontSize: 12
                    lit: root.service === modelData
                    onClicked: {
                        root.service = modelData;
                        Mixxx.DJMantra.setSetting("queue_service", modelData);
                    }
                }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WrapAnywhere
            text: root.items.length + qsTr(" songs  \u00B7  ") + Mixxx.DJMantra.downloadQueueFile(root.service)
            color: Style.subText
            font.pixelSize: 12
        }
    }

    ListView {
        anchors.top: head.bottom
        anchors.topMargin: 8
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        model: root.items

        delegate: Rectangle {
            id: entry

            required property var modelData
            required property int index

            width: ListView.view.width
            height: 56
            color: "transparent"

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: openButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    width: parent.width
                    text: entry.modelData.title
                    color: Style.text
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: entry.modelData.artist
                    color: Style.subText
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }

            DButton {
                id: openButton

                anchors.right: removeButton.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: 64
                height: 40
                text: qsTr("OPEN")
                fontSize: 12
                onClicked: Qt.openUrlExternally(entry.modelData.link)
            }

            DButton {
                id: removeButton

                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                height: 40
                text: "\u00D7"
                fontSize: 20
                onClicked: {
                    Mixxx.DJMantra.removeFromDownloadQueue(root.service, entry.index);
                    Toasts.show(qsTr("REMOVED FROM ") + root.service.toUpperCase());
                    ++root.version;
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Style.surface
            }
        }
    }
}
