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
    // Marko 14.10.2026: two views, FOLDERS (the phone's folders) and PLAYLISTS (the app's own);
    // playlistsVersion makes the lists read again after a song is added / removed
    property bool playlistsView: Mixxx.DJMantra.setting("library_mode", "folders") === "playlists"
    property int playlistsVersion: 0
    readonly property bool inPlaylist: folder.startsWith("playlist:")
    readonly property bool playlistsRoot: folder === "" && playlistsView
    property var entries: {
        // read again after a playlist change only where playlists are shown
        const version = (playlistsRoot || inPlaylist) ? playlistsVersion : 0;
        return folder === "" ? (playlistsView ? Mixxx.DJMantra.playlistRoots() : Mixxx.DJMantra.musicRoots())
                             : Mixxx.DJMantra.listFolder(folder);
    }
    // Marko 14.10.2026: the list opens in the app's own root, djmantra_app (← shows Music,
    // Download beside it)
    Component.onCompleted: {
        const roots = Mixxx.DJMantra.musicRoots();
        if (folder === "" && !playlistsView && roots.length > 0 && roots[0].name === "djmantra_app")
            folder = roots[0].path;
    }

    // full screen / docked (Marko 14.10.2026), Main.qml lays it out
    property bool fullScreen: false
    signal toggleFullScreen()
    // the AutoMix tab is this same song list (Marko 14.10.2026: "same as songs with only just few
    // additional buttons"): an AUTOMIX button opens its dropdown, the playing and the next song
    // are marked, a song tapped while AutoMix runs is mixed in next
    property bool automixMode: false
    property bool automixRunning: false
    property string automixNow: ""
    property string automixNext: ""
    signal automixMenu()
    signal automixPick(string path)
    function pick(path) {
        if (automixMode && automixRunning)
            automixPick(path);
        else
            load(path, deck !== "" ? deck : Mixxx.DJMantra.freeDeck(), false);
    }
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

    function playlistInfo(e) {
        const d = new Date(e.created || 0);
        return (e.artist ? e.artist + "  ·  " : "") + (e.count || 0) + qsTr(" songs") + (e.created > 0 ? "  \u00B7  " + d.toLocaleDateString(Qt.locale(), Locale.ShortFormat) : "");
    }

    function rowCopy(r) {
        return {name: r.name, path: r.path, isFolder: r.isFolder, artist: r.artist || "",
                title: r.title || r.name, bpm: r.bpm || 0, key: r.key || "", cover: r.cover || "",
                duration: r.duration || 0, created: r.created || 0, count: r.count || 0, known: true};
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
        for (const h of hits) {
            const row = rowCopy(h);
            row.known = false; // its BPM, key and cover: read a few at a time (foundFiller)
            found.append(row);
        }
        foundFiller.next = 0;
        foundFiller.restart();
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
        // r41: the phone's hits get their BPM, key and cover as the folder's songs do
        id: foundFiller

        property int next: 0

        interval: 30
        repeat: true
        onTriggered: {
            let done = 0;
            while (next < found.count && done < 3) {
                const row = found.get(next);
                if (!row.known && row.path.startsWith("playlist:")) {
                    songs.set(root.filled, {duration: Mixxx.DJMantra.playlistLength(row.path.substring(9)), known: true});
                    ++done;
                } else if (!row.known) {
                    const i = Mixxx.DJMantra.songInfo(row.path);
                    found.set(next, {bpm: i.bpm || 0, key: i.key || "", cover: i.cover || "",
                                     artist: i.artist || row.artist, title: i.title || row.title,
                                     duration: i.duration || row.duration, known: true});
                    ++done;
                }
                ++next;
            }
            if (next >= found.count || !root.phoneMode)
                stop();
        }
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
            songs.append({name: e.name, path: e.path, isFolder: root.folder === "" || e.isFolder === true,
                          artist: root.playlistsRoot ? root.playlistInfo(e) : "", title: e.name,
                          bpm: 0.0, key: "", cover: "", duration: 0.0, created: e.created || 0, count: e.count || 0,
                          // a playlist's length is added up by the filler
                          known: (root.folder === "" || e.isFolder === true) && !root.playlistsRoot});
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
                       bpm: r.bpm, key: r.key, cover: r.cover, duration: r.duration, created: r.created,
                       count: r.count, known: r.known});
        }
        // Camelot keys in wheel order: 1A 1B 2A ... 12B
        const keyRank = (k) => k === "" ? 99 : parseInt(k) * 2 + (k.endsWith("B") ? 1 : 0);
        const value = (r) => key === "bpm" ? (r.bpm > 0 ? r.bpm : 1e9)
                : key === "duration" ? (r.duration > 0 ? r.duration : 1e9)
                : key === "created" ? r.created
                : key === "count" ? r.count
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
        if (inPlaylist) {
            folder = "";
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
            // full screen / docked (Marko 14.10.2026)
            id: fullButton

            objectName: "libraryFull"
            anchors.right: closeButton.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: 48
            height: 40
            text: root.fullScreen ? qsTr("DOCK") : qsTr("FULL")
            fontSize: 11
            lit: root.fullScreen
            onClicked: root.toggleFullScreen()
        }

        DButton {
            // the AutoMix tab: its settings drop down from here
            id: automixButton

            objectName: "automixMenu"
            anchors.right: fullButton.left
            anchors.rightMargin: visible ? 6 : 0
            anchors.verticalCenter: parent.verticalCenter
            visible: root.automixMode
            width: visible ? 104 : 0
            height: 40
            // the arrow is drawn (dropdown): U+25BE was an empty box in the phone's font
            text: qsTr("AUTOMIX") + "   "
            dropdown: true
            fontSize: 11
            lit: root.automixRunning
            onClicked: root.automixMenu()
        }

        DButton {
            // list / thumbnails
            id: viewButton

            objectName: "libraryView"
            anchors.right: automixButton.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            visible: root.folder !== "" || root.playlistsRoot
            width: visible ? 44 : 0
            height: 40
            // r41: the glyphs were empty boxes in the phone's font: words
            text: root.thumbnails ? qsTr("LIST") : qsTr("GRID")
            fontSize: 11
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

        Row {
            // FOLDERS | PLAYLISTS at the top level (Marko 14.10.2026); r41 (V70 phone): they ran over
            // LIST: they share the room left of the bar's buttons
            id: modeRow

            readonly property real room: viewButton.x - (backButton.x + backButton.width) - 8 - 6
            anchors.left: backButton.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            visible: root.folder === ""
            spacing: 4

            Repeater {
                model: [[false, qsTr("FOLDERS")], [true, qsTr("PLAYLISTS")]]

                delegate: DButton {
                    required property var modelData

                    objectName: modelData[0] ? "viewPlaylists" : "viewFolders"
                    width: Math.max(60, Math.min(92, (modeRow.room - 4) / 2))
                    height: 36
                    mode: ""
                    text: modelData[1]
                    fontSize: 11
                    lit: root.playlistsView === modelData[0]
                    onClicked: {
                        root.playlistsView = modelData[0];
                        Mixxx.DJMantra.setSetting("library_mode", modelData[0] ? "playlists" : "folders");
                        Toasts.show(modelData[0] ? qsTr("VIEW \u2192 PLAYLISTS") : qsTr("VIEW \u2192 FOLDERS"));
                    }
                }
            }
        }

        Text {
            anchors.left: backButton.right
            anchors.leftMargin: 12
            anchors.right: target.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            visible: root.folder !== ""
            text: root.inPlaylist ? root.folder.substring(9) : root.folder.substring(root.folder.lastIndexOf("/") + 1)
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
            anchors.right: sunoButton.left
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
                // r41: Gboard keeps the word being typed as "composing" text until space /
                // Enter; the filter follows it too, letter by letter
                onTextChanged: root.filterText = (text + preeditText).trim()
                onPreeditTextChanged: root.filterText = (text + preeditText).trim()
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
                text: "\u00D7" // r41: U+2715 was an empty box in the phone's font
                color: Style.subText
                font.pixelSize: 24

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6
                    onClicked: root.clearSearch()
                }
            }
        }

        DButton {
            // SUNO PLAYLISTS (Marko 14.10.2026): paste Suno links, the songs come in
            id: sunoButton

            objectName: "sunoPlaylists"
            anchors.right: tidalButton.left
            anchors.rightMargin: visible ? 6 : 0
            anchors.verticalCenter: parent.verticalCenter
            visible: root.playlistsRoot || root.inPlaylist
            width: visible ? 56 : 0
            height: 38
            mode: ""
            text: qsTr("SUNO")
            fontSize: 11
            onClicked: root.openSuno()
        }

        DButton {
            // TIDAL playlists both ways (Marko 14.10.2026), in the playlists view
            id: tidalButton

            objectName: "tidalPlaylists"
            anchors.right: phoneButton.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            visible: root.playlistsRoot || root.inPlaylist
            width: visible ? 60 : 0
            height: 38
            mode: ""
            text: qsTr("TIDAL")
            fontSize: 11
            onClicked: root.openTidal()
        }

        DButton {
            id: phoneButton

            objectName: "findOnPhone"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            // r41 (V70 phone): with SUNO and TIDAL beside it the filter had no room: two lines
            width: sunoButton.visible ? 74 : 118
            height: 38
            mode: ""
            text: qsTr("FIND ON PHONE")
            wrap: sunoButton.visible
            fontSize: sunoButton.visible ? 10 : 11
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
        visible: root.folder !== "" || root.phoneMode || root.playlistsRoot

        Repeater {
            // Marko 14.10.2026: also by LENGTH
            model: root.playlistsRoot
                   ? [{key: "title", label: qsTr("NAME"), w: 0.40}, {key: "created", label: qsTr("CREATED"), w: 0.24},
                      {key: "count", label: qsTr("SONGS"), w: 0.16}, {key: "duration", label: qsTr("LENGTH"), w: 0.20}]
                   : [{key: "title", label: qsTr("TITLE"), w: 0.32}, {key: "artist", label: qsTr("ARTIST"), w: 0.24},
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
            height: Math.max(0, list.height - list.count * 30)
            onPressed: barWasShown = Mixxx.DJMantra.barsVisible()
            onClicked: {
                if (!barWasShown) {
                    Mixxx.DJMantra.log("show the navigation bar");
                    Mixxx.DJMantra.showBars();
                }
            }
        }

        visible: !root.thumbnails

        // compact, as a macOS list view (Marko 14.10.2026: "even more compact ... without folder text
        // ... as much as possible text on the screen and without cutting ... half size"): one line per
        // row, 30 px; a folder's name has the whole row; a song's title, artist, length, BPM and key
        // sit under their column headers; a playlist row shows its date, songs and length
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
            required property real created
            required property int count
            readonly property bool wasPlayed: !isFolder && root.played[path] === true
            readonly property bool autoNow: root.automixMode && root.automixRunning && path === root.automixNow
            readonly property bool autoNext: root.automixMode && root.automixRunning && path === root.automixNext
            readonly property bool playlistRow: isFolder && path.startsWith("playlist:")
            readonly property real w: list.width

            required property int index

            width: list.width
            height: 30
            color: rowArea.pressed ? Style.pressed
                    : root.controllerSelect && index === list.currentIndex || autoNow ? Style.surface : "transparent"
            border.color: root.controllerSelect && index === list.currentIndex || autoNow ? Style.accent
                    : autoNext ? Style.line : "transparent"
            border.width: 1

            Rectangle {
                // the cover (small), a folder or a note while there is none
                id: art

                x: 6
                anchors.verticalCenter: parent.verticalCenter
                width: 22
                height: 22
                radius: 3
                color: row.isFolder ? "transparent" : Style.tile
                clip: true

                Image {
                    anchors.fill: parent
                    visible: !row.isFolder && row.cover !== ""
                    source: row.isFolder ? "" : row.cover
                    sourceSize.width: 44
                    sourceSize.height: 44
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }

                Text {
                    anchors.centerIn: parent
                    visible: row.isFolder || row.cover === ""
                    text: row.isFolder ? "\uD83D\uDCC1" : "\u266B"
                    color: row.isFolder ? Style.accent : Style.subText
                    font.pixelSize: row.isFolder ? 15 : 12
                }
            }

            Text {
                // the name: a folder's runs to the row's end; a playlist's and a song's to its column
                anchors.left: art.right
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: row.isFolder && !row.playlistRow ? row.w - x - 8
                       : row.playlistRow ? row.w * 0.40 - x - 4 : row.w * 0.32 - x - 4
                text: (row.autoNow ? qsTr("PLAYING  ") : row.autoNext ? qsTr("NEXT  ") : "")
                      + (row.wasPlayed ? "\u2713 " : "") + (row.isFolder ? row.name : row.title)
                color: row.autoNow ? Style.accent : row.wasPlayed ? Style.line : Style.text
                font.pixelSize: 14
                font.bold: row.isFolder
                elide: Text.ElideRight
            }

            Text {
                // the artist (songs)
                x: row.w * 0.32
                width: row.w * 0.24 - 4
                anchors.verticalCenter: parent.verticalCenter
                visible: !row.isFolder
                text: row.artist
                color: row.wasPlayed ? Style.line : Style.subText
                font.pixelSize: 13
                elide: Text.ElideRight
            }

            Row {
                // a playlist: its date, songs, length (NAME / CREATED / SONGS / LENGTH)
                x: row.w * 0.40
                anchors.verticalCenter: parent.verticalCenter
                visible: row.playlistRow

                Text {
                    width: row.w * 0.24
                    horizontalAlignment: Text.AlignHCenter
                    text: row.created > 0 ? new Date(row.created).toLocaleDateString(Qt.locale(), Locale.ShortFormat) : ""
                    color: Style.subText
                    font.pixelSize: 13
                }
                Text {
                    width: row.w * 0.16
                    horizontalAlignment: Text.AlignHCenter
                    text: row.count
                    color: Style.subText
                    font.pixelSize: 13
                }
                Text {
                    width: row.w * 0.20
                    horizontalAlignment: Text.AlignHCenter
                    text: root.lengthText(row.duration)
                    color: Style.subText
                    font.pixelSize: 13
                }
            }

            Row {
                // a song: LENGTH, BPM, KEY under their headers
                x: row.w * 0.56
                anchors.verticalCenter: parent.verticalCenter
                visible: !row.isFolder

                Text {
                    width: row.w * 0.16
                    horizontalAlignment: Text.AlignHCenter
                    text: root.lengthText(row.duration)
                    color: Style.subText
                    font.pixelSize: 13
                }
                Text {
                    width: row.w * 0.14
                    horizontalAlignment: Text.AlignHCenter
                    text: row.bpm > 0 ? row.bpm.toFixed(1) : ""
                    color: Style.text
                    font.pixelSize: 13
                }
                Text {
                    width: row.w * 0.14
                    horizontalAlignment: Text.AlignHCenter
                    text: row.key
                    color: Style.accent
                    font.pixelSize: 13
                    font.bold: true
                }
            }

            MouseArea {
                id: rowArea

                anchors.fill: parent
                pressAndHoldInterval: 1000
                // a long press on a song: add it to a playlist (Marko 14.10.2026)
                onPressAndHold: if (!row.isFolder) root.askPlaylist(row.path)
                onClicked: {
                    if (row.isFolder)
                        root.folder = row.path;
                    else
                        root.pick(row.path);
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                x: 6
                width: parent.width - 12
                height: 1
                color: Style.surface
            }
        }
    }

    // YouTube's length on the thumbnail: white on a dark rounded box, bottom right
    component LengthBadge: Rectangle {
        property alias text: label.text
        property int fontSize: 12
        // bottom left instead of bottom right (the thumbnails' BPM and key, Marko 14.10.2026)
        property bool onLeft: false

        anchors.right: onLeft ? undefined : parent.right
        anchors.left: onLeft ? parent.left : undefined
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

                LengthBadge {
                    // BPM and key bottom left, in the same box as the length (Marko 14.10.2026)
                    onLeft: true
                    anchors.margins: 4
                    text: tile.isFolder ? "" : ((tile.bpm > 0 ? tile.bpm.toFixed(0) : "")
                            + (tile.bpm > 0 && tile.key !== "" ? "  " : "") + tile.key)
                    fontSize: 12
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
                    text: (root.automixMode && root.automixRunning && tile.path === root.automixNow ? qsTr("PLAYING  ")
                          : root.automixMode && root.automixRunning && tile.path === root.automixNext ? qsTr("NEXT  ") : "")
                          + (tile.wasPlayed ? "\u2713 " : "") + (tile.isFolder ? tile.name : tile.title)
                    color: tile.wasPlayed ? Style.line : Style.text
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    width: parent.width
                    text: tile.isFolder ? (tile.artist !== "" ? tile.artist : qsTr("Folder")) : tile.artist
                    color: tile.wasPlayed ? Style.line : Style.subText
                    font.pixelSize: 11
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                id: tileArea

                anchors.fill: parent
                pressAndHoldInterval: 1000
                onPressAndHold: if (!tile.isFolder) root.askPlaylist(tile.path)
                onClicked: {
                    if (tile.isFolder)
                        root.folder = tile.path;
                    else
                        root.pick(tile.path);
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

    // ADD TO PLAYLIST (Marko 14.10.2026: "a long press on the song ... all the possible playlists
    // or create new playlist"); inside a playlist also REMOVE
    property string playlistSong: ""
    property var playlistChoices: []

    function askPlaylist(path) {
        playlistSong = path;
        playlistChoices = Mixxx.DJMantra.playlistRoots();
        newPlaylistName.text = "";
        Mixxx.DJMantra.log("playlist popup for " + path);
    }

    function addSong(name) {
        const added = Mixxx.DJMantra.addToPlaylist(name, playlistSong);
        Toasts.show((added ? qsTr("ADDED TO ") : qsTr("ALREADY IN ")) + name);
        playlistSong = "";
        ++playlistsVersion;
    }

    Rectangle {
        anchors.fill: parent
        visible: root.playlistSong !== ""
        color: "#b0000000"
        z: 50

        MouseArea {
            anchors.fill: parent
            onClicked: root.playlistSong = ""
        }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 32
            height: Math.min(parent.height - 32, popupColumn.implicitHeight + 24)
            radius: 12
            color: Style.surface
            clip: true

            MouseArea {
                anchors.fill: parent
            }

            Flickable {
                anchors.fill: parent
                anchors.margins: 12
                contentHeight: popupColumn.implicitHeight
                clip: true

                Column {
                    id: popupColumn

                    width: parent.width
                    spacing: 8

                    Text {
                        width: parent.width
                        text: qsTr("ADD TO PLAYLIST")
                        color: Style.accent
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Text {
                        width: parent.width
                        text: root.playlistSong.substring(root.playlistSong.lastIndexOf("/") + 1)
                        color: Style.subText
                        font.pixelSize: 13
                        elide: Text.ElideMiddle
                    }

                    Repeater {
                        model: root.playlistChoices

                        delegate: DButton {
                            required property var modelData

                            width: popupColumn.width
                            height: 46
                            mode: ""
                            text: modelData.name + "   (" + modelData.count + ")"
                            fontSize: 14
                            onClicked: root.addSong(modelData.name)
                        }
                    }

                    Row {
                        spacing: 6

                        Rectangle {
                            width: popupColumn.width - 6 - 90
                            height: 46
                            radius: Style.radius
                            color: Style.tile

                            TextInput {
                                id: newPlaylistName

                                objectName: "newPlaylist"
                                anchors.fill: parent
                                anchors.margins: 10
                                verticalAlignment: TextInput.AlignVCenter
                                color: Style.text
                                font.pixelSize: 15
                                clip: true
                                onAccepted: if (text.trim() !== "") root.addSong(text.trim())

                                Text {
                                    anchors.fill: parent
                                    visible: newPlaylistName.text === "" && !newPlaylistName.activeFocus
                                    text: qsTr("New playlist name")
                                    color: Style.line
                                    font.pixelSize: 15
                                    verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }

                        DButton {
                            width: 90
                            height: 46
                            mode: ""
                            text: qsTr("CREATE")
                            fontSize: 13
                            lit: newPlaylistName.text.trim() !== ""
                            onClicked: if (newPlaylistName.text.trim() !== "") root.addSong(newPlaylistName.text.trim())
                        }
                    }

                    DButton {
                        visible: root.inPlaylist
                        width: popupColumn.width
                        height: 46
                        mode: ""
                        text: qsTr("REMOVE FROM ") + root.folder.substring(9)
                        fontSize: 13
                        onClicked: {
                            Mixxx.DJMantra.removeFromPlaylist(root.folder.substring(9), root.playlistSong);
                            Toasts.show(qsTr("REMOVED FROM ") + root.folder.substring(9));
                            root.playlistSong = "";
                            ++root.playlistsVersion;
                        }
                    }

                    DButton {
                        width: popupColumn.width
                        height: 46
                        text: qsTr("CANCEL")
                        onClicked: root.playlistSong = ""
                    }
                }
            }
        }
    }

    // SUNO PLAYLISTS (Marko 14.10.2026: "the user will paste the links from Suno, and they will
    // be remembered in the app"): the links pasted go into a playlist (Suno by default, or the
    // one open), each song saved in djmantra_app/Suno and playable / mixable as any song
    property bool sunoOpen: false
    property bool sunoBusy: false
    property string sunoStatus: ""
    property var sunoKnown: []

    function openSuno() {
        sunoOpen = true;
        sunoStatus = "";
        sunoFind();
        sunoName.text = inPlaylist ? folder.substring(9) : qsTr("Suno");
    }

    // the songs waiting for their file: found by title in djmantra_app/Suno and Download
    function sunoFind() {
        const n = Mixxx.DJMantra.sunoScan();
        if (n > 0) {
            sunoStatus = n + qsTr(" Suno songs came in");
            Toasts.show(qsTr("SUNO: ") + n + qsTr(" SONGS IN"));
            ++playlistsVersion;
        }
        sunoKnown = Mixxx.DJMantra.sunoLinks();
    }

    // back from GET (the Suno browser): its download is matched at once
    Connections {
        target: Qt.application
        function onStateChanged() {
            if (root.sunoOpen && Qt.application.state === Qt.ApplicationActive)
                root.sunoFind();
        }
    }

    function addSuno() {
        const n = Mixxx.DJMantra.sunoAdd(sunoName.text, sunoPaste.text);
        if (n === 0) {
            sunoStatus = qsTr("No Suno links found: paste links like https://suno.com/song/...");
            return;
        }
        sunoBusy = true;
        sunoStatus = n + qsTr(" links remembered, saving the songs ...");
        sunoPaste.text = "";
        sunoKnown = Mixxx.DJMantra.sunoLinks();
    }

    Connections {
        target: Mixxx.DJMantra
        function onSunoResult(action, result) {
            if (action === "song") {
                root.sunoStatus = qsTr("Saved: ") + result;
            } else if (action === "title") {
                root.sunoStatus = qsTr("Remembered: ") + result + qsTr(" (GET it with your Suno login)");
            } else if (action === "error") {
                root.sunoStatus = result;
            } else if (action === "done") {
                const p = result.split("|");
                root.sunoBusy = false;
                root.sunoStatus = p[0] + qsTr(" songs in ") + sunoName.text
                        + (p[1] !== "0" ? ", " + p[1] + qsTr(" could not be read") : "")
                        + (p[2] && p[2] !== "0" ? ", " + p[2] + qsTr(" to GET with your Suno login (Suno keeps the audio for its owner)") : "");
                Toasts.show(qsTr("SUNO: ") + p[0] + qsTr(" SONGS IN ") + sunoName.text.toUpperCase());
            }
            root.sunoKnown = Mixxx.DJMantra.sunoLinks();
            ++root.playlistsVersion;
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.sunoOpen
        color: Style.background
        z: 60

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: sunoColumn

            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Item {
                width: parent.width
                height: 44

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("SUNO PLAYLIST")
                    color: Style.accent
                    font.pixelSize: 18
                    font.bold: true
                }

                CloseButton {
                    objectName: "sunoClose"
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: {
                        root.sunoOpen = false;
                        ++root.playlistsVersion;
                    }
                }
            }

            Rectangle {
                // the playlist the songs go into
                width: parent.width
                height: 40
                radius: Style.radius
                color: Style.tile

                TextInput {
                    id: sunoName

                    objectName: "sunoName"
                    anchors.fill: parent
                    anchors.margins: 10
                    verticalAlignment: TextInput.AlignVCenter
                    color: Style.text
                    font.pixelSize: 16
                    clip: true
                }
            }

            Rectangle {
                // the links: paste one or many (any text; every Suno link in it is taken)
                width: parent.width
                height: 110
                radius: Style.radius
                color: Style.tile
                border.color: sunoPaste.activeFocus ? Style.accent : Style.surface

                Flickable {
                    anchors.fill: parent
                    anchors.margins: 8
                    contentHeight: sunoPaste.implicitHeight
                    clip: true

                    TextEdit {
                        id: sunoPaste

                        objectName: "sunoPaste"
                        width: parent.width
                        color: Style.text
                        font.pixelSize: 14
                        wrapMode: TextEdit.WrapAnywhere
                        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly

                        Text {
                            anchors.fill: parent
                            visible: sunoPaste.text === "" && !sunoPaste.activeFocus
                            text: qsTr("Paste Suno links here (https://suno.com/song/...)")
                            color: Style.line
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            Row {
                spacing: 6

                DButton {
                    objectName: "sunoPasteButton"
                    width: (sunoColumn.width - 6) / 2
                    height: 46
                    mode: ""
                    text: qsTr("PASTE")
                    onClicked: {
                        sunoPaste.selectAll();
                        sunoPaste.paste();
                    }
                }

                DButton {
                    objectName: "sunoAdd"
                    width: (sunoColumn.width - 6) / 2
                    height: 46
                    text: qsTr("ADD TO ") + sunoName.text.toUpperCase()
                    fontSize: 12
                    lit: true
                    enabled: !root.sunoBusy
                    onClicked: root.addSuno()
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: (root.sunoBusy ? "... " : "") + root.sunoStatus
                color: Style.text
                font.pixelSize: 14
            }

            Text {
                text: root.sunoKnown.length + qsTr(" SUNO LINKS REMEMBERED")
                color: Style.subText
                font.pixelSize: 12
                font.bold: true
            }

            ListView {
                width: parent.width
                height: Math.max(0, sunoColumn.height - y)
                clip: true
                model: root.sunoKnown

                delegate: Item {
                    id: sunoRow

                    required property var modelData

                    width: ListView.view.width
                    height: 44

                    Text {
                        anchors.left: parent.left
                        anchors.right: getButton.left
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: (sunoRow.modelData.title ? sunoRow.modelData.title : sunoRow.modelData.url)
                              + "   \u00B7  " + sunoRow.modelData.playlist
                        color: sunoRow.modelData.file ? Style.text : Style.subText
                        font.pixelSize: 13
                    }

                    DButton {
                        // no file yet: Suno's own download, logged in (SunoActivity)
                        id: getButton

                        objectName: "sunoGet"
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !sunoRow.modelData.file
                        width: visible ? 64 : 0
                        height: 38
                        mode: ""
                        text: qsTr("GET")
                        fontSize: 11
                        onClicked: Mixxx.DJMantra.sunoGet(sunoRow.modelData.url, sunoRow.modelData.title || "")
                    }
                }
            }
        }
    }

    // TIDAL (Marko 14.10.2026): log in, take a TIDAL playlist in (its songs found on the phone,
    // the rest into the TIDAL download queue), send one of the app's playlists to TIDAL
    property bool tidalOpen: false
    property bool tidalBusy: false
    property string tidalStatus: ""
    property var tidalLists: []
    property bool tidalOn: false // logged in (asked again after each TIDAL answer)

    function openTidal() {
        tidalOpen = true;
        tidalStatus = "";
        tidalOn = Mixxx.DJMantra.tidalLoggedIn();
        if (Mixxx.DJMantra.tidalLoggedIn()) {
            tidalBusy = true;
            tidalStatus = qsTr("Reading your TIDAL playlists ...");
            Mixxx.DJMantra.tidalPlaylists();
        }
    }

    Connections {
        target: Mixxx.DJMantra
        function onTidalResult(action, result) {
            root.tidalBusy = false;
            root.tidalOn = Mixxx.DJMantra.tidalLoggedIn();
            if (action === "login") {
                root.tidalStatus = result === "" ? qsTr("Connected to TIDAL") : qsTr("Login: ") + result;
                if (result === "")
                    root.openTidal();
            } else if (action === "playlists") {
                root.tidalLists = JSON.parse(result || "[]");
                root.tidalStatus = root.tidalLists.length + qsTr(" TIDAL playlists");
            } else if (action === "import") {
                const p = result.split("|");
                root.tidalStatus = p[0] + ": " + p[1] + qsTr(" songs on the phone, ") + p[2] + qsTr(" into the TIDAL download queue");
                Toasts.show(qsTr("IMPORTED ") + p[0]);
                ++root.playlistsVersion;
            } else if (action === "push") {
                const p = result.split("|");
                root.tidalStatus = p[1].indexOf("/") > 0 ? p[0] + qsTr(" on TIDAL: ") + p[1] + qsTr(" songs found") : p[1];
                Toasts.show(qsTr("SENT TO TIDAL: ") + p[0]);
            } else if (action === "sync") {
                root.tidalStatus = result + qsTr(" playlists synced to TIDAL");
                Toasts.show(qsTr("TIDAL SYNC: ") + result + qsTr(" PLAYLISTS"));
                Mixxx.DJMantra.tidalPlaylists();
            } else if (action === "logout") {
                root.tidalLists = [];
                root.tidalStatus = qsTr("Logged out");
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.tidalOpen
        color: Style.background
        z: 60

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: tidalHead

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
                    text: qsTr("TIDAL PLAYLISTS")
                    color: Style.accent
                    font.pixelSize: 18
                    font.bold: true
                }

                CloseButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.tidalOpen = false
                }
            }

            DButton {
                visible: !root.tidalOn
                width: parent.width
                height: 48
                text: qsTr("LOG IN TO TIDAL (opens the browser)")
                lit: true
                onClicked: {
                    root.tidalBusy = true;
                    root.tidalStatus = qsTr("Log in in the browser, then come back ...");
                    Mixxx.DJMantra.tidalLogin();
                }
            }

            DButton {
                visible: root.inPlaylist && root.tidalOn
                width: parent.width
                height: 48
                text: qsTr("SEND \"") + root.folder.substring(9) + qsTr("\" TO TIDAL")
                lit: true
                onClicked: {
                    root.tidalBusy = true;
                    root.tidalStatus = qsTr("Finding the songs on TIDAL ...");
                    Mixxx.DJMantra.tidalPush(root.folder.substring(9));
                }
            }

            DButton {
                // every app playlist to TIDAL (Marko 14.10.2026: "Every playlist from the
                // application goes to Tidal"); a TIDAL playlist of the same name gets only what it lacks
                objectName: "tidalSyncAll"
                visible: root.tidalOn
                width: parent.width
                height: 48
                text: qsTr("SYNC ALL PLAYLISTS TO TIDAL")
                onClicked: {
                    root.tidalBusy = true;
                    root.tidalStatus = qsTr("Sending every playlist to TIDAL ...");
                    Mixxx.DJMantra.tidalSyncAll();
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: (root.tidalBusy ? "... " : "") + root.tidalStatus
                color: Style.text
                font.pixelSize: 13
            }
        }

        ListView {
            anchors.top: tidalHead.bottom
            anchors.topMargin: 8
            anchors.bottom: tidalFoot.top
            anchors.left: parent.left
            anchors.right: parent.right
            clip: true
            model: root.tidalLists

            delegate: Rectangle {
                id: tlist

                required property var modelData

                width: ListView.view.width
                height: 52
                color: "transparent"

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.right: importButton.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: tlist.modelData.name + "   (" + tlist.modelData.count + ")"
                    color: Style.text
                    font.pixelSize: 14
                    elide: Text.ElideRight
                }

                DButton {
                    id: importButton

                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    width: 96
                    height: 40
                    text: qsTr("TAKE IN")
                    fontSize: 12
                    onClicked: {
                        root.tidalBusy = true;
                        root.tidalStatus = qsTr("Taking in ") + tlist.modelData.name + " ...";
                        Mixxx.DJMantra.tidalImport(tlist.modelData.id, tlist.modelData.name);
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

        DButton {
            id: tidalFoot

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 12
            visible: root.tidalOn
            height: visible ? 44 : 0
            text: qsTr("LOG OUT OF TIDAL")
            fontSize: 12
            onClicked: Mixxx.DJMantra.tidalLogout()
        }
    }
}
