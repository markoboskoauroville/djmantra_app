import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// ROOM EQ (Marko 14.10.2026: "EQ output to adjust actually to the room, correcting the room ...
// as in FabFilter ... under EQ there is analyzer analyzing the input ... a noise generator ... a
// frequency sweep ... 3 measures in the different parts of the room, and then it will average
// and make a room correction curve"). Ten octave bands on the main output only (the speakers,
// not the recording). The graph, as FabFilter Pro-Q: log frequency, the live microphone
// analyzer underneath, the measured places and their average, the EQ curve with a handle per
// band (drag up / down, double tap = 0 dB). MEASURE plays pink noise or a sweep and listens;
// APPLY turns the average into the correction: cut only, so the speakers are never driven harder.
Rectangle {
    id: root

    signal done()

    readonly property var centers: [31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
    readonly property var names: ["31", "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"]
    property var bands: Mixxx.DJMantra.roomBands()
    property bool on: Mixxx.DJMantra.setting("room_eq_on", "0") === "1"
    property bool analyzerOn: false
    property var live: []
    // the places measured: [{bands: [10], columns: [61]}] (null = not yet)
    property var places: [null, null, null]
    property int measuring: -1
    property string signalKind: Mixxx.DJMantra.setting("room_eq_signal", "noise")
    property int seconds: 8
    property string status: qsTr("Speakers at a normal loud level, the room quiet. Hold the phone where the "
                                 + "dancers' ears are, microphone up, and MEASURE three places.")

    readonly property var average: {
        const done = places.filter((p) => p !== null);
        if (done.length === 0)
            return null;
        const avg = [];
        for (let b = 0; b < 10; ++b)
            avg.push(done.reduce((sum, p) => sum + p.bands[b], 0) / done.length);
        return avg;
    }

    function refreshBands() {
        bands = Mixxx.DJMantra.roomBands();
        graph.requestPaint();
    }

    function measure(place) {
        if (measuring >= 0)
            return;
        measuring = place;
        status = qsTr("PLACE ") + (place + 1) + ": " + (signalKind === "sweep" ? qsTr("sweep") : qsTr("pink noise"))
                + qsTr(" for ") + seconds + qsTr(" s ... keep quiet");
        Mixxx.DJMantra.roomEqMeasure(signalKind, seconds);
    }

    // the average turned into the correction (cut only: the loudest band ends at 0 dB)
    function apply() {
        if (average === null) {
            Toasts.show(qsTr("MEASURE A PLACE FIRST"));
            return;
        }
        const correction = average.map((db) => Math.max(-12, Math.min(6, -db)));
        const lift = Math.max(0, Math.max.apply(null, correction));
        for (let b = 0; b < 10; ++b)
            Mixxx.DJMantra.setRoomBand(b, Math.max(-12, correction[b] - lift));
        Mixxx.DJMantra.setRoomEqOn(true);
        on = true;
        refreshBands();
        const n = places.filter((p) => p !== null).length;
        status = qsTr("Room correction from ") + n + (n === 1 ? qsTr(" place") : qsTr(" places")) + qsTr(" applied (ON)");
        Toasts.show(qsTr("ROOM EQ APPLIED"));
    }

    function flat() {
        for (let b = 0; b < 10; ++b)
            Mixxx.DJMantra.setRoomBand(b, 0);
        refreshBands();
        Toasts.show(qsTr("ROOM EQ → FLAT"));
    }

    // the EQ's answer at f Hz in dB: the ten peaking filters (RBJ, Q = sqrt 2, 48 kHz)
    function response(f) {
        let total = 0;
        for (let b = 0; b < 10; ++b) {
            const g = bands[b];
            if (Math.abs(g) < 0.01)
                continue;
            const A = Math.pow(10, g / 40), w0 = 2 * Math.PI * centers[b] / 48000;
            const alpha = Math.sin(w0) / (2 * 1.41421356), c = Math.cos(w0);
            const b0 = 1 + alpha * A, b1 = -2 * c, b2 = 1 - alpha * A;
            const a0 = 1 + alpha / A, a1 = -2 * c, a2 = 1 - alpha / A;
            const w = 2 * Math.PI * f / 48000;
            const cr = Math.cos(w), ci = -Math.sin(w), c2r = Math.cos(2 * w), c2i = -Math.sin(2 * w);
            const nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i;
            const dr = a0 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
            total += 10 * Math.log((nr * nr + ni * ni) / (dr * dr + di * di)) / Math.LN10;
        }
        return total;
    }

    color: Style.background

    Component.onDestruction: Mixxx.DJMantra.roomEqLive(false)

    Connections {
        target: Mixxx.DJMantra
        function onRoomEqResult(action, result) {
            if (action !== "measure")
                return;
            const place = root.measuring;
            root.measuring = -1;
            if (result.startsWith("error")) {
                root.status = result;
                Toasts.show(qsTr("MEASUREMENT FAILED"));
                return;
            }
            const parts = result.split("|");
            const copy = root.places.slice();
            copy[place] = {bands: parts[0].split(",").map(Number), columns: parts[1].split(",").map(Number)};
            root.places = copy;
            root.status = qsTr("PLACE ") + (place + 1) + qsTr(" measured. ")
                    + (root.places.filter((p) => p !== null).length < 3 ? qsTr("Move to the next place, or APPLY.")
                                                                         : qsTr("All three: APPLY."));
            Toasts.show(qsTr("PLACE ") + (place + 1) + qsTr(" MEASURED"));
            graph.requestPaint();
        }
    }

    Timer {
        // the live analyzer, about 10 pictures a second
        interval: 100
        repeat: true
        running: root.analyzerOn
        onTriggered: {
            const text = Mixxx.DJMantra.roomEqLiveColumns();
            if (text === "")
                return;
            const cols = text.split(",").map(Number);
            // relative to its own 250 Hz - 4 kHz (columns 22..46), as the measurements
            let ref = 0;
            for (let c = 22; c <= 46; ++c)
                ref += cols[c] / 25;
            root.live = cols.map((v) => v - ref);
            graph.requestPaint();
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Column {
        id: content

        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Item {
            width: parent.width
            height: 44

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("ROOM EQ")
                color: Style.accent
                font.pixelSize: 18
                font.bold: true
            }

            Row {
                anchors.right: closeButton.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                DButton {
                    objectName: "roomEqOn"
                    width: 76
                    height: 40
                    mode: ""
                    text: root.on ? qsTr("ON") : qsTr("OFF")
                    lit: root.on
                    onClicked: {
                        root.on = !root.on;
                        Mixxx.DJMantra.setRoomEqOn(root.on);
                        Toasts.show(qsTr("ROOM EQ ") + (root.on ? qsTr("ON") : qsTr("OFF")));
                    }
                }

                DButton {
                    objectName: "roomEqAnalyzer"
                    width: 104
                    height: 40
                    mode: ""
                    text: qsTr("ANALYZER")
                    fontSize: 12
                    lit: root.analyzerOn
                    onClicked: {
                        root.analyzerOn = !root.analyzerOn;
                        Mixxx.DJMantra.roomEqLive(root.analyzerOn);
                        if (!root.analyzerOn) {
                            root.live = [];
                            graph.requestPaint();
                        }
                        Toasts.show(qsTr("ANALYZER ") + (root.analyzerOn ? qsTr("ON (the microphone)") : qsTr("OFF")));
                    }
                }
            }

            CloseButton {
                id: closeButton

                objectName: "roomEqClose"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.done()
            }
        }

        Canvas {
            id: graph

            readonly property real dbTop: 12
            readonly property real dbBottom: -18
            readonly property real padLeft: 34

            function xOf(f) {
                return padLeft + (width - padLeft - 6) * (Math.log(f / 20) / Math.log(1000));
            }
            function yOf(db) {
                return 6 + (height - 24) * (dbTop - db) / (dbTop - dbBottom);
            }
            function dbOf(y) {
                return dbTop - (y - 6) / (height - 24) * (dbTop - dbBottom);
            }

            objectName: "roomEqGraph"
            width: parent.width
            height: Math.max(180, root.height * 0.42)
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Component.onCompleted: requestPaint()

            onPaint: {
                const g = getContext("2d");
                g.reset();
                g.fillStyle = String(Style.surface);
                g.fillRect(0, 0, width, height);
                g.font = "bold 10px sans-serif";
                // the grid: octaves and every 6 dB
                g.lineWidth = 1;
                for (let b = 0; b < 10; ++b) {
                    const x = xOf(root.centers[b]);
                    g.strokeStyle = "#22305a";
                    g.beginPath(); g.moveTo(x, 4); g.lineTo(x, height - 16); g.stroke();
                    g.fillStyle = String(Style.subText);
                    g.fillText(root.names[b], x - 8, height - 3);
                }
                for (let db = 12; db >= -18; db -= 6) {
                    const y = yOf(db);
                    g.strokeStyle = db === 0 ? "#3d4c86" : "#22305a";
                    g.beginPath(); g.moveTo(padLeft, y); g.lineTo(width - 4, y); g.stroke();
                    g.fillStyle = String(Style.subText);
                    g.fillText((db > 0 ? "+" : "") + db, 4, y + 4);
                }
                // the live analyzer, filled, underneath
                if (root.live.length === 61) {
                    g.fillStyle = "rgba(180, 140, 255, 0.28)";
                    g.strokeStyle = "rgba(180, 140, 255, 0.8)";
                    g.beginPath();
                    g.moveTo(xOf(20), yOf(dbBottom));
                    for (let c = 0; c < 61; ++c)
                        g.lineTo(xOf(20 * Math.pow(2, c / 6)), yOf(Math.max(dbBottom, Math.min(dbTop, root.live[c]))));
                    g.lineTo(xOf(20000), yOf(dbBottom));
                    g.closePath();
                    g.fill();
                }
                // the places measured, thin; their average, dashed white
                const colours = ["#5AA2F0", "#6CCB72", "#FF6B5E"];
                for (let p = 0; p < 3; ++p) {
                    const place = root.places[p];
                    if (!place)
                        continue;
                    g.strokeStyle = colours[p];
                    g.lineWidth = 1;
                    g.beginPath();
                    for (let c = 0; c < place.columns.length; ++c) {
                        const x = xOf(20 * Math.pow(2, c / 6)), y = yOf(Math.max(dbBottom, Math.min(dbTop, place.columns[c])));
                        if (c === 0) g.moveTo(x, y); else g.lineTo(x, y);
                    }
                    g.stroke();
                }
                if (root.average) {
                    g.strokeStyle = "#ffffff";
                    g.lineWidth = 2;
                    g.setLineDash ? g.setLineDash([5, 4]) : 0;
                    g.beginPath();
                    for (let b = 0; b < 10; ++b) {
                        const x = xOf(root.centers[b]), y = yOf(Math.max(dbBottom, Math.min(dbTop, root.average[b])));
                        if (b === 0) g.moveTo(x, y); else g.lineTo(x, y);
                    }
                    g.stroke();
                    g.setLineDash ? g.setLineDash([]) : 0;
                }
                // the EQ curve and its handles
                g.strokeStyle = String(Style.accent);
                g.lineWidth = 2.5;
                g.beginPath();
                for (let i = 0; i <= 120; ++i) {
                    const f = 20 * Math.pow(1000, i / 120);
                    const y = yOf(Math.max(dbBottom, Math.min(dbTop, root.response(f))));
                    if (i === 0) g.moveTo(xOf(f), y); else g.lineTo(xOf(f), y);
                }
                g.stroke();
                for (let b = 0; b < 10; ++b) {
                    const x = xOf(root.centers[b]), y = yOf(root.bands[b]);
                    g.fillStyle = b === drag.band ? String(Style.text) : String(Style.accent);
                    g.beginPath(); g.arc(x, y, b === drag.band ? 9 : 6, 0, 2 * Math.PI); g.fill();
                }
            }

            MouseArea {
                // drag a band's handle up / down; a double tap puts it back at 0 dB
                id: drag

                property int band: -1

                function nearest(x) {
                    let best = 0;
                    for (let b = 1; b < 10; ++b) {
                        if (Math.abs(graph.xOf(root.centers[b]) - x) < Math.abs(graph.xOf(root.centers[best]) - x))
                            best = b;
                    }
                    return best;
                }

                anchors.fill: parent
                preventStealing: true
                onPressed: (mouse) => {
                    band = nearest(mouse.x);
                    graph.requestPaint();
                }
                onPositionChanged: (mouse) => {
                    if (band < 0)
                        return;
                    const db = Math.round(Math.max(-12, Math.min(12, graph.dbOf(mouse.y))) * 2) / 2;
                    Mixxx.DJMantra.setRoomBand(band, db);
                    root.refreshBands();
                }
                onReleased: {
                    if (band >= 0)
                        Toasts.show(root.names[band] + " Hz " + (root.bands[band] > 0 ? "+" : "") + root.bands[band].toFixed(1) + " dB");
                    band = -1;
                    graph.requestPaint();
                }
                onDoubleClicked: (mouse) => {
                    const b = nearest(mouse.x);
                    Mixxx.DJMantra.setRoomBand(b, 0);
                    root.refreshBands();
                    Toasts.show(root.names[b] + " Hz → 0 dB");
                }
            }
        }

        Row {
            // the test signal and its length
            spacing: 6

            Repeater {
                model: [["noise", qsTr("PINK NOISE")], ["sweep", qsTr("SWEEP")]]

                delegate: DButton {
                    required property var modelData

                    width: (content.width - 3 * 6) / 4
                    height: 42
                    mode: ""
                    text: modelData[1]
                    fontSize: 12
                    lit: root.signalKind === modelData[0]
                    onClicked: {
                        root.signalKind = modelData[0];
                        Mixxx.DJMantra.setSetting("room_eq_signal", modelData[0]);
                    }
                }
            }

            Repeater {
                model: [5, 10]

                delegate: DButton {
                    required property int modelData

                    width: (content.width - 3 * 6) / 4
                    height: 42
                    mode: ""
                    text: modelData + " s"
                    fontSize: 12
                    lit: root.seconds === modelData
                    onClicked: root.seconds = modelData
                }
            }
        }

        Row {
            // the three places
            spacing: 6

            Repeater {
                model: 3

                delegate: DButton {
                    required property int index

                    objectName: "roomEqPlace" + (index + 1)
                    width: (content.width - 2 * 6) / 3
                    height: 52
                    mode: ""
                    text: root.measuring === index ? qsTr("LISTENING ...")
                          : qsTr("MEASURE ") + (index + 1) + (root.places[index] ? "  ✓" : "")
                    fontSize: 13
                    lit: root.places[index] !== null || root.measuring === index
                    onClicked: root.measure(index)
                }
            }
        }

        Row {
            spacing: 6

            DButton {
                objectName: "roomEqApply"
                width: (content.width - 6) * 0.6
                height: 52
                text: qsTr("APPLY THE AVERAGE")
                lit: root.average !== null
                onClicked: root.apply()
            }

            DButton {
                objectName: "roomEqFlat"
                width: (content.width - 6) * 0.4
                height: 52
                mode: ""
                text: qsTr("FLAT")
                onClicked: root.flat()
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: root.status
            color: Style.text
            font.pixelSize: 13
        }
    }
}
