import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// BEATSTATS (Marko 14.10.2026): the charts read from beatstats.com (OPEN BEATSTATS: the site in a
// browser inside the app; there READ CHART stores the chart). Flip between the charts read; per
// song three search links, SPOTIFY / TIDAL / YT MUSIC (music.youtube.com, no videos), and + to
// put it in a download queue (djmantra_app/Download_Queue/<service>.txt).
Rectangle {
    id: root

    signal done()

    property var charts: Mixxx.DJMantra.beatStatsCharts()
    property int current: 0
    readonly property var chart: charts.length > 0 ? charts[Math.min(current, charts.length - 1)] : null
    property var queueSong: null

    function reload() {
        charts = Mixxx.DJMantra.beatStatsCharts();
    }

    color: Style.background

    // back from the browser (READ CHART there): the new chart shows
    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state === Qt.ApplicationActive)
                root.reload();
        }
    }

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
                text: qsTr("BEATSTATS")
                color: Style.accent
                font.pixelSize: 18
                font.bold: true
            }

            CloseButton {
                objectName: "beatstatsClose"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.done()
            }
        }

        DButton {
            objectName: "openBeatStats"
            width: parent.width
            height: 48
            text: qsTr("OPEN BEATSTATS  →  a chart  →  READ CHART")
            fontSize: 12
            lit: true
            onClicked: Mixxx.DJMantra.openBeatStats()
        }

        Flickable {
            // the charts read: flip between them
            width: parent.width
            height: 40
            contentWidth: chartRow.width
            clip: true
            visible: root.charts.length > 0

            Row {
                id: chartRow

                spacing: 6

                Repeater {
                    model: root.charts

                    delegate: DButton {
                        required property var modelData
                        required property int index

                        width: Math.max(90, label.implicitWidth + 24)
                        height: 40
                        mode: ""
                        text: modelData.chart_title || qsTr("CHART")
                        fontSize: 11
                        lit: index === root.current
                        onClicked: root.current = index

                        Text {
                            id: label

                            visible: false
                            text: parent.text
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }
        }

        Text {
            visible: root.charts.length === 0
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("No chart read yet. OPEN BEATSTATS, pass the site's check once, open a chart (e.g. ORGANIC HOUSE) and tap READ CHART.")
            color: Style.subText
            font.pixelSize: 13
        }
    }

    ListView {
        anchors.top: head.bottom
        anchors.topMargin: 8
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        model: root.chart ? root.chart.entries : []

        delegate: Rectangle {
            id: song

            required property var modelData

            width: ListView.view.width
            height: 64
            color: "transparent"

            Text {
                id: rank

                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                horizontalAlignment: Text.AlignRight
                text: song.modelData.rank
                color: Style.accent
                font.pixelSize: 15
                font.bold: true
            }

            Column {
                anchors.left: rank.right
                anchors.leftMargin: 8
                anchors.right: links.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    width: parent.width
                    text: song.modelData.title
                    color: Style.text
                    font.pixelSize: 14
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: song.modelData.artist + (song.modelData.isrc ? "  ·  ISRC " + song.modelData.isrc : "")
                    color: Style.subText
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            Row {
                id: links

                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Repeater {
                    model: [["Spotify", "SPOT"], ["TIDAL", "TIDAL"], ["YouTube Music", "YT M"]]

                    delegate: DButton {
                        required property var modelData

                        width: 52
                        height: 44
                        mode: ""
                        text: modelData[1]
                        fontSize: 10
                        onClicked: Qt.openUrlExternally(Mixxx.DJMantra.searchLink(modelData[0],
                                song.modelData.artist, song.modelData.title,
                                song.modelData.isrc || "", song.modelData.tidal || ""))
                    }
                }

                DButton {
                    width: 40
                    height: 44
                    mode: ""
                    text: "+"
                    fontSize: 20
                    onClicked: root.queueSong = song.modelData
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

    Rectangle {
        // + : into which download queue
        anchors.fill: parent
        visible: root.queueSong !== null
        color: "#b0000000"

        MouseArea {
            anchors.fill: parent
            onClicked: root.queueSong = null
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - 48
            spacing: 8

            Text {
                width: parent.width
                text: qsTr("ADD TO DOWNLOAD QUEUE")
                color: Style.accent
                font.pixelSize: 16
                font.bold: true
            }

            Repeater {
                model: ["Spotify", "TIDAL", "YouTube Music"]

                delegate: DButton {
                    required property string modelData

                    width: parent.width
                    height: 48
                    text: modelData.toUpperCase()
                    onClicked: {
                        const added = Mixxx.DJMantra.addToDownloadQueue(modelData,
                                root.queueSong.artist, root.queueSong.title,
                                root.queueSong.isrc || "", root.queueSong.tidal || "");
                        Toasts.show((added ? qsTr("ADDED TO ") : qsTr("ALREADY IN ")) + modelData.toUpperCase());
                        root.queueSong = null;
                    }
                }
            }

            DButton {
                width: parent.width
                height: 48
                text: qsTr("CANCEL")
                onClicked: root.queueSong = null
            }
        }
    }
}
