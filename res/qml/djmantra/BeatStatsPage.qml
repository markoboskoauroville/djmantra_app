import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// BEATSTATS (Marko 14.10.2026): the charts read from beatstats.com (OPEN BEATSTATS: the site in a
// browser inside the app; there READ CHART stores the chart). Flip between the charts read; per
// song three search links, SPOTIFY / TIDAL / YT MUSIC (music.youtube.com, no videos), and + to
// put it in a download queue (Documents/DJ Mantra/Download_Queue/<service>.txt). V82 (Marko 16.10.2026: "There are
// many songs and to add one by one for me ... put plus as a column and when I press ... it is going to write all these
// songs from the list to one text file"): + ALL at the head of the + column puts every song of the chart in one
// queue's text file at once.
Rectangle {
    id: root

    signal done()

    property var charts: Mixxx.DJMantra.beatStatsCharts()
    property int current: 0
    readonly property var chart: charts.length > 0 ? charts[Math.min(current, charts.length - 1)] : null
    property var queueSong: null
    // + ALL: the whole chart into one queue
    property bool queueAll: false
    // V87: the chart's round through TIDAL is running
    property bool tidalBusy: false
    property string tidalLast: ""

    Connections {
        target: Mixxx.DJMantra
        function onTidalResult(action, result) {
            if (action !== "chart")
                return;
            root.tidalBusy = false;
            // "<name>|<found>/<total> or error|<on the phone>|<queued>"
            const p = result.split("|");
            root.tidalLast = p[1] && p[1].indexOf("error") === 0
                    ? qsTr("TIDAL: ") + p[1]
                    : qsTr("TIDAL: %1 ON TIDAL, %2 IN THE APP PLAYLIST \"%3\", %4 TO THE TIDAL QUEUE").arg(p[1]).arg(p[2]).arg(p[0]).arg(p[3]);
            Toasts.show(root.tidalLast);
        }
    }

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

        Item {
            // the list's head: the songs, and + ALL over the + column
            visible: root.chart !== null
            width: parent.width
            height: visible ? 40 : 0

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.chart ? root.chart.entries.length + qsTr(" SONGS") : ""
                color: Style.subText
                font.pixelSize: 13
                font.bold: true
            }

            DButton {
                // V87 (Marko 16.10.2026: "go through beatstats, scrap few of the playlists, and play them
                // through the dj app"): the chart -> a TIDAL playlist "BeatStats - <chart>" (exact songs by
                // ISRC, else the Playlist Generator's search) -> the app's playlist of the same name with
                // the songs on the phone (ready for the decks); the rest into the TIDAL download queue
                objectName: "beatstatsTidal"
                anchors.right: addAll.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: 150
                height: 40
                mode: ""
                text: root.tidalBusy ? qsTr("TIDAL ...") : qsTr("TIDAL PLAYLIST")
                fontSize: 13
                lit: !root.tidalBusy
                onClicked: {
                    if (root.tidalBusy || !root.chart)
                        return;
                    if (!Mixxx.DJMantra.tidalLoggedIn()) {
                        Toasts.show(qsTr("LOG IN TO TIDAL FIRST (SETTINGS)"));
                        return;
                    }
                    root.tidalBusy = true;
                    const name = "BeatStats - " + root.chart.chart_title;
                    const songs = root.chart.entries.map(s => ({artist: s.artist || "", title: s.title || "", isrc: s.isrc || ""}));
                    Toasts.show(qsTr("TIDAL: MAKING ") + name);
                    Mixxx.DJMantra.log("BeatStats: TIDAL playlist " + name + " (" + songs.length + " songs)");
                    Mixxx.DJMantra.tidalChart(name, JSON.stringify(songs));
                }
            }

            DButton {
                id: addAll

                objectName: "beatstatsAddAll"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 96
                height: 40
                mode: ""
                text: qsTr("+ ALL")
                fontSize: 14
                lit: true
                onClicked: root.queueAll = true
            }
        }

        Text {
            // V87: the last TIDAL round of this view
            visible: root.tidalLast !== "" || root.tidalBusy
            width: parent.width
            wrapMode: Text.WordWrap
            text: root.tidalBusy ? qsTr("TIDAL: finding the songs, making the playlist, matching the phone's songs ...") : root.tidalLast
            color: Style.accent
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
        visible: root.queueSong !== null || root.queueAll
        color: "#b0000000"

        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.queueSong = null;
                root.queueAll = false;
            }
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - 48
            spacing: 8

            Text {
                width: parent.width
                text: root.queueAll && root.chart
                      ? qsTr("ADD ALL %1 SONGS TO DOWNLOAD QUEUE").arg(root.chart.entries.length)
                      : qsTr("ADD TO DOWNLOAD QUEUE")
                wrapMode: Text.WordWrap
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
                        if (root.queueAll) {
                            // the whole chart, in its order, into this queue's one text file
                            let added = 0;
                            const songs = root.chart ? root.chart.entries : [];
                            Mixxx.DJMantra.queueBatch(true);
                            for (const s of songs) {
                                if (Mixxx.DJMantra.addToDownloadQueue(modelData, s.artist, s.title,
                                        s.isrc || "", s.tidal || ""))
                                    ++added;
                            }
                            Mixxx.DJMantra.queueBatch(false);
                            Toasts.show(qsTr("%1 ADDED TO %2 (%3 ALREADY IN)").arg(added)
                                    .arg(modelData.toUpperCase()).arg(songs.length - added));
                            Mixxx.DJMantra.log("BeatStats: + ALL " + (root.chart ? root.chart.chart_title : "")
                                    + " -> " + Mixxx.DJMantra.downloadQueueFile(modelData) + ": " + added
                                    + " added, " + (songs.length - added) + " already in");
                            root.queueAll = false;
                            return;
                        }
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
                onClicked: {
                    root.queueSong = null;
                    root.queueAll = false;
                }
            }
        }
    }
}
