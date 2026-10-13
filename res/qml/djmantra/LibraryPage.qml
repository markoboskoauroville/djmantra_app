import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The playlist (Marko, 12.10.2026, the redesign: "the rest of user interface
// is the playlist ... if I click on the song, it is going to be loaded inside
// the free deck, the one which is stopped"): the phone's music folders
// (Music, Download). A tap on a folder opens it, a tap on a song loads it
// onto the free deck (after a cover tap: onto that deck). Back: the folder up.
Rectangle {
    id: root

    property string folder: ""
    property string deck: "" // a deck's cover was tapped: onto that deck
    property var entries: folder === "" ? Mixxx.DJMantra.musicRoots() : Mixxx.DJMantra.listFolder(folder)
    // DJ Pro style (Marko, round 19): cover, title, artist, BPM, key; a tap
    // on a column header sorts by it (again: the other way round)
    // the songs played in this session (Marko, round 22: "mark played songs
    // ... so the user doesn't play the same song twice"): path -> true
    property var played: ({})
    property string sortKey: ""
    property bool sortUp: true
    property int filled: 0
    readonly property int analysisLeft: Mixxx.DJMantra.analysisLeft

    ListModel { id: songs }

    Connections {
        // a song analysed in advance: its BPM / key into the list
        target: Mixxx.DJMantra
        function onAnalysisChanged() {
            for (let i = 0; i < songs.count; ++i) {
                const r = songs.get(i);
                if (r.isFolder || (r.bpm > 0 && r.key !== ""))
                    continue;
                const info = Mixxx.DJMantra.songInfo(r.path);
                if ((info.bpm || 0) !== r.bpm || (info.key || "") !== r.key)
                    songs.set(i, {bpm: info.bpm || 0, key: info.key || ""});
            }
            root.updateStats();
        }
    }

    // Marko 13.10.2026: the playlist's statistics (songs, total time, % analysed) and
    // the analysis only of the open folder: it starts once the folder has been open
    // for 3 s and stops when the folder is left (the next visit goes on)
    property var missing: []
    property int songCount: 0
    property real totalSeconds: 0
    property int analysedPercent: 0

    function updateStats() {
        let count = 0, total = 0, done = 0;
        for (let i = 0; i < songs.count; ++i) {
            const r = songs.get(i);
            if (r.isFolder)
                continue;
            ++count;
            total += r.duration || 0;
            if (r.bpm > 0 && r.key !== "")
                ++done;
        }
        songCount = count;
        totalSeconds = total;
        analysedPercent = count > 0 ? Math.floor(100 * done / count) : 0;
    }

    function totalText() {
        const t = Math.round(totalSeconds);
        const h = Math.floor(t / 3600), m = Math.floor(t / 60) % 60, sec = t % 60;
        const two = (n) => (n < 10 ? "0" : "") + n;
        return (h > 0 ? h + ":" + two(m) : String(m)) + ":" + two(sec);
    }

    onFolderChanged: {
        analysisStart.stop();
        missing = [];
        Mixxx.DJMantra.stopAnalysis();
        if (folder !== "")
            analysisStart.restart();
    }

    Timer {
        // 3 s in the folder, then its songs without a BPM / key are analysed
        id: analysisStart

        interval: 3000
        onTriggered: {
            if (root.filled < songs.count) {
                restart(); // the list is still being read
                return;
            }
            if (root.missing.length > 0) {
                Mixxx.DJMantra.log("Analysis: folder " + root.folder + ", " + root.missing.length + " songs");
                Mixxx.DJMantra.analyzeAhead(root.missing);
            }
        }
    }

    onEntriesChanged: {
        songs.clear();
        for (const e of entries)
            songs.append({name: e.name, path: e.path, isFolder: root.folder === "" || e.isFolder === true, artist: "", title: e.name,
                          bpm: 0.0, key: "", cover: "", duration: 0.0, known: root.folder === "" || e.isFolder === true});
        filled = 0;
        filler.restart();
    }

    Timer {
        // the details a few songs at a time, so the list never freezes (the
        // first time a song is read its tags go into the library)
        id: filler

        interval: 30
        repeat: true
        onTriggered: {
            let done = 0;
            while (root.filled < songs.count && done < 4) {
                const row = songs.get(root.filled);
                if (!row.known) {
                    const i = Mixxx.DJMantra.songInfo(row.path);
                    songs.set(root.filled, {artist: i.artist || "", title: i.title || row.name,
                                           bpm: i.bpm || 0, key: i.key || "", cover: i.cover || "",
                                           duration: i.duration || 0, known: true});
                    ++done;
                }
                ++root.filled;
            }
            if (root.filled >= songs.count) {
                stop();
                // the songs without a BPM or key: analysed in advance, in the
                // background (Marko, round 19)
                const missing = [];
                for (let i = 0; i < songs.count; ++i) {
                    const r = songs.get(i);
                    if (!r.isFolder && (r.bpm <= 0 || r.key === ""))
                        missing.push(r.path);
                }
                root.missing = missing;
                root.updateStats();
                if (root.sortKey !== "")
                    root.sortBy(root.sortKey, root.sortUp);
            }
        }
    }

    // the deck's song now (its file path), "" if none
    function pathOnDeck(group) {
        const player = Mixxx.PlayerManager.getPlayer(group);
        if (!player || !player.isLoaded)
            return "";
        return decodeURIComponent(String(player.trackLocationUrl).replace(/^file:\/\//, ""));
    }

    // NEXT on a deck: the first song after the deck's song in the list as shown
    // (sorted), not played in this session and not on a deck, from the top again
    function nextUnplayed(group) {
        const here = pathOnDeck(group);
        const other = pathOnDeck(group === "[Channel1]" ? "[Channel2]" : "[Channel1]");
        let start = 0;
        for (let i = 0; i < songs.count; ++i) {
            if (songs.get(i).path === here) {
                start = i + 1;
                break;
            }
        }
        for (let k = 0; k < songs.count; ++k) {
            const r = songs.get((start + k) % songs.count);
            if (!r.isFolder && !played[r.path] && r.path !== here && r.path !== other)
                return r.path;
        }
        return "";
    }

    function loadOnto(path, group) {
        Mixxx.DJMantra.log("next " + group + ": load " + path);
        Mixxx.PlayerManager.getPlayer(group).loadTrackFromLocation(path, false);
        Mixxx.DJMantra.noteLoaded(group);
    }

    // the AutoMix tab plays this folder in the order shown (Marko 13.10.2026)
    function songPaths() {
        const paths = [];
        for (let i = 0; i < songs.count; ++i) {
            const r = songs.get(i);
            if (!r.isFolder)
                paths.push(r.path);
        }
        return paths;
    }

    function sortBy(key, up) {
        sortKey = key;
        sortUp = up;
        const rows = [];
        for (let i = 0; i < songs.count; ++i) {
            const r = songs.get(i);
            rows.push({name: r.name, path: r.path, isFolder: r.isFolder, artist: r.artist, title: r.title,
                       bpm: r.bpm, key: r.key, cover: r.cover, known: r.known});
        }
        // Camelot keys in wheel order: 1A 1B 2A ... 12B
        const keyRank = (k) => k === "" ? 99 : parseInt(k) * 2 + (k.endsWith("B") ? 1 : 0);
        const value = (r) => key === "bpm" ? (r.bpm > 0 ? r.bpm : 1e9)
                : key === "key" ? keyRank(r.key)
                : String(r[key]).toLowerCase();
        rows.sort((a, b) => {
            if (a.isFolder !== b.isFolder)
                return a.isFolder ? -1 : 1;
            const va = value(a), vb = value(b);
            const c = va < vb ? -1 : va > vb ? 1 : 0;
            return up ? c : -c;
        });
        songs.clear();
        for (const r of rows)
            songs.append(r);
        Mixxx.DJMantra.log("songs sorted by " + key + (up ? " up" : " down"));
    }
    signal done()
    // the ✕: closed without loading a song (Marko 13.10.2026: "I'm stuck")
    signal closed()

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
    // in front of the decks (browsing): ← closes it at the top level too
    property bool closable: false

    // The controller (Marko, 12.10.2026): its BROWSE knob moves a highlighted
    // row, a press or a deck's LOAD takes it
    property bool controllerSelect: false

    function moveSelection(steps) {
        if (songs.count === 0)
            return;
        controllerSelect = true;
        const index = list.currentIndex < 0 ? 0 : list.currentIndex + steps;
        list.currentIndex = Math.max(0, Math.min(songs.count - 1, index));
        list.positionViewAtIndex(list.currentIndex, ListView.Contain);
    }

    // a press (group "": the free deck, or the deck whose cover was tapped)
    // or a deck's LOAD: a folder opens, a song loads (a playing deck: the same
    // press again confirms)
    function activate(group) {
        if (list.currentIndex < 0 || list.currentIndex >= songs.count) {
            moveSelection(0);
            return;
        }
        const item = songs.get(list.currentIndex);
        if (item.isFolder) {
            Mixxx.DJMantra.log("controller: open folder " + item.path);
            folder = item.path;
            return;
        }
        const target = group !== "" ? group : deck !== "" ? deck : Mixxx.DJMantra.freeDeck();
        Mixxx.DJMantra.log("controller: load " + item.path + " onto " + target);
        load(item.path, target, pendingPath === item.path && pendingGroup === target);
    }

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
        height: 44
        color: Style.surface

        DButton {
            id: backButton

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            visible: root.folder !== "" || root.closable
            width: visible ? 52 : 0
            height: 40
            text: "\u2190"
            fontSize: 22
            onClicked: root.back()
        }

        CloseButton {
            id: closeButton

            objectName: "libraryClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            visible: root.closable
            width: visible ? 48 : 0
            onClicked: {
                Mixxx.DJMantra.log("song list closed (✕)");
                root.closed();
            }
        }

        Column {
            // the playlist: songs, total time (matrix font, as the clock) and % analysed
            id: target

            anchors.right: closeButton.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            visible: root.folder !== ""
            spacing: 3

            MatrixText {
                objectName: "playlistStats"
                anchors.right: parent.right
                height: 17
                text: root.songCount + " " + root.totalText()
            }

            MatrixText {
                objectName: "playlistAnalysed"
                anchors.right: parent.right
                height: 17
                text: root.analysedPercent + "%"
                litColor: root.analysedPercent >= 100 ? Style.earth : Style.accent
            }
        }

        Text {
            anchors.left: backButton.right
            anchors.leftMargin: 12
            anchors.right: target.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.folder === "" ? qsTr("Folders") : root.folder.substring(root.folder.lastIndexOf("/") + 1)
            color: Style.text
            font.pixelSize: 20
            font.bold: true
            elide: Text.ElideMiddle
        }
    }

    Row {
        // the column headers: a tap sorts by that column, again reverses
        id: header

        anchors.top: bar.bottom
        width: parent.width
        height: 32
        visible: root.folder !== ""

        Repeater {
            model: [{key: "title", label: qsTr("TITLE"), w: 0.40}, {key: "artist", label: qsTr("ARTIST"), w: 0.30},
                    {key: "bpm", label: qsTr("BPM"), w: 0.15}, {key: "key", label: qsTr("KEY"), w: 0.15}]

            delegate: Rectangle {
                id: column

                required property var modelData

                objectName: "sort_" + modelData.key
                width: header.width * modelData.w
                height: header.height
                color: area.pressed ? Style.pressed : Style.background

                Text {
                    anchors.centerIn: parent
                    text: column.modelData.label + (root.sortKey === column.modelData.key ? (root.sortUp ? " \u25B2" : " \u25BC") : "")
                    color: root.sortKey === column.modelData.key ? Style.accent : Style.subText
                    font.pixelSize: 12
                    font.bold: true
                }

                MouseArea {
                    id: area

                    anchors.fill: parent
                    onClicked: root.sortBy(column.modelData.key,
                            root.sortKey === column.modelData.key ? !root.sortUp : true)
                }
            }
        }
    }

    ListView {
        id: list

        anchors.top: header.visible ? header.bottom : bar.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true
        model: songs
        // the empty space under the songs: a tap brings Android's navigation
        // bar, as anywhere else empty (round 20: the list swallowed the tap)
        footer: MouseArea {
            property bool barWasShown: false

            width: list.width
            height: Math.max(0, list.height - songs.count * 56 - header.height)
            onPressed: barWasShown = Mixxx.DJMantra.barsVisible()
            onClicked: {
                if (!barWasShown) {
                    Mixxx.DJMantra.log("show the navigation bar");
                    Mixxx.DJMantra.showBars();
                }
            }
        }

        delegate: Rectangle {
            id: row

            required property string name
            required property string path
            required property bool isFolder
            required property string artist
            required property string title
            required property real bpm
            required property string key
            required property string cover
            readonly property bool wasPlayed: !isFolder && root.played[path] === true

            required property int index

            width: list.width
            height: 56
            color: rowArea.pressed ? Style.pressed
                    : root.controllerSelect && index === list.currentIndex ? Style.surface : "transparent"
            border.color: root.controllerSelect && index === list.currentIndex ? Style.accent : "transparent"
            border.width: 2

            Rectangle {
                // the cover (DJ Pro style), a note while there is none
                id: art

                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                height: 44
                radius: 6
                color: Style.tile
                clip: true

                Image {
                    anchors.fill: parent
                    visible: !row.isFolder && row.cover !== ""
                    source: row.isFolder ? "" : row.cover
                    sourceSize.width: 88
                    sourceSize.height: 88
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                Text {
                    anchors.centerIn: parent
                    visible: row.isFolder || row.cover === ""
                    text: row.isFolder ? "\uD83D\uDCC1" : "\u266B"
                    color: row.isFolder ? Style.accent : Style.subText
                    font.pixelSize: 20
                }
            }

            Column {
                // title over artist, as wide as the TITLE and ARTIST columns
                anchors.left: art.right
                anchors.leftMargin: 8
                anchors.right: bpmText.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    width: parent.width
                    // played in this set: dimmed, with a tick
                    text: (row.wasPlayed ? "\u2713 " : "") + (row.isFolder ? row.name : row.title)
                    color: row.wasPlayed ? Style.line : Style.text
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: row.isFolder ? qsTr("Folder") : row.artist
                    color: row.wasPlayed ? Style.line : Style.subText
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }

            Text {
                id: bpmText

                anchors.right: keyText.left
                anchors.verticalCenter: parent.verticalCenter
                width: list.width * 0.15
                horizontalAlignment: Text.AlignHCenter
                text: !row.isFolder && row.bpm > 0 ? row.bpm.toFixed(1) : ""
                color: Style.text
                font.pixelSize: 14
            }

            Text {
                id: keyText

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: list.width * 0.15
                horizontalAlignment: Text.AlignHCenter
                text: row.isFolder ? "" : row.key
                color: Style.accent
                font.pixelSize: 14
                font.bold: true
            }

            MouseArea {
                id: rowArea

                anchors.fill: parent
                onClicked: {
                    if (row.isFolder)
                        root.folder = row.path;
                    else
                        root.load(row.path, root.deck !== "" ? root.deck : Mixxx.DJMantra.freeDeck(), false);
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
