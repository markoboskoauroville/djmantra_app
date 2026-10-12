import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12
import QtQuick.Window 2.12

// DJ Mantra's interface in Qt Quick: everything is drawn by the GPU (Marko,
// 12.10.2026). Portrait first: the top row (logo left, Camelot badge right,
// the middle free for the selfie camera hole), the two deck headers, the
// tabs MIXER | WAVEFORMS | PADS, then SYNC / CUE / PLAY and the crossfader.
// The system bars and a camera hole off the middle are kept clear with the
// insets from Mixxx.DJMantra.
Window {
    id: window

    visible: true
    width: 413
    height: 919
    color: Style.background
    title: "DJ Mantra"

    property int page: 0 // 0 mix (beat pads), 1 mixer, 2 keys (Camelot wheel)
    readonly property bool landscape: width > height
    property string overlay: "" // "", "menu", "library", "wheel"
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
        anchors.leftMargin: Mixxx.DJMantra.insetLeft
        anchors.topMargin: Mixxx.DJMantra.insetTop
        anchors.rightMargin: Mixxx.DJMantra.insetRight
        anchors.bottomMargin: Mixxx.DJMantra.insetBottom
        focus: true

        Keys.onBackPressed: (event) => {
            // Android's back: closes what is open, never the app
            event.accepted = true;
            window.overlay = "";
        }

        Item {
            id: topBar

            width: parent.width
            height: 44

            Rectangle {
                id: logo

                objectName: "logo"
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 36
                height: 36
                radius: 8
                color: Style.accent

                Text {
                    anchors.centerIn: parent
                    text: "dj"
                    color: Style.onAccent
                    font.pixelSize: 20
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        Mixxx.DJMantra.log("menu");
                        window.overlay = "menu";
                    }
                }
            }

            KeyBadge {
                objectName: "keyBadge"
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                onWheelRequested: window.overlay = "wheel"
            }
        }

        Row {
            id: headers

            anchors.top: topBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Style.gap
            spacing: 14

            DeckHeader {
                objectName: "deck1"
                width: (headers.width - headers.spacing) / 2
                height: 74
                group: "[Channel1]"
                deckColor: Style.deck1
                onCoverTapped: {
                    window.libraryDeck = "[Channel1]";
                    window.overlay = "library";
                }
            }

            DeckHeader {
                objectName: "deck2"
                width: (headers.width - headers.spacing) / 2
                height: 74
                group: "[Channel2]"
                mirrored: true
                deckColor: Style.deck2
                onCoverTapped: {
                    window.libraryDeck = "[Channel2]";
                    window.overlay = "library";
                }
            }
        }

        MatrixDeck { id: matrix1; group: "[Channel1]"; deckColor: Style.deck1 }
        MatrixDeck { id: matrix2; group: "[Channel2]"; deckColor: Style.deck2 }

        MatrixDisplay {
            // the OXI E16's dot-matrix display (Marko, 12.10.2026): each
            // deck's title, key and BPM, section countdown and phrase beat
            id: matrix

            objectName: "matrix"
            anchors.top: headers.bottom
            anchors.topMargin: Style.gap
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Style.gap
            anchors.rightMargin: Style.gap
            height: 56
            left: matrix1.lines
            right: matrix2.lines
        }

        PhraseBar {
            // Marko's mixing tool: both decks' phrase lights and section
            // countdowns, always on screen
            id: phraseBar

            objectName: "phraseBar"
            anchors.top: matrix.bottom
            anchors.topMargin: Style.gap
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Style.gap
            anchors.rightMargin: Style.gap
        }

        Loader {
            // portrait: tabs, page, SYNC / CUE / PLAY and crossfader below;
            // landscape: each deck's SYNC / CUE / PLAY at its side, the page and
            // the crossfader in the middle (round 13: the page was 40 px high)
            id: body

            anchors.top: phraseBar.bottom
            anchors.topMargin: Style.gap
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            sourceComponent: window.landscape ? landscapeBody : portraitBody
        }

        Component {
            id: tabsComponent

            NavBar {
                current: window.page
                onSelected: (index) => window.page = index
            }
        }

        Component {
            id: pagesComponent

            Item {
                objectName: "page"

                MixPage {
                    anchors.fill: parent
                    visible: window.page === 0
                }

                MixerPage {
                    anchors.fill: parent
                    visible: window.page === 1
                }

                WheelPage {
                    anchors.fill: parent
                    visible: window.page === 2
                    embedded: true
                }
            }
        }

        Component {
            id: portraitBody

            Item {
                Loader {
                    anchors.top: parent.top
                    anchors.bottom: portraitTransport.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    sourceComponent: pagesComponent
                }

                Transport {
                    id: portraitTransport

                    anchors.bottom: portraitNav.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                }

                Loader {
                    id: portraitNav

                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    sourceComponent: tabsComponent
                }
            }
        }

        Component {
            id: landscapeBody

            Item {
                DeckTransport {
                    id: leftDeck

                    anchors.left: parent.left
                    anchors.leftMargin: Style.gap
                    anchors.top: parent.top
                    width: 96
                    group: "[Channel1]"
                    deck: 1
                }

                DeckTransport {
                    id: rightDeck

                    anchors.right: parent.right
                    anchors.rightMargin: Style.gap
                    anchors.top: parent.top
                    width: 96
                    group: "[Channel2]"
                    deck: 2
                }

                Loader {
                    id: landscapeTabs

                    anchors.bottom: parent.bottom
                    anchors.left: leftDeck.right
                    anchors.right: rightDeck.left
                    anchors.leftMargin: Style.gap
                    anchors.rightMargin: Style.gap
                    sourceComponent: tabsComponent
                }

                Loader {
                    anchors.top: parent.top
                    anchors.bottom: crossfader.top
                    anchors.bottomMargin: Style.gap
                    anchors.left: leftDeck.right
                    anchors.right: rightDeck.left
                    anchors.leftMargin: Style.gap
                    anchors.rightMargin: Style.gap
                    sourceComponent: pagesComponent
                }

                DFader {
                    id: crossfader

                    objectName: "crossfader"
                    anchors.bottom: landscapeTabs.top
                    anchors.bottomMargin: Style.gap
                    anchors.left: leftDeck.right
                    anchors.right: rightDeck.left
                    anchors.leftMargin: Style.gap
                    anchors.rightMargin: Style.gap
                    height: 44
                    vertical: false
                    group: "[Master]"
                    key: "crossfader"
                }
            }
        }

        Loader {
            // below the top bar: the camera hole stays clear (round 13: the
            // wheel's title ran under it)
            anchors.top: topBar.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            active: window.overlay !== ""
            sourceComponent: window.overlay === "menu" ? menuComponent
                    : window.overlay === "library" ? libraryComponent
                    : window.overlay === "wheel" ? wheelComponent : null
        }
    }

    Component {
        id: menuComponent

        MenuSheet {
            onLibrary: {
                window.libraryDeck = "";
                window.overlay = "library";
            }
            onWheel: window.overlay = "wheel"
            onDone: window.overlay = ""
        }
    }

    Component {
        id: libraryComponent

        LibraryPage {
            deck: window.libraryDeck
            onDone: window.overlay = ""
        }
    }

    Component {
        id: wheelComponent

        WheelPage {
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
