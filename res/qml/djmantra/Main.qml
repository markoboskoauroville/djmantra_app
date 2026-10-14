import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12
import QtQuick.Window 2.12

// DJ Mantra's interface in Qt Quick: everything is drawn by the GPU (Marko,
// 12.10.2026). The redesign (Marko, "let's redesign this so we don't have
// redundancy"), top to bottom: the top row (menu button left, Camelot badge
// right, the middle free for the selfie camera hole); the deck headers (art,
// artist / title / key, total and remaining time); the player controls (SYNC,
// CUE, PLAY, JUMP per deck) and the mixing modes (AUTOMIX, EQ, TEMPO BLEND,
// 8 / 16 / 32 BEATS); the matrix display, the only phrase lights; the overview
// waveforms; the rest: the playlist (or the mixer, or the keys), the
// crossfader and the tabs. Landscape: the decks left, the rest right.
// The system bars and a camera hole off the middle are kept clear with the
// insets from Mixxx.DJMantra.
Window {
    id: window

    visible: true
    width: 413
    height: 919
    color: Style.background
    title: "DJ Mantra"

    property bool libraryFull: false // the song list over the decks (LibraryPage FULL / DOCK)
    property int page: 0 // 0 songs (the playlist), 1 phrasing, 2 mixer, 3 automix, 4 keys (Camelot wheel, key match)
    readonly property bool landscape: width > height
    property string overlay: "" // "", "menu"
    property string libraryDeck: ""
    // the song list in front (the controller's BROWSE, a cover tap in
    // landscape); the tab to go back to when a song is loaded
    property bool browsing: false
    property int pageBefore: 0

    function openBrowser(deckGroup) {
        if (!browsing) {
            pageBefore = page;
            browsing = true;
        }
        if (deckGroup !== undefined)
            libraryDeck = deckGroup;
        page = 0;
        Mixxx.DJMantra.log("browse open" + (libraryDeck !== "" ? " for " + libraryDeck : ""));
    }

    function closeBrowser() {
        if (!browsing)
            return;
        browsing = false;
        page = pageBefore;
        Mixxx.DJMantra.log("browse closed");
    }

    // The Mix Ultra (Marko, 12.10.2026): BROWSE brings the song list in front,
    // the pad mode buttons choose the tabs (res/controllers/...Mix-Ultra-script.js)
    Mixxx.ControlProxy {
        id: controllerPage

        group: "[DJMantra]"; key: "page"
        // 1 Songs, 2 Phrasing, 3 Mixer, 4 AutoMix, 5 Keys (the pad mode buttons' order)
        onValueChanged: if (value >= 1 && value <= 5) {
            Mixxx.DJMantra.log("controller: page " + value);
            window.browsing = false;
            window.page = value - 1;
            // back to 0 here (r41): the next press of the same button changes it again
            controllerPage.value = 0;
        }
    }
    // the Mix Ultra's BROWSE, LOAD and its PLAY with a stop time: each value stays until it
    // is handled here and set back to 0 (r41: a set-and-reset pulse was read as 0)
    Mixxx.ControlProxy {
        id: browseMove

        group: "[DJMantra]"; key: "browse_move"
        onValueChanged: if (value !== 0) {
            const steps = Math.round(value);
            value = 0;
            Mixxx.DJMantra.log("controller: browse " + steps);
            if (!window.browsing)
                window.openBrowser();
            else
                playlistPage.moveSelection(steps);
        }
    }
    Mixxx.ControlProxy {
        id: browsePress

        group: "[DJMantra]"; key: "browse_press"
        onValueChanged: if (value > 0) {
            value = 0;
            Mixxx.DJMantra.log("controller: browse press");
            if (!window.browsing)
                window.openBrowser();
            else
                playlistPage.activate("");
        }
    }
    TurntableStop { id: turntable1; group: "[Channel1]" }
    TurntableStop { id: turntable2; group: "[Channel2]" }
    Mixxx.ControlProxy {
        // the controller's PLAY on a playing deck with a stop time (Mix Ultra script)
        group: "[DJMantra]"; key: "stop_deck"
        onValueChanged: if (value >= 1) {
            const t = Math.round(value) === 2 ? turntable2 : turntable1;
            value = 0;
            if (t.braking)
                t.cancel();
            else
                t.stop();
        }
    }
    Mixxx.ControlProxy {
        group: "[DJMantra]"; key: "browse_back"
        onValueChanged: if (value > 0) {
            value = 0;
            if (window.browsing)
                playlistPage.back();
        }
    }
    Mixxx.ControlProxy {
        group: "[DJMantra]"; key: "load"
        onValueChanged: if (value >= 1) {
            const group = "[Channel" + Math.round(value) + "]";
            value = 0;
            Mixxx.DJMantra.log("controller: load " + group);
            if (!window.browsing)
                window.openBrowser(group);
            else
                playlistPage.activate(group);
        }
    }
    // the songs played in this session (path -> true): dimmed in the list,
    // skipped by NEXT
    property var playedSongs: ({})
    function markPlayed(path) {
        if (path === "" || playedSongs[path])
            return;
        const next = Object.assign({}, playedSongs);
        next[path] = true;
        playedSongs = next;
        Mixxx.DJMantra.log("played in this set: " + path);
    }

    function logMap() {
        // the phone tests tap by these (device pixels)
        const ratio = Screen.devicePixelRatio;
        const walk = (item) => {
            if (item.objectName !== "" && item.visible) {
                const p = item.mapToItem(null, 0, 0);
                Mixxx.DJMantra.log("map " + item.objectName + " " + Math.round(p.x * ratio) + " " +
                        Math.round(p.y * ratio) + " " + Math.round(item.width * ratio) + " " +
                        Math.round(item.height * ratio));
            }
            for (let i = 0; i < item.children.length; ++i)
                walk(item.children[i]);
        };
        walk(content);
    }

    Item {
        id: content

        anchors.fill: parent
        // clear of the bars, the camera hole and the screen's rounded corners
        // (round 19: the top right meter was cut by the corner, the app
        // reached the very edge)
        readonly property int corner: Mixxx.DJMantra.cornerRadius
        anchors.leftMargin: Math.max(Mixxx.DJMantra.insetLeft, Math.round(corner * 0.3))
        anchors.topMargin: Mixxx.DJMantra.insetTop + (corner > 0 ? 4 : 0)
        anchors.rightMargin: Math.max(Mixxx.DJMantra.insetRight, Math.round(corner * 0.3))
        anchors.bottomMargin: Math.max(Mixxx.DJMantra.insetBottom, Math.round(corner * 0.2))
        focus: true

        Keys.onBackPressed: (event) => {
            // Android's back: closes what is open, never the app
            event.accepted = true;
            if (window.overlay !== "")
                window.overlay = "";
            else if (window.browsing) {
                window.libraryDeck = "";
                window.closeBrowser();
            }
        }

        MouseArea {
            // a tap on the empty space (no button there) brings Android's
            // navigation bar back; the next touch anywhere hides it (round 18,
            // Marko). A tap on the empty space while the bar is there only
            // hides it (the press did), it does not bring it back.
            property bool barWasShown: false

            anchors.fill: parent
            onPressed: barWasShown = Mixxx.DJMantra.barsVisible()
            onClicked: {
                if (barWasShown)
                    return;
                Mixxx.DJMantra.log("show the navigation bar");
                Mixxx.DJMantra.showBars();
            }
        }

        Item {
            id: topBar

            width: parent.width
            height: 44

            Rectangle {
                // the menu (Library, Camelot wheel, settings): a drawn menu
                // icon, three lines on a navy tile, yellow while it is open
                // (Marko, 12.10.2026: no "dj" logo in the interface)
                id: menuButton

                readonly property bool open: window.overlay === "menu"

                objectName: "menuButton"
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 36
                height: 36
                radius: 8
                color: menuArea.pressed ? Style.pressed : Style.tile
                border.color: open ? Style.accent : Style.line
                border.width: 1

                Column {
                    anchors.centerIn: parent
                    spacing: 4

                    Repeater {
                        model: 3

                        Rectangle {
                            width: 18
                            height: 2.5
                            radius: 1.25
                            color: menuButton.open ? Style.accent : Style.text
                        }
                    }
                }

                Rectangle {
                    // the controller's link on the menu button's corner (Marko 14.10.2026):
                    // USB / BT in green while the Mix Ultra is connected, nothing when not
                    objectName: "controllerLink"
                    x: parent.width - width + 8
                    y: -4
                    z: 2
                    visible: controllerLink.value > 0
                    width: linkText.implicitWidth + 6
                    height: 14
                    radius: 4
                    color: Style.earth
                    border.color: Style.background
                    border.width: 1

                    Text {
                        id: linkText

                        anchors.centerIn: parent
                        text: controllerLink.value === 2 ? "BT" : "USB"
                        color: Style.inkOnAccent
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Mixxx.ControlProxy {
                        id: controllerLink

                        group: "[DJMantra]"
                        key: "controller_link"
                        property int last: 0
                        onValueChanged: {
                            const v = Math.round(value);
                            if (v === last)
                                return;
                            Toasts.show(v === 0 ? qsTr("CONTROLLER DISCONNECTED")
                                    : qsTr("CONTROLLER CONNECTED \u00B7 ") + (v === 2 ? "BLUETOOTH" : "USB"));
                            Mixxx.DJMantra.log("controller link: " + (v === 0 ? "none" : v === 2 ? "bluetooth" : "usb"));
                            last = v;
                        }
                    }
                }

                MouseArea {
                    id: menuArea

                    anchors.fill: parent
                    onClicked: {
                        Mixxx.DJMantra.log("menu");
                        window.overlay = window.overlay === "menu" ? "" : "menu";
                    }
                }
            }

            KeyBadge {
                // left of the camera hole, next to the menu (the usage meter
                // takes the top right corner)
                objectName: "keyBadge"
                anchors.left: menuButton.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                onWheelRequested: window.page = 4
            }

            ClockDisplay {
                // the time of day, left of the usage meter (Marko, 12.10.2026);
                // the middle of the bar is the camera hole
                objectName: "clock"
                anchors.right: battery.left
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                // two rows while recording: the clock and the recording's time
                height: Recorder.recording ? 40 : 24
                width: implicitWidth
            }

            Rectangle {
                // the phone's battery (Marko 14.10.2026): a thin vertical line between the
                // clock and the CPU / GPU meter, filled from the bottom, green; red at 10 % or less
                id: battery

                property int percent: -1

                objectName: "battery"
                anchors.right: usageMeter.left
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                width: percent >= 0 ? 5 : 0
                height: 24
                radius: 1
                color: "#121735"

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: Math.max(1, parent.height * battery.percent / 100)
                    radius: 1
                    color: battery.percent <= 10 ? Style.red : Style.earth
                }

                Timer {
                    interval: 60000
                    repeat: true
                    running: true
                    triggeredOnStart: true
                    onTriggered: battery.percent = Mixxx.DJMantra.batteryPercent()
                }
            }

            UsageMeter {
                // CPU and GPU use, top right (Marko, 12.10.2026)
                id: usageMeter

                objectName: "usage"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                height: 24
                width: implicitWidth
            }
        }

        Column {
            // the decks: headers, controls, modes, matrix, overviews
            id: decks

            anchors.top: topBar.bottom
            anchors.left: parent.left
            anchors.leftMargin: Style.gap
            // landscape (Marko, 12.10.2026): only the decks, over the whole
            // width; the song list comes in front when browsing
            width: parent.width - 2 * Style.gap
            spacing: Style.gap
            // landscape: the height left for the matrix and the overviews
            readonly property real room: parent.height - topBar.height - Style.gap * 7 - 56 - 44 - 38

            Row {
                id: headers

                width: parent.width
                spacing: 14

                DeckHeader {
                    objectName: "deck1"
                    width: (headers.width - headers.spacing) / 2
                    height: 56
                    group: "[Channel1]"
                    deckColor: Style.deck1
                    onCoverTapped: {
                        if (window.landscape)
                            window.openBrowser("[Channel1]");
                        else {
                            window.libraryDeck = "[Channel1]";
                            window.page = 0;
                        }
                    }
                }

                DeckHeader {
                    objectName: "deck2"
                    width: (headers.width - headers.spacing) / 2
                    height: 56
                    group: "[Channel2]"
                    mirrored: true
                    deckColor: Style.deck2
                    onCoverTapped: {
                        if (window.landscape)
                            window.openBrowser("[Channel2]");
                        else {
                            window.libraryDeck = "[Channel2]";
                            window.page = 0;
                        }
                    }
                }
            }

            Row {
                id: controls

                width: parent.width
                spacing: 10

                DeckControls {
                    width: (controls.width - controls.spacing) / 2
                    height: 44
                    group: "[Channel1]"
                    deckColor: Style.deck1
                    matrixDeck: matrix1
                    turntable: turntable1
                    playlist: playlistPage
                }

                DeckControls {
                    width: (controls.width - controls.spacing) / 2
                    height: 44
                    group: "[Channel2]"
                    deckColor: Style.deck2
                    mirrored: true
                    matrixDeck: matrix2
                    turntable: turntable2
                    playlist: playlistPage
                }
            }

            AutoMix {
                width: parent.width
                height: 38
                phase: autoPhase
                // the LED map changes with the beat count
                onBeatsChanged: mapTimer.restart()
            }

            MatrixDisplay {
                // the OXI E16's dot-matrix display: the only phrase lights
                id: matrix

                objectName: "matrix"
                width: parent.width
                height: window.landscape ? Math.max(84, decks.room * 0.5) : 84
                decks: [matrix1.info, matrix2.info]
                onCellTapped: (deck, cell) => (deck === 0 ? matrix1 : matrix2).tap(cell)
            }

            Overview {
                id: overview1

                objectName: "overview1"
                width: parent.width
                height: window.landscape ? Math.max(26, decks.room * 0.25) : 26
                group: "[Channel1]"
                deckColor: Style.deck1
            }

            Overview {
                id: overview2

                objectName: "overview2"
                width: parent.width
                height: window.landscape ? Math.max(26, decks.room * 0.25) : 26
                group: "[Channel2]"
                deckColor: Style.deck2
            }
        }

        MatrixDeck {
            id: matrix1

            group: "[Channel1]"
            deckColor: Style.deck1
            onStarted: (path) => window.markPlayed(path)
        }
        AutoPhase { id: autoPhase; deck1: matrix1; deck2: matrix2; hold: overview1.scrubbing || overview2.scrubbing }
        AutoCue { id: autoCueMode }
        MatrixDeck {
            id: matrix2

            group: "[Channel2]"
            deckColor: Style.deck2
            onStarted: (path) => window.markPlayed(path)
        }

        Item {
            // the rest: the playlist, the mixer or the keys; the crossfader; the tabs
            id: rest

            // the song list FULL screen (Marko 14.10.2026): over the decks, under the top bar
            readonly property bool libraryFull: window.libraryFull && (window.page === 0 || window.page === 3)
            anchors.top: window.landscape || libraryFull ? topBar.bottom : decks.bottom
            anchors.topMargin: Style.gap
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            // landscape: only while browsing, in front of the decks
            visible: !window.landscape || window.browsing
            z: window.landscape || libraryFull ? 5 : 0

            Rectangle {
                anchors.fill: parent
                anchors.topMargin: -Style.gap
                visible: window.landscape || rest.libraryFull
                color: Style.background
            }

            Item {
                objectName: "page"
                anchors.top: parent.top
                anchors.bottom: window.landscape ? parent.bottom : crossfader.top
                anchors.bottomMargin: Style.gap
                anchors.left: parent.left
                anchors.right: parent.right

                LibraryPage {
                    id: playlistPage

                    objectName: "playlist"
                    played: window.playedSongs
                    closable: window.browsing
                    anchors.fill: parent
                    // the AutoMix tab is the same song list with the AUTOMIX dropdown (Marko 14.10.2026)
                    visible: window.page === 0 || window.page === 3
                    automixMode: window.page === 3
                    automixRunning: automixPage.running
                    automixNow: automixPage.nowPath
                    automixNext: automixPage.nextPath
                    onAutomixMenu: automixPage.open = !automixPage.open
                    onAutomixPick: (path) => automixPage.pick(path)
                    deck: window.libraryDeck
                    fullScreen: window.libraryFull
                    onToggleFullScreen: {
                        window.libraryFull = !window.libraryFull;
                        Toasts.show(window.libraryFull ? qsTr("SONGS \u2192 FULL SCREEN") : qsTr("SONGS \u2192 DOCKED"));
                    }
                    onDone: {
                        window.libraryDeck = ""; // loaded: the next song to the free deck
                        window.closeBrowser();
                    }
                    onClosed: {
                        window.libraryDeck = "";
                        window.closeBrowser();
                    }
                }

                // AutoMix (Marko 13.10.2026): always made, so the mix goes on on every tab
                AutoMixPage {
                    id: automixPage

                    objectName: "automix"
                    library: playlistPage
                    anchors.fill: parent
                    visible: window.page === 3
                }

                // round 22: ~80 % CPU on every tab: a hidden tab costs nothing
                // now, it is only made while it is shown
                Loader {
                    anchors.fill: parent
                    active: window.page === 1 && rest.visible
                    visible: active

                    sourceComponent: PhrasingPage {
                        objectName: "phrasing"
                        deck1: matrix1
                        deck2: matrix2
                    }
                }

                Loader {
                    anchors.fill: parent
                    active: window.page === 2 && rest.visible
                    visible: active

                    sourceComponent: MixerPage {}
                }

                Loader {
                    anchors.fill: parent
                    active: window.page === 4 && rest.visible
                    visible: active

                    sourceComponent: Item {

                    WheelPage {
                        anchors.top: parent.top
                        anchors.bottom: keyTools.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        embedded: true
                    }

                    Column {
                        id: keyTools

                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: Style.gap
                        spacing: Style.gap

                        KeyTools {
                            width: parent.width
                            height: 50
                            group: "[Channel1]"
                            deckColor: Style.deck1
                        }

                        KeyTools {
                            width: parent.width
                            height: 50
                            group: "[Channel2]"
                            deckColor: Style.deck2
                        }
                    }
                    }
                }
            }

            DFader {
                id: crossfader

                objectName: "crossfader"
                visible: !window.landscape
                anchors.bottom: nav.top
                anchors.bottomMargin: Style.gap
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Style.gap
                anchors.rightMargin: Style.gap
                height: 40
                vertical: false
                group: "[Master]"
                key: "crossfader"
            }

            NavBar {
                id: nav

                visible: !window.landscape
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                current: window.page
                onSelected: (index) => window.page = index
            }
        }

        Loader {
            // below the top bar: the camera hole stays clear (round 13: the
            // wheel's title ran under it)
            anchors.top: topBar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            active: window.overlay === "menu" || window.overlay === "settings" || window.overlay === "queues"
                    || window.overlay === "beatstats"
            sourceComponent: window.overlay === "settings" ? settingsComponent
                           : window.overlay === "queues" ? queuesComponent
                           : window.overlay === "beatstats" ? beatStatsComponent : menuComponent
        }
    }

    Component {
        id: menuComponent

        MenuSheet {
            onLibrary: {
                window.libraryDeck = "";
                window.page = 0;
                window.overlay = "";
            }
            onSettings: window.overlay = "settings"
            onQueues: window.overlay = "queues"
            onBeatStats: window.overlay = "beatstats"
            onWheel: {
                window.page = 4;
                window.overlay = "";
            }
            onDone: window.overlay = ""
        }
    }

    Component {
        id: beatStatsComponent

        BeatStatsPage {
            onDone: window.overlay = ""
        }
    }

    Component {
        id: queuesComponent

        DownloadQueuePage {
            onDone: window.overlay = ""
        }
    }

    Component {
        id: settingsComponent

        SettingsPage {
            autoCue: autoCueMode
            onDone: window.overlay = ""
        }
    }

    Timer {
        // the map for the phone tests, after the layout settled and after each change
        id: mapTimer

        interval: 1500
        running: true
        onTriggered: window.logMap()
    }

    Timer {
        // round 24: the map logged 1.5 s after the start was ~100 px off (the
        // bars were still going away): once more when the start has settled
        interval: 6000
        running: true
        onTriggered: window.logMap()
    }

    Connections {
        // the tap map again when the bars come or go (round 20)
        target: Mixxx.DJMantra
        function onInsetsChanged() { mapTimer.restart(); }
    }

    onWidthChanged: mapTimer.restart()
    onHeightChanged: mapTimer.restart()
    onPageChanged: mapTimer.restart()
    onLandscapeChanged: mapTimer.restart()

    Rectangle {
        // the toast (Toasts.show): what a long press / double tap did, above the finger
        id: toast

        objectName: "toast"
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(parent.height * 0.12)
        z: 1000
        width: Math.min(parent.width - 32, toastText.implicitWidth + 32)
        height: toastText.implicitHeight + 18
        radius: height / 2
        color: "#E6000000"
        border.color: Style.accent
        border.width: 1
        opacity: 0
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: 180 } }

        Text {
            id: toastText

            anchors.centerIn: parent
            width: Math.min(implicitWidth, toast.parent.width - 64)
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            color: Style.text
            font.pixelSize: 17
            font.bold: true
        }

        Timer {
            id: toastTimer

            interval: 1400
            onTriggered: toast.opacity = 0
        }

        Connections {
            target: Toasts
            function onShown(text) {
                toastText.text = text;
                toast.opacity = 1;
                toastTimer.restart();
                Mixxx.DJMantra.log("toast: " + text);
            }
        }
    }

    Component.onCompleted: {
        // the decks' FILTER = the Pioneer DJM filter (also for the controller's filter knob)
        Mixxx.DJMantra.useDjmFilter();
        // every song normalized to a 0 dBFS peak, always (Marko 14.10.2026: GAIN "can only be reduced
        // because it's normalized anyway"); no switch any more
        Mixxx.DJMantra.setAutoGain(true);
        // the EQ starts flat (0 dB = value 1): Mixxx keeps the last values, and a
        // bug before V34 left them at the maximum (r40: HIGH / MID / LOW at max at start)
        for (const deck of ["[Channel1]", "[Channel2]"]) {
            for (const key of ["parameter1", "parameter2", "parameter3"])
                Mixxx.DJMantra.setControl("[EqualizerRack1_" + deck + "_Effect1]", key, 1);
            // GAIN at 0 dB, its top (V60: cut only, never above the song's normalized peak)
            Mixxx.DJMantra.setControl(deck, "pregain", 1);
        }
        Mixxx.DJMantra.log("interface ready " + width + "x" + height);
        Mixxx.DJMantra.hideBars();
    }
}
