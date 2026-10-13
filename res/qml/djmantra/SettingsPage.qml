import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Settings (Marko, 12.10.2026): automatic pre-listen, and the audio router:
// MASTER and HEADPHONES each to any output the phone has now (speaker, wired,
// USB interface, Bluetooth ...), the list following what is plugged in; on a
// multi-channel interface each to any stereo pair (1-2, 3-4 ...): master on
// 1-2 and pre-listen on 3-4 of the same interface, or the other way round.
Rectangle {
    id: root

    property var autoCue // AutoCue
    property var outputs: []
    property string master: ""
    property string headphones: "none"
    property int masterPair: 0     // 0 = channels 1-2, 1 = 3-4 ...
    property int headphonesPair: 0

    function channelsOf(key) {
        for (const o of outputs) {
            if (o.key === key)
                return o.channels || 2;
        }
        return 2;
    }
    property string status: ""
    property int sectionAlgorithm: Number(Mixxx.DJMantra.setting("section_algorithm", "0"))
    property int sectionSensitivity: Number(Mixxx.DJMantra.setting("section_sensitivity", "3"))
    property int beatGridMode: Number(Mixxx.DJMantra.setting("beat_grid_mode", "0"))
    property bool beatOnKick: Mixxx.DJMantra.setting("beat_on_kick", "1") !== "0"
    property int cqTapKey: Number(Mixxx.DJMantra.setting("cq_tap_key", "0"))
    property int cqFx: Number(Mixxx.DJMantra.setting("cq_fx", "1"))
    property int seenVersion: -1
    signal done()

    color: Style.background

    function refresh() {
        outputs = Mixxx.DJMantra.audioOutputs();
        seenVersion = Mixxx.DJMantra.audioOutputsVersion();
    }

    Component.onCompleted: {
        const route = Mixxx.DJMantra.audioRoute();
        master = route.master;
        headphones = route.headphones;
        masterPair = route.masterPair || 0;
        headphonesPair = route.headphonesPair || 0;
        refresh();
    }

    Timer {
        // live: an output plugged in or out
        interval: 1000
        repeat: true
        running: root.visible
        onTriggered: {
            if (Mixxx.DJMantra.audioOutputsVersion() !== root.seenVersion)
                root.refresh();
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    // round 22: the ← button stays put, only the settings scroll
    Row {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        spacing: 10

        DButton {
            width: 52
            height: 40
            text: "←"
            fontSize: 22
            onClicked: root.done()
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Settings")
            color: Style.text
            font.pixelSize: 20
            font.bold: true
        }
    }

    CloseButton {
        id: settingsClose

        objectName: "settingsClose"
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: header.verticalCenter
        onClicked: root.done()
    }

    VersionLink {
        // the version, top right: a tap opens the newest release (update)
        objectName: "settingsVersion"
        anchors.right: settingsClose.left
        anchors.rightMargin: 12
        anchors.verticalCenter: header.verticalCenter
    }

    Flickable {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        contentHeight: column.height
        clip: true

        Column {
            id: column

            width: parent.width
            spacing: 10

            DButton {
                // automatic pre-listen
                objectName: "autoCue"
                width: parent.width
                height: 48
                text: qsTr("AUTO CUE: headphones follow the deck that is not on air")
                fontSize: 13
                lit: root.autoCue ? root.autoCue.active : false
                onClicked: if (root.autoCue) root.autoCue.active = !root.autoCue.active
            }

            // V8 (Marko 13.10.2026): sections and beats are a choice, to find the best
            // in practice. Each choice is written to the log ("Settings:", "Sections
            // options:", "Beats options:").
            // the Allen & Heath CQ mixer on the network (Marko 13.10.2026): its delay
            // in time with the decks, triggered from the app
            Text {
                text: qsTr("CQ MIXER (Allen & Heath, on the network)")
                color: Style.accent
                font.pixelSize: 14
                font.bold: true
            }

            Row {
                spacing: 6

                Rectangle {
                    width: column.width - 6 - 120
                    height: 42
                    radius: Style.radius
                    color: Style.tile
                    border.color: Style.surface

                    TextInput {
                        id: cqAddress

                        objectName: "cqAddress"
                        anchors.fill: parent
                        anchors.margins: 10
                        verticalAlignment: TextInput.AlignVCenter
                        color: Style.text
                        font.pixelSize: 16
                        inputMethodHints: Qt.ImhPreferNumbers
                        text: Mixxx.DJMantra.setting("cq_ip", "")
                    }
                }

                DButton {
                    objectName: "cqConnect"
                    width: 120
                    height: 42
                    text: Mixxx.DJMantra.cqConnected ? qsTr("DISCONNECT") : qsTr("CONNECT")
                    fontSize: 13
                    lit: Mixxx.DJMantra.cqConnected
                    onClicked: Mixxx.DJMantra.cqConnected ? Mixxx.DJMantra.cqDisconnect()
                                                          : Mixxx.DJMantra.cqConnect(cqAddress.text)
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: Mixxx.DJMantra.cqStatus + qsTr("  (the CQ's IP address, e.g. 169.254.5.29)")
                color: Style.subText
                font.pixelSize: 12
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("The delay's FX slot, and the soft key set to its Tap Tempo on the CQ (the app taps the on-air deck's BPM in)")
                color: Style.subText
                font.pixelSize: 13
            }

            Row {
                spacing: 6

                Repeater {
                    model: 4

                    delegate: DButton {
                        required property int index

                        objectName: "cqFx_" + (index + 1)
                        width: (column.width - 18) / 4
                        height: 38
                        text: "FX " + (index + 1)
                        fontSize: 13
                        lit: root.cqFx === index + 1
                        onClicked: {
                            root.cqFx = index + 1;
                            Mixxx.DJMantra.setSetting("cq_fx", index + 1);
                        }
                    }
                }
            }

            Row {
                spacing: 6

                Repeater {
                    model: [qsTr("NO TAP"), qsTr("SOFT KEY 1"), qsTr("SOFT KEY 2"), qsTr("SOFT KEY 3")]

                    delegate: DButton {
                        required property int index
                        required property string modelData

                        objectName: "cqTapKey_" + index
                        width: (column.width - 18) / 4
                        height: 38
                        text: modelData
                        fontSize: 11
                        lit: root.cqTapKey === index
                        onClicked: {
                            root.cqTapKey = index;
                            Mixxx.DJMantra.setSetting("cq_tap_key", index);
                        }
                    }
                }
            }

            Text {
                text: qsTr("SECTIONS: how they are found")
                color: Style.accent
                font.pixelSize: 14
                font.bold: true
            }

            Repeater {
                model: [{value: 0, label: qsTr("ENERGY CHANGES (default)"),
                         about: qsTr("Phrases learned from rekordbox, kept only where the energy changes a lot (kick in / out, breakdown, drop). About 8 per song.")},
                        {value: 1, label: qsTr("PHRASES"),
                         about: qsTr("Every phrase start, as rekordbox marks them (verse, chorus, bridge). About 20 per song.")},
                        {value: 2, label: qsTr("ENERGY ONLY"),
                         about: qsTr("No learned model: every 4 bars is a candidate, kept where the energy changes a lot.")},
                        {value: 3, label: qsTr("CLASSIC 8-BAR"),
                         about: qsTr("The first detector: kick and band changes on an 8-bar grid. Sensitivity does not change it.")}]

                delegate: DButton {
                    required property var modelData

                    objectName: "sectionAlgorithm_" + modelData.value
                    width: column.width
                    height: 58
                    text: modelData.label + "\n" + modelData.about
                    fontSize: 12
                    wrap: true
                    lit: root.sectionAlgorithm === modelData.value
                    onClicked: {
                        root.sectionAlgorithm = modelData.value;
                        Mixxx.DJMantra.setSetting("section_algorithm", modelData.value);
                    }
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Sensitivity: 1 = only the strongest changes, 5 = the most changes")
                color: Style.subText
                font.pixelSize: 13
            }

            Row {
                spacing: 6

                Repeater {
                    model: 5

                    delegate: DButton {
                        required property int index

                        objectName: "sectionSensitivity_" + (index + 1)
                        width: (column.width - 24) / 5
                        height: 42
                        text: String(index + 1)
                        lit: root.sectionSensitivity === index + 1
                        onClicked: {
                            root.sectionSensitivity = index + 1;
                            Mixxx.DJMantra.setSetting("section_sensitivity", index + 1);
                        }
                    }
                }
            }

            Text {
                text: qsTr("BEATS: the grid")
                color: Style.accent
                font.pixelSize: 14
                font.bold: true
            }

            Repeater {
                model: [{value: 0, label: qsTr("AUTO (default)"),
                         about: qsTr("A steady grid; a beat map that follows the drummer when the tempo drifts (live drums).")},
                        {value: 1, label: qsTr("ALWAYS A STEADY GRID"),
                         about: qsTr("One tempo for the whole song, even with live drums.")}]

                delegate: DButton {
                    required property var modelData

                    objectName: "beatGridMode_" + modelData.value
                    width: column.width
                    height: 58
                    text: modelData.label + "\n" + modelData.about
                    fontSize: 12
                    wrap: true
                    lit: root.beatGridMode === modelData.value
                    onClicked: {
                        root.beatGridMode = modelData.value;
                        Mixxx.DJMantra.setSetting("beat_grid_mode", modelData.value);
                    }
                }
            }

            DButton {
                objectName: "beatOnKick"
                width: parent.width
                height: 58
                text: qsTr("GRID ON THE KICK") + "\n"
                        + qsTr("The grid moves half a beat when the kick drum lines up better (hi-hat songs).")
                fontSize: 12
                wrap: true
                lit: root.beatOnKick
                onClicked: {
                    root.beatOnKick = !root.beatOnKick;
                    Mixxx.DJMantra.setSetting("beat_on_kick", root.beatOnKick ? 1 : 0);
                }
            }

            DButton {
                objectName: "analyzeAgain"
                width: parent.width
                height: 48
                text: qsTr("ANALYSE THE STOPPED DECKS AGAIN (beat options apply to new analyses)")
                fontSize: 12
                wrap: true
                onClicked: {
                    const count = Mixxx.DJMantra.analyzeDecksAgain();
                    root.status = count > 0 ? qsTr("Analysing %1 song(s) again ...").arg(count)
                                            : qsTr("No stopped deck with a song.");
                }
            }

            Repeater {
                // MASTER, then HEADPHONES: one button per output
                model: [{dest: "master", title: qsTr("MASTER (the dance floor)")},
                        {dest: "headphones", title: qsTr("HEADPHONES (pre-listen)")}]

                delegate: Column {
                    id: destination

                    required property var modelData

                    width: column.width
                    spacing: 6

                    Text {
                        text: destination.modelData.title
                        color: Style.accent
                        font.pixelSize: 14
                        font.bold: true
                    }

                    Repeater {
                        model: destination.modelData.dest === "headphones"
                                ? [{key: "none", label: qsTr("Off (no separate headphones)")}].concat(root.outputs)
                                : root.outputs

                        delegate: DButton {
                            required property var modelData

                            objectName: "route_" + destination.modelData.dest + "_" + modelData.key
                            width: destination.width
                            height: 42
                            text: modelData.label
                            fontSize: 13
                            lit: (destination.modelData.dest === "master" ? root.master : root.headphones) === modelData.key
                            onClicked: {
                                if (destination.modelData.dest === "master") {
                                    root.master = modelData.key;
                                    root.masterPair = 0;
                                } else {
                                    root.headphones = modelData.key;
                                    // the same interface: the next free pair
                                    root.headphonesPair = modelData.key === root.master
                                            && root.channelsOf(modelData.key) > 2 ? 1 : 0;
                                }
                            }
                        }
                    }

                    Row {
                        // the stereo pair on a multi-channel interface
                        readonly property string chosen: destination.modelData.dest === "master" ? root.master : root.headphones
                        readonly property int pairs: chosen === "none" ? 1 : Math.floor(root.channelsOf(chosen) / 2)

                        visible: pairs > 1
                        spacing: 6

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("channels")
                            color: Style.subText
                            font.pixelSize: 13
                        }

                        Repeater {
                            model: parent.pairs

                            delegate: DButton {
                                required property int index

                                objectName: "pair_" + destination.modelData.dest + "_" + (index + 1)
                                width: 64
                                height: 36
                                text: (2 * index + 1) + "-" + (2 * index + 2)
                                fontSize: 13
                                lit: (destination.modelData.dest === "master" ? root.masterPair : root.headphonesPair) === index
                                onClicked: {
                                    if (destination.modelData.dest === "master")
                                        root.masterPair = index;
                                    else
                                        root.headphonesPair = index;
                                }
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: root.master.startsWith("bt:") || root.headphones.startsWith("bt:")
                text: qsTr("Bluetooth adds about 200 ms of delay: fine for listening, not for beat matching by ear.")
                color: Style.fire
                font.pixelSize: 13
            }

            DButton {
                objectName: "routeApply"
                width: parent.width
                height: 48
                text: qsTr("APPLY")
                lit: true
                onClicked: {
                    root.status = qsTr("switching ...");
                    const error = Mixxx.DJMantra.setAudioRoute(root.master, root.masterPair,
                            root.headphones, root.headphonesPair);
                    root.status = error === "" ? qsTr("Done.") : error;
                    const route = Mixxx.DJMantra.audioRoute();
                    root.master = route.master;
                    root.headphones = route.headphones;
                    root.masterPair = route.masterPair || 0;
                    root.headphonesPair = route.headphonesPair || 0;
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: root.status
                color: Style.subText
                font.pixelSize: 13
            }
        }
    }
}
