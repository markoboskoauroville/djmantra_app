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

    property int page: 0 // 0 songs (the playlist), 1 phrasing, 2 mixer, 3 keys (Camelot wheel, key match)
    readonly property bool landscape: width > height
    property string overlay: "" // "", "menu"
    property string libraryDeck: ""

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
            window.overlay = "";
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
                onWheelRequested: window.page = 3
            }

            UsageMeter {
                // CPU and GPU use, top right (Marko, 12.10.2026)
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
            width: window.landscape ? parent.width * 0.55 - 2 * Style.gap : parent.width - 2 * Style.gap
            spacing: Style.gap

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
                        window.libraryDeck = "[Channel1]";
                        window.page = 0;
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
                        window.libraryDeck = "[Channel2]";
                        window.page = 0;
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
                }

                DeckControls {
                    width: (controls.width - controls.spacing) / 2
                    height: 44
                    group: "[Channel2]"
                    deckColor: Style.deck2
                    mirrored: true
                    matrixDeck: matrix2
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
                height: 84
                decks: [matrix1.info, matrix2.info]
                onCellTapped: (deck, cell) => (deck === 0 ? matrix1 : matrix2).tap(cell)
            }

            Overview {
                objectName: "overview1"
                width: parent.width
                height: 26
                group: "[Channel1]"
                deckColor: Style.deck1
            }

            Overview {
                objectName: "overview2"
                width: parent.width
                height: 26
                group: "[Channel2]"
                deckColor: Style.deck2
            }
        }

        MatrixDeck { id: matrix1; group: "[Channel1]"; deckColor: Style.deck1 }
        AutoPhase { id: autoPhase; deck1: matrix1; deck2: matrix2 }
        MatrixDeck { id: matrix2; group: "[Channel2]"; deckColor: Style.deck2 }

        Item {
            // the rest: the playlist, the mixer or the keys; the crossfader; the tabs
            id: rest

            anchors.top: window.landscape ? topBar.bottom : decks.bottom
            anchors.topMargin: Style.gap
            anchors.bottom: parent.bottom
            anchors.left: window.landscape ? decks.right : parent.left
            anchors.leftMargin: window.landscape ? Style.gap : 0
            anchors.right: parent.right

            Item {
                objectName: "page"
                anchors.top: parent.top
                anchors.bottom: crossfader.top
                anchors.bottomMargin: Style.gap
                anchors.left: parent.left
                anchors.right: parent.right

                LibraryPage {
                    objectName: "playlist"
                    anchors.fill: parent
                    visible: window.page === 0
                    deck: window.libraryDeck
                    onDone: window.libraryDeck = "" // loaded: the next song to the free deck
                }

                PhrasingPage {
                    objectName: "phrasing"
                    anchors.fill: parent
                    visible: window.page === 1
                    deck1: matrix1
                    deck2: matrix2
                }

                MixerPage {
                    anchors.fill: parent
                    visible: window.page === 2
                }

                Item {
                    anchors.fill: parent
                    visible: window.page === 3

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

            DFader {
                id: crossfader

                objectName: "crossfader"
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
            active: window.overlay === "menu"
            sourceComponent: menuComponent
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
            onWheel: {
                window.page = 3;
                window.overlay = "";
            }
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

    onWidthChanged: mapTimer.restart()
    onHeightChanged: mapTimer.restart()
    onPageChanged: mapTimer.restart()
    onLandscapeChanged: mapTimer.restart()

    Component.onCompleted: {
        Mixxx.DJMantra.log("interface ready " + width + "x" + height);
        Mixxx.DJMantra.hideBars();
    }
}
