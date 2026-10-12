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

    property int page: 0 // 0 mixer, 1 waveforms, 2 pads
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
                color: "#000000"
                border.color: "#3a3c41"

                Text {
                    anchors.centerIn: parent
                    text: "dj"
                    color: "white"
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
                height: 92
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
                height: 92
                group: "[Channel2]"
                mirrored: true
                deckColor: Style.deck2
                onCoverTapped: {
                    window.libraryDeck = "[Channel2]";
                    window.overlay = "library";
                }
            }
        }

        Row {
            id: tabs

            anchors.top: headers.bottom
            anchors.topMargin: Style.gap
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10

            Repeater {
                model: [qsTr("MIXER"), qsTr("WAVEFORMS"), qsTr("PADS")]

                delegate: DButton {
                    required property string modelData
                    required property int index

                    objectName: "tab" + index
                    width: 108
                    height: 38
                    text: modelData
                    fontSize: 13
                    lit: window.page === index
                    onClicked: window.page = index
                }
            }
        }

        Item {
            id: pages

            objectName: "page"
            anchors.top: tabs.bottom
            anchors.topMargin: Style.gap
            anchors.bottom: transport.top
            anchors.left: parent.left
            anchors.right: parent.right

            MixerPage {
                anchors.fill: parent
                visible: window.page === 0
            }

            WaveformsPage {
                anchors.fill: parent
                visible: window.page === 1
            }

            PadsPage {
                anchors.fill: parent
                visible: window.page === 2
            }
        }

        Transport {
            id: transport

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }

        Loader {
            anchors.fill: parent
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

    Component.onCompleted: {
        Mixxx.DJMantra.log("interface ready " + width + "x" + height);
        Mixxx.DJMantra.hideBars();
    }
}
