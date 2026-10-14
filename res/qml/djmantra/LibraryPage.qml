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

    // Marko 14.10.2026: a FILTER above the list (the open folder's songs whose title, artist or
    // file name has every typed word) and FIND ON PHONE (the same words through every song on
    // the phone: Android's media index of the phone, SD card and USB sticks). Both show their
    // hits in `found`, in the list or thumbnail view; clearing the text goes back to the folder.
    ListModel { id: found }
    property string filterText: ""
    property bool phoneMode: false
    readonly property bool searching: filterText !== "" || phoneMode
    readonly property var shownModel: searching ? found : songs

    function rowCopy(r) {
        return {name: r.name, path: r.path, isFolder: r.isFolder, artist: r.artist || "",
                title: r.title || r.name, bpm: r.bpm || 0, key: r.key || "", cover: r.cover || "",
                duration: r.duration || 0, known: true};
    }

    function refilter() {
        if (phoneMode || filterText === "")
            return;
        const words = filterText.toLowerCase().split(/\s+/).filter(w => w !== "");
        found.clear();
        for (let i = 0; i < songs.count; ++i) {
            const r = songs.get(i);
            const text = (r.title + " " + r.artist + " " + r.name).toLowerCase();
            if (words.every(w => text.indexOf(w) >= 0))
                found.append(rowCopy(r));
        }
        view.currentIndex = -1;
    }

    function findOnPhone() {
        if (filterText.trim() === "") {
            Toasts.show(qsTr("TYPE A TITLE OR ARTIST FIRST"));
            return;
        }
        phoneMode = true;
        found.clear();
        const hits = Mixxx.DJMantra.findSongs(filterText);
        for (const h of hits)
            found.append(rowCopy(h));
        Toasts.show(qsTr("FOUND ON PHONE: ") + hits.length);
        Mixxx.DJMantra.log("find on phone '" + filterText + "': " + hits.length);
    }

    function clearSearch() {
        searchInput.text = "";
        filterText = "";
        phoneMode = false;
        found.clear();
    }

    onFilterTextChanged: {
        if (filterText === "")
            phoneMode = false;
        else if (phoneMode)
            phoneSearchDelay.restart();
        else
            refilter();
    }

    Timer {
        // phone search again while typing (after a short pause: it reads the media index)
        id: phoneSearchDelay

        interval: 400
        onTriggered: root.findOnPhone()
    }

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
        clearSearch();
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
                root.refilter();
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
                       bpm: r.bpm, key: r.key, cover: r.cover, duration: r.duration, known: r.known});
        }
        // Camelot keys in wheel order: 1A 1B 2A ... 12B
        const keyRank = (k) => k === "" ? 99 : parseInt(k) * 2 + (k.endsWith("B") ? 1 : 0);
        const value = (r) => key === "bpm" ? (r.bpm > 0 ? r.bpm : 1e9)
                : key === "duration" ? (r.duration > 0 ? r.duration : 1e9)
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
        refilter();
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
    // Marko 14.10.2026: "two views: list view and thumbnail view" (remembered)
    property bool thumbnails: Mixxx.DJMantra.setting("library_view", "list") === "thumbnails"
    readonly property Item view: thumbnails ? grid : list

    // a song's length as YouTube writes it on its thumbnails: 3:07, 1:02:45
    function lengthText(seconds) {
        if (!(seconds > 0))
            return "";
        const t = Math.round(seconds);
        const h = Math.floor(t / 3600), m = Math.floor(t / 60) % 60, sec = t % 60;
        const ss = (sec < 10 ? "0" : "") + sec;
        return h > 0 ? h + ":" + (m < 10 ? "0" : "") + m + ":" + ss : m + ":" + ss;
    }

    function moveSelection(steps) {
        if (shownModel.count === 0)
            return;
        controllerSelect = true;
        const index = view.currentIndex < 0 ? 0 : view.currentIndex + steps;
        view.currentIndex = Math.max(0, Math.min(shownModel.count - 1, index));
        view.positionViewAtIndex(view.currentIndex, ListView.Contain);
    }

    // a press (group "": the free deck, or the deck whose cover was tapped)
    // or a deck's LOAD: a folder opens, a song loads (a playing deck: the same
    // press again confirms)
    function activate(group) {
        if (view.currentIndex < 0 || view.currentIndex >= shownModel.count) {
            moveSelection(0);
            return;
        }
        const item = shownModel.get(view.currentIndex);
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

        DButton {
            // list / thumbnails
            id: viewButton

            objectName: "libraryView"
            anchors.right: closeButton.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            visible: root.folder !== ""
            width: visible ? 44 : 0
            height: 40
            text: root.thumbnails ? "\u2630" : "\u25A6"
            fontSize: 20
            onClicked: {
                const keep = root.view.currentIndex;
                root.thumbnails = !root.thumbnails;
                Mixxx.DJMantra.setSetting("library_view", root.thumbnails ? "thumbnails" : "list");
                Mixxx.DJMantra.log("library view: " + (root.thumbnails ? "thumbnails" : "list"));
                Toasts.show(root.thumbnails ? qsTr("VIEW \u2192 THUMBNAILS") : qsTr("VIEW \u2192 LIST"));
                if (keep >= 0) {
                    root.view.currentIndex = keep;
                    root.view.positionViewAtIndex(keep, ListView.Contain);
                }
            }
        }

        Column {
            // the playlist: songs, total time (matrix font, as the clock) and % analysed
            id: target

            anchors.right: viewButton.left
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

    Rectangle {
        // FILTER (this folder) and FIND ON PHONE (Marko 14.10.2026)
        id: searchRow

        anchors.top: bar.bottom
        width: parent.width
        height: 46
        color: Style.background

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.right: phoneButton.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            height: 38
            radius: Style.radius
            color: Style.tile
            border.color: searchInput.activeFocus ? Style.accent : Style.surface

            Text {
                // the magnifier and, while empty, the hint
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: "\uD83D\uDD0D"
                color: Style.subText
                font.pixelSize: 18
            }

            TextInput {
                id: searchInput

                objectName: "songFilter"
                anchors.left: parent.left
                anchors.leftMargin: 32
                anchors.right: clearSearchButton.left
                anchors.verticalCenter: parent.verticalCenter
                color: Style.text
                font.pixelSize: 16
                clip: true
                inputMethodHints: Qt.ImhNoPredictiveText
                onTextChanged: root.filterText = text.trim()
                onAccepted: focus = false

                Text {
                    anchors.fill: parent
                    visible: searchInput.text === "" && !searchInput.activeFocus
                    text: root.folder !== "" ? qsTr("Filter this folder") : qsTr("Find a song")
                    color: Style.line
                    font.pixelSize: 16
                }
            }

            Text {
                id: clearSearchButton

                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                width: visible ? 32 : 0
                visible: searchInput.text !== ""
                horizontalAlignment: Text.AlignHCenter
                text: "\u2715"
                color: Style.subText
                font.pixelSize: 16

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6
                    onClicked: root.clearSearch()
                }
            }
        }

        DButton {
            id: phoneButton

            objectName: "findOnPhone"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 118
            height: 38
            mode: ""
            text: qsTr("FIND ON PHONE")
            fontSize: 11
            lit: root.phoneMode
            onClicked: {
                searchInput.focus = false;
                root.findOnPhone();
            }
        }
    }

    Row {
        // the column headers: a tap sorts by that column, again reverses
        id: header

        anchors.top: searchRow.bottom
        width: parent.width
        height: 32
        visible: root.folder !== "" || root.phoneMode

        Repeater {
            // Marko 14.10.2026: also by LENGTH
            model: [{key: "title", label: qsTr("TITLE"), w: 0.32}, {key: "artist", label: qsTr("ARTIST"), w: 0.24},
                    {key: "duration", label: qsTr("LENGTH"), w: 0.16}, {key: "bpm", label: qsTr("BPM"), w: 0.14},
                    {key: "key", label: qsTr("KEY"), w: 0.14}]

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
                    onClicked: {
                        root.sortBy(column.modelData.key, root.sortKey === column.modelData.key ? !root.sortUp : true);
                        Toasts.show(qsTr("SORTED BY ") + column.modelData.label + (root.sortUp ? " \u25B2" : " \u25BC"));
                    }
                }
            }
        }
    }

    ListView {
        id: list

        anchors.top: header.visible ? header.bottom : searchRow.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true
        model: root.shownModel
        // the empty space under the songs: a tap brings Android's navigation
        // bar, as anywhere else empty (round 20: the list swallowed the tap)
        footer: MouseArea {
            property bool barWasShown: false

            width: list.width
            height: Math.max(0, list.height - list.count * 56)
            onPressed: barWasShown = Mixxx.DJMantra.barsVisible()
            onClicked: {
                if (!barWasShown) {
                    Mixxx.DJMantra.log("show the navigation bar");
                    Mixxx.DJMantra.showBars();
                }
            }
        }

        visible: !root.thumbnails

        delegate: Rectangle {
            id: row

            required property real duration
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

                LengthBadge {
                    text: row.isFolder ? "" : root.lengthText(row.duration)
                    fontSize: 9
                }
            }

            Column {
                // title over artist, as wide as the TITLE and ARTIST columns
                anchors.left: art.right
                anchors.leftMargin: 8
                anchors.right: lengthText.left
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
                id: lengthText

                anchors.right: bpmText.left
                anchors.verticalCenter: parent.verticalCenter
                width: list.width * 0.16
                horizontalAlignment: Text.AlignHCenter
                text: row.isFolder ? "" : root.lengthText(row.duration)
                color: Style.subText
                font.pixelSize: 14
            }

            Text {
                id: bpmText

                anchors.right: keyText.left
                anchors.verticalCenter: parent.verticalCenter
                width: list.width * 0.14
                horizontalAlignment: Text.AlignHCenter
                text: !row.isFolder && row.bpm > 0 ? row.bpm.toFixed(1) : ""
                color: Style.text
                font.pixelSize: 14
            }

            Text {
                id: keyText

                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: list.width * 0.14
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

    // YouTube's length on the thumbnail: white on a dark rounded box, bottom right
    component LengthBadge: Rectangle {
        property alias text: label.text
        property int fontSize: 12

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 2
        visible: text !== ""
        width: label.implicitWidth + 2 * Math.round(fontSize / 3)
        height: label.implicitHeight
        radius: 3
        color: "#cc000000"

        Text {
            id: label

            anchors.centerIn: parent
            color: "white"
            font.pixelSize: parent.fontSize
            font.bold: true
        }
    }

    GridView {
        // the thumbnail view: big covers, 3 a row upright (more on a wide screen)
        id: grid

        objectName: "libraryGrid"
        anchors.top: header.visible ? header.bottom : searchRow.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 4
        width: parent.width - 8
        visible: root.thumbnails
        clip: true
        model: root.shownModel
        readonly property int columns: Math.max(3, Math.floor(width / 150))
        cellWidth: width / columns
        cellHeight: cellWidth + 44

        delegate: Item {
            id: tile

            required property real duration
            required property string name
            required property string path
            required property bool isFolder
            required property string artist
            required property string title
            required property real bpm
            required property string key
            required property string cover
            required property int index
            readonly property bool wasPlayed: !isFolder && root.played[path] === true

            width: grid.cellWidth
            height: grid.cellHeight

            Rectangle {
                anchors.fill: parent
                anchors.margins: 2
                radius: 8
                color: tileArea.pressed ? Style.pressed : "transparent"
                border.color: root.controllerSelect && tile.index === grid.currentIndex ? Style.accent : "transparent"
                border.width: 2
            }

            Rectangle {
                id: thumb

                x: 5
                y: 5
                width: tile.width - 10
                height: width
                radius: 8
                color: Style.tile
                clip: true
                opacity: tile.wasPlayed ? 0.5 : 1

                Image {
                    anchors.fill: parent
                    visible: !tile.isFolder && tile.cover !== ""
                    source: tile.isFolder ? "" : tile.cover
                    sourceSize.width: 256
                    sourceSize.height: 256
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                Text {
                    anchors.centerIn: parent
                    visible: tile.isFolder || tile.cover === ""
                    text: tile.isFolder ? "\uD83D\uDCC1" : "\u266B"
                    color: tile.isFolder ? Style.accent : Style.subText
                    font.pixelSize: 40
                }

                Text {
                    // BPM and key, top left, as small as the length
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 4
                    visible: text !== ""
                    text: tile.isFolder ? "" : (tile.bpm > 0 ? tile.bpm.toFixed(0) : "") + (tile.key !== "" ? " " + tile.key : "")
                    color: "white"
                    style: Text.Outline
                    styleColor: "#cc000000"
                    font.pixelSize: 11
                    font.bold: true
                }

                LengthBadge {
                    anchors.margins: 4
                    text: tile.isFolder ? "" : root.lengthText(tile.duration)
                    fontSize: 12
                }
            }

            Column {
                anchors.top: thumb.bottom
                anchors.topMargin: 3
                x: 5
                width: tile.width - 10

                Text {
                    width: parent.width
                    text: (tile.wasPlayed ? "\u2713 " : "") + (tile.isFolder ? tile.name : tile.title)
                    color: tile.wasPlayed ? Style.line : Style.text
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: tile.isFolder ? qsTr("Folder") : tile.artist
                    color: tile.wasPlayed ? Style.line : Style.subText
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: tileArea

                anchors.fill: parent
                onClicked: {
                    if (tile.isFolder)
                        root.folder = tile.path;
                    else
                        root.load(tile.path, root.deck !== "" ? root.deck : Mixxx.DJMantra.freeDeck(), false);
                }
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
