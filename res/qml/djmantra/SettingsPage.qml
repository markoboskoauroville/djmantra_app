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
