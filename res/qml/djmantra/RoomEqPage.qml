import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// ROOM CORRECTION (Marko 14.10.2026, as IK Multimedia's ARC: "I press the button, the sweep is
// happening ... and then the curve is generated"; industry-standard correction from several
// places averaged; rooms saved; seven places; a BYPASS; a dropdown with the algorithm and a
// guide; the phone's or an external microphone).
//
// 31 third-octave bands on the main output only (the speakers, not the recording). A place is
// measured by playing the test signal (a log sweep or pink noise) on the app's own mixer (the
// 9th sampler, so it reaches every output: the PA over USB, Bluetooth) while the chosen
// microphone records; RoomEqBridge.java turns it into the room's answer. The places a room keeps
// (7) are each in or out of the average; APPLY turns the average into the correction:
//   the average of the places' power, relative to 200 Hz - 2 kHz; the TARGET curve (FLAT,
//   HOUSE, CLUB); below the speakers' low limit (12 dB down) and above 16 kHz (where a phone's
//   microphone is not to be trusted) nothing; boosts at most +6 dB, cuts at most -12 dB, times
//   the STRENGTH, smoothed over the neighbours; then shifted so the highest band is 0 dB
//   (cut only: the speakers are never driven harder, nothing clips).
Rectangle {
    id: root

    signal done()

    readonly property var centers: [20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500,
        630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000, 20000]
    readonly property var placeColours: ["#5AA2F0", "#6CCB72", "#FF6B5E", "#F2D35B", "#B48CFF", "#4DD0C8", "#FF9F43"]
    readonly property string sampler: "[Sampler9]"

    property var bands: Mixxx.DJMantra.roomBands()
    property bool bypassed: Mixxx.DJMantra.setting("room_eq_on", "0") !== "1"
    property bool analyzerOn: false
    property var live: []
    property var rooms: []
    property int current: 0
    readonly property var room: rooms.length > 0 ? rooms[Math.min(current, rooms.length - 1)] : null
    property int measuring: -1
    property bool autoApply: false
    property string signalKind: Mixxx.DJMantra.setting("room_eq_signal", "sweep")
    property int seconds: Number(Mixxx.DJMantra.setting("room_eq_seconds", "8"))
    property int level: Number(Mixxx.DJMantra.setting("room_eq_level", "-18"))
    property int inputId: Number(Mixxx.DJMantra.setting("room_eq_input", "0"))
    property var inputs: Mixxx.DJMantra.roomEqInputs()
    property string panel: "" // "", "more", "rooms", "guide"
    property string status: qsTr("AUTO CALIBRATE: put the phone where the listeners are, microphone up, "
                                 + "and press it. Then move and press again (up to 7 places).")

    // --- the rooms ---------------------------------------------------------------

    function emptyRoom(name) {
        return {name: name, target: "flat", strength: 100, points: [null, null, null, null, null, null, null]};
    }

    function loadRooms() {
        let list = [];
        try {
            list = JSON.parse(Mixxx.DJMantra.roomsLoad());
        } catch (e) {
            list = [];
        }
        if (!Array.isArray(list) || list.length === 0)
            list = [emptyRoom(qsTr("My room"))];
        rooms = list;
        current = Math.max(0, Math.min(list.length - 1, Number(Mixxx.DJMantra.setting("room_current", "0"))));
    }

    // every change is saved at once: the room's own file, Documents/DJ Mantra/Room_Corrections/<name>.json
    function saveRooms() {
        if (room)
            lastSaved = Mixxx.DJMantra.roomSave(JSON.stringify(room));
        Mixxx.DJMantra.setSetting("room_current", current);
        rooms = rooms.slice(); // the bindings see the change
        graph.requestPaint();
    }

    property string lastSaved: ""

    // a room chosen: its own correction comes back with it (FLAT if it has none yet)
    function useRoom(index) {
        current = index;
        status = qsTr("AUTO CALIBRATE: put the phone where the listeners are, microphone up, and press it.");
        Mixxx.DJMantra.setSetting("room_current", current);
        const corr = room && room.correction && room.correction.length === 31 ? room.correction : null;
        for (let b = 0; b < 31; ++b)
            Mixxx.DJMantra.setRoomBand(b, corr ? corr[b] : 0);
        refreshBands();
    }

    function setRoom(change) {
        const copy = rooms.slice();
        const r = JSON.parse(JSON.stringify(copy[current]));
        change(r);
        copy[current] = r;
        rooms = copy;
        saveRooms();
    }

    // --- measuring ---------------------------------------------------------------

    function nextEmpty() {
        if (!room)
            return -1;
        for (let i = 0; i < 7; ++i) {
            if (!room.points[i])
                return i;
        }
        return -1;
    }

    function measure(place, thenApply) {
        if (measuring >= 0)
            return;
        if (deck1Play.value > 0 || deck2Play.value > 0) {
            Toasts.show(qsTr("PAUSE THE MUSIC FIRST"));
            return;
        }
        const path = Mixxx.DJMantra.roomEqSignal(signalKind, seconds);
        if (path === "") {
            status = qsTr("The test signal could not be made (only on the phone)");
            return;
        }
        measuring = place;
        autoApply = thenApply;
        // the room is measured as it is: the correction is out of the way meanwhile
        roomOut.value = 0;
        Mixxx.PlayerManager.getPlayer(sampler).loadTrackFromLocation(path, false);
        samplerVolume.value = Math.pow(10, level / 20);
        samplerRate.value = 0;
        status = qsTr("PLACE ") + (place + 1) + qsTr(": loading the test signal ...");
        loadWait.tries = 0;
        loadWait.start();
    }

    function finishMeasuring() {
        samplerPlay.value = 0;
        roomOut.value = bypassed ? 0 : 1;
        measuring = -1;
    }

    function averageOf(key, count) {
        if (!room)
            return null;
        const used = room.points.filter((p) => p && p.use);
        if (used.length === 0)
            return null;
        const avg = [];
        for (let i = 0; i < count; ++i) {
            // the places' power averaged (as acoustic measurements are)
            const power = used.reduce((sum, p) => sum + Math.pow(10, p[key][i] / 10), 0) / used.length;
            avg.push(10 * Math.log(power) / Math.LN10);
        }
        return avg;
    }

    function target(f, curve) {
        if (curve === "house")
            return 4 / (1 + Math.pow(f / 80, 2)) - (f > 1000 ? 0.8 * Math.log(f / 1000) / Math.LN2 : 0);
        if (curve === "club")
            return 7 / (1 + Math.pow(f / 70, 2)) - (f > 2000 ? 1.0 * Math.log(f / 2000) / Math.LN2 : 0);
        return 0;
    }

    function apply() {
        const avg = averageOf("thirds", 31);
        if (avg === null) {
            Toasts.show(qsTr("MEASURE A PLACE FIRST"));
            return;
        }
        let ref = 0, n = 0;
        for (let b = 0; b < 31; ++b) {
            if (centers[b] >= 200 && centers[b] <= 2000) {
                ref += avg[b];
                ++n;
            }
        }
        ref /= n;
        // the speakers' low limit: the first band no more than 12 dB down
        let low = 0;
        while (low < 31 && avg[low] - ref < -12)
            ++low;
        const strength = room.strength / 100;
        let c = [];
        for (let b = 0; b < 31; ++b) {
            if (b < low || centers[b] > 16000) {
                c.push(0);
                continue;
            }
            const want = (target(centers[b], room.target) - (avg[b] - ref)) * strength;
            c.push(Math.max(-12, Math.min(b === low ? 3 : 6, want)));
        }
        // smoothed over the neighbours: no narrow peaks and notches a third apart
        const s = c.map((v, i) => i === 0 || i === 30 ? v : 0.25 * c[i - 1] + 0.5 * v + 0.25 * c[i + 1]);
        const lift = Math.max(0, Math.max.apply(null, s));
        const finalBands = s.map((v) => Math.round(Math.max(-12, v - lift) * 10) / 10);
        for (let b = 0; b < 31; ++b)
            Mixxx.DJMantra.setRoomBand(b, finalBands[b]);
        setRoom((r) => { r.correction = finalBands; });
        setBypass(false);
        refreshBands();
        const used = room.points.filter((p) => p && p.use).length;
        status = qsTr("Correction from ") + used + (used === 1 ? qsTr(" place") : qsTr(" places"))
                + " (" + room.target.toUpperCase() + ", " + room.strength + qsTr(" %) applied. Listen; BYPASS compares.");
        Toasts.show(qsTr("ROOM CORRECTION APPLIED"));
    }

    // the curve as it is now, kept with the room
    function keepCorrection() {
        const now = Mixxx.DJMantra.roomBands();
        setRoom((r) => { r.correction = now.map((v) => Math.round(v * 10) / 10); });
    }

    function flat() {
        for (let b = 0; b < 31; ++b)
            Mixxx.DJMantra.setRoomBand(b, 0);
        refreshBands();
        keepCorrection();
        Toasts.show(qsTr("ROOM CORRECTION → FLAT"));
    }

    function setBypass(on) {
        bypassed = on;
        Mixxx.DJMantra.setRoomEqOn(!on);
    }

    function refreshBands() {
        bands = Mixxx.DJMantra.roomBands();
        graph.requestPaint();
    }

    // the EQ's answer at f Hz in dB: 31 peaking filters (RBJ, Q 4.318, 48 kHz)
    function response(f) {
        let total = 0;
        const w = 2 * Math.PI * f / 48000;
        const cr = Math.cos(w), ci = -Math.sin(w), c2r = Math.cos(2 * w), c2i = -Math.sin(2 * w);
        for (let b = 0; b < 31; ++b) {
            const g = bands[b];
            if (Math.abs(g) < 0.01)
                continue;
            const A = Math.pow(10, g / 40), w0 = 2 * Math.PI * Math.min(centers[b], 21600) / 48000;
            const alpha = Math.sin(w0) / (2 * 4.318), c = Math.cos(w0);
            const b0 = 1 + alpha * A, b1 = -2 * c, b2 = 1 - alpha * A;
            const a0 = 1 + alpha / A, a1 = -2 * c, a2 = 1 - alpha / A;
            const nr = b0 + b1 * cr + b2 * c2r, ni = b1 * ci + b2 * c2i;
            const dr = a0 + a1 * cr + a2 * c2r, di = a1 * ci + a2 * c2i;
            total += 10 * Math.log((nr * nr + ni * ni) / (dr * dr + di * di)) / Math.LN10;
        }
        return total;
    }

    color: Style.background

    Component.onCompleted: loadRooms()
    Component.onDestruction: {
        Mixxx.DJMantra.roomEqLive(false, 0);
        if (measuring >= 0)
            finishMeasuring();
    }

    Mixxx.ControlProxy { id: roomOut; group: "[OutputEffectRack_[Master]_Effect1]"; key: "enabled" }
    Mixxx.ControlProxy { id: samplerLoaded; group: root.sampler; key: "track_loaded" }
    Mixxx.ControlProxy { id: samplerPlay; group: root.sampler; key: "play" }
    Mixxx.ControlProxy { id: samplerStart; group: root.sampler; key: "cue_gotoandplay" }
    Mixxx.ControlProxy { id: samplerVolume; group: root.sampler; key: "volume" }
    Mixxx.ControlProxy { id: samplerRate; group: root.sampler; key: "rate" }
    Mixxx.ControlProxy { id: deck1Play; group: "[Channel1]"; key: "play" }
    Mixxx.ControlProxy { id: deck2Play; group: "[Channel2]"; key: "play" }

    Timer {
        // the test signal loaded on the sampler: the recording starts, then the signal
        id: loadWait

        property int tries: 0

        interval: 100
        repeat: true
        onTriggered: {
            if (samplerLoaded.value > 0) {
                stop();
                root.status = qsTr("PLACE ") + (root.measuring + 1) + ": "
                        + (root.signalKind === "sweep" ? qsTr("sweeping") : qsTr("pink noise")) + qsTr(" ... keep quiet");
                Mixxx.DJMantra.roomEqMeasure(root.signalKind, root.seconds, root.inputId);
            } else if (++tries > 80) {
                stop();
                root.finishMeasuring();
                root.status = qsTr("The test signal did not load");
            }
        }
    }

    Connections {
        target: Mixxx.DJMantra
        function onRoomEqResult(action, result) {
            if (action === "recording") {
                samplerStart.value = 1;
                return;
            }
            if (action !== "measure")
                return;
            const place = root.measuring;
            root.finishMeasuring();
            if (result.startsWith("error")) {
                root.status = result;
                Toasts.show(qsTr("MEASUREMENT FAILED"));
                return;
            }
            const parts = result.split("|");
            const peak = Number(parts[2]);
            // r41 V70: a measurement that cannot be a room (silence, noise) is not kept: between
            // 63 Hz and 8 kHz a real room stays within about 40 dB, and neighbouring thirds never
            // jump 25 dB
            const thirds = parts[0].split(",").map(Number);
            let lo = 1e9, hi = -1e9, jump = 0;
            for (let b = 5; b <= 26; ++b) {
                lo = Math.min(lo, thirds[b]);
                hi = Math.max(hi, thirds[b]);
                if (b > 5)
                    jump = Math.max(jump, Math.abs(thirds[b] - thirds[b - 1]));
            }
            if (thirds.length !== 31 || thirds.some((v) => !isFinite(v)) || hi - lo > 40 || jump > 25) {
                root.status = qsTr("PLACE ") + (place + 1) + qsTr(" not kept: the microphone did not hear the test "
                        + "signal clearly (peak ") + peak.toFixed(0) + qsTr(" dBFS). Quieter room, a higher TEST LEVEL, or "
                        + "check the MICROPHONE in MORE, then measure again.");
                Toasts.show(qsTr("MEASUREMENT NOT KEPT"));
                return;
            }
            root.setRoom((r) => {
                r.points[place] = {thirds: parts[0].split(",").map(Number), columns: parts[1].split(",").map(Number),
                                   kind: root.signalKind, level: peak, time: Date.now(), use: true};
            });
            root.status = qsTr("PLACE ") + (place + 1) + qsTr(" measured")
                    + (peak > -3 ? qsTr(" (the microphone was overloaded: lower the TEST LEVEL)")
                       : peak < -45 ? qsTr(" (very quiet: raise the TEST LEVEL or the speakers)") : "") + ".";
            Toasts.show(qsTr("PLACE ") + (place + 1) + qsTr(" MEASURED"));
            if (root.autoApply)
                root.apply();
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
            let ref = 0;
            for (let c = 24; c <= 43; ++c) // 200 Hz .. 2 kHz
                ref += cols[c] / 20;
            root.live = cols.map((v) => v - ref);
            graph.requestPaint();
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    Flickable {
        anchors.fill: parent
        contentHeight: content.height + 20
        clip: true

        Column {
            id: content

            x: 10
            y: 10
            width: parent.width - 20
            spacing: 8

            Item {
                width: parent.width
                height: 44

                Text {
                    anchors.left: parent.left
                    anchors.right: headButtons.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("ROOM CORRECTION")
                    color: Style.accent
                    font.pixelSize: 17
                    font.bold: true
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 10
                }

                Row {
                    id: headButtons

                    anchors.right: closeButton.left
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    DButton {
                        // BYPASS (Marko: "really easily be bypassed")
                        objectName: "roomBypass"
                        width: 92
                        height: 40
                        mode: ""
                        text: qsTr("BYPASS")
                        fontSize: 13
                        lit: root.bypassed
                        onClicked: {
                            root.setBypass(!root.bypassed);
                            Toasts.show(root.bypassed ? qsTr("ROOM CORRECTION BYPASSED") : qsTr("ROOM CORRECTION ON"));
                        }
                    }

                    DButton {
                        // the dropdown: the algorithm, the microphone, the signal, the guide
                        objectName: "roomMore"
                        width: 84
                        height: 40
                        mode: ""
                        text: qsTr("MORE") + "   "
                        dropdown: true
                        fontSize: 12
                        lit: root.panel === "more"
                        onClicked: root.panel = root.panel === "more" ? "" : "more"
                    }
                }

                CloseButton {
                    id: closeButton

                    objectName: "roomClose"
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    onClicked: root.done()
                }
            }

            Row {
                spacing: 6

            DButton {
                // the room (rooms are kept; tap: choose, add, rename, delete)
                objectName: "roomChoose"
                width: content.width - 6 - 84
                height: 40
                mode: ""
                text: qsTr("ROOM: ") + (root.room ? root.room.name : "") + "   "
                dropdown: true
                fontSize: 13
                lit: root.panel === "rooms"
                onClicked: root.panel = root.panel === "rooms" ? "" : "rooms"
            }

            DButton {
                // SAVE (Marko 14.10.2026): every change is saved by itself; this one saves and
                // says where (Documents/DJ Mantra/Room_Corrections/<room>.json)
                objectName: "roomSave"
                width: 84
                height: 40
                mode: ""
                text: qsTr("SAVE")
                fontSize: 13
                onClicked: {
                    root.keepCorrection();
                    Toasts.show(qsTr("SAVED: Room_Corrections/") + root.lastSaved.substring(root.lastSaved.lastIndexOf("/") + 1));
                }
            }
            }

            Canvas {
                id: graph

                readonly property real dbTop: 12
                readonly property real dbBottom: -18
                readonly property real padLeft: 30

                function xOf(f) {
                    return padLeft + (width - padLeft - 6) * (Math.log(f / 20) / Math.log(1000));
                }
                function yOf(db) {
                    return 6 + (height - 24) * (dbTop - db) / (dbTop - dbBottom);
                }
                function dbOf(y) {
                    return dbTop - (y - 6) / (height - 24) * (dbTop - dbBottom);
                }
                function clampDb(db) {
                    return Math.max(dbBottom, Math.min(dbTop, db));
                }
                function curve(g, values, step) {
                    g.beginPath();
                    for (let c = 0; c < values.length; ++c) {
                        const x = xOf(20 * Math.pow(2, c / step)), y = yOf(clampDb(values[c]));
                        if (c === 0) g.moveTo(x, y); else g.lineTo(x, y);
                    }
                    g.stroke();
                }

                objectName: "roomGraph"
                width: parent.width
                height: Math.max(190, root.height * 0.36)
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    const g = getContext("2d");
                    g.reset();
                    g.fillStyle = String(Style.surface);
                    g.fillRect(0, 0, width, height);
                    g.font = "bold 10px sans-serif";
                    g.lineWidth = 1;
                    const labels = [[31.5, "31"], [63, "63"], [125, "125"], [250, "250"], [500, "500"], [1000, "1k"],
                                    [2000, "2k"], [4000, "4k"], [8000, "8k"], [16000, "16k"]];
                    for (const l of labels) {
                        const x = xOf(l[0]);
                        g.strokeStyle = "#22305a";
                        g.beginPath(); g.moveTo(x, 4); g.lineTo(x, height - 16); g.stroke();
                        g.fillStyle = String(Style.subText);
                        g.fillText(l[1], x - 8, height - 3);
                    }
                    for (let db = 12; db >= -18; db -= 6) {
                        const y = yOf(db);
                        g.strokeStyle = db === 0 ? "#3d4c86" : "#22305a";
                        g.beginPath(); g.moveTo(padLeft, y); g.lineTo(width - 4, y); g.stroke();
                        g.fillStyle = String(Style.subText);
                        g.fillText((db > 0 ? "+" : "") + db, 3, y + 4);
                    }
                    if (root.live.length === 61) {
                        g.fillStyle = "rgba(180, 140, 255, 0.28)";
                        g.beginPath();
                        g.moveTo(xOf(20), yOf(dbBottom));
                        for (let c = 0; c < 61; ++c)
                            g.lineTo(xOf(20 * Math.pow(2, c / 6)), yOf(clampDb(root.live[c])));
                        g.lineTo(xOf(20000), yOf(dbBottom));
                        g.closePath();
                        g.fill();
                    }
                    if (root.room) {
                        for (let p = 0; p < 7; ++p) {
                            const point = root.room.points[p];
                            if (!point)
                                continue;
                            g.strokeStyle = root.placeColours[p];
                            g.globalAlpha = point.use ? 0.9 : 0.25;
                            g.lineWidth = 1;
                            curve(g, point.columns, 6);
                        }
                        g.globalAlpha = 1;
                        const avg = root.averageOf("columns", 61);
                        if (avg) {
                            g.strokeStyle = "#ffffff";
                            g.lineWidth = 2;
                            if (g.setLineDash) g.setLineDash([5, 4]);
                            curve(g, avg, 6);
                            if (g.setLineDash) g.setLineDash([]);
                        }
                        if (root.room.target !== "flat") {
                            const t = [];
                            for (let c = 0; c < 61; ++c)
                                t.push(root.target(20 * Math.pow(2, c / 6), root.room.target));
                            g.strokeStyle = "rgba(255, 255, 255, 0.35)";
                            g.lineWidth = 1;
                            curve(g, t, 6);
                        }
                    }
                    const eq = [];
                    for (let i = 0; i <= 120; ++i)
                        eq.push(root.response(20 * Math.pow(1000, i / 120)));
                    g.strokeStyle = root.bypassed ? "rgba(255, 213, 74, 0.35)" : String(Style.accent);
                    g.lineWidth = 2.5;
                    curve(g, eq, 12);
                    for (let b = 0; b < 31; ++b) {
                        g.fillStyle = b === drag.band ? String(Style.text) : (root.bypassed ? "rgba(255, 213, 74, 0.4)" : String(Style.accent));
                        g.beginPath();
                        g.arc(xOf(root.centers[b]), yOf(root.bands[b]), b === drag.band ? 7 : 3.5, 0, 2 * Math.PI);
                        g.fill();
                    }
                }

                MouseArea {
                    // drag the band under the finger up / down; a double tap: 0 dB
                    id: drag

                    property int band: -1

                    function nearest(x) {
                        let best = 0;
                        for (let b = 1; b < 31; ++b) {
                            if (Math.abs(graph.xOf(root.centers[b]) - x) < Math.abs(graph.xOf(root.centers[best]) - x))
                                best = b;
                        }
                        return best;
                    }
                    function name(b) {
                        const f = root.centers[b];
                        return f >= 1000 ? (f / 1000) + " kHz" : f + " Hz";
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
                        Mixxx.DJMantra.setRoomBand(band, Math.round(Math.max(-12, Math.min(12, graph.dbOf(mouse.y))) * 2) / 2);
                        root.refreshBands();
                    }
                    onReleased: {
                        if (band >= 0) {
                            Toasts.show(name(band) + " " + (root.bands[band] > 0 ? "+" : "") + root.bands[band].toFixed(1) + " dB");
                            root.keepCorrection();
                        }
                        band = -1;
                        graph.requestPaint();
                    }
                    onDoubleClicked: (mouse) => {
                        const b = nearest(mouse.x);
                        Mixxx.DJMantra.setRoomBand(b, 0);
                        root.refreshBands();
                        root.keepCorrection();
                        Toasts.show(name(b) + " → 0 dB");
                    }
                }
            }

            DButton {
                // one button, as IK's ARC: the next free place is measured, the average applied
                objectName: "roomAuto"
                width: parent.width
                height: 56
                text: root.measuring >= 0 ? qsTr("MEASURING PLACE ") + (root.measuring + 1) + " ..."
                      : root.nextEmpty() < 0 ? qsTr("ALL 7 PLACES MEASURED: APPLY BELOW")
                      : qsTr("AUTO CALIBRATE  (place ") + (root.nextEmpty() + 1) + qsTr(" of 7)")
                fontSize: 15
                lit: true
                onClicked: {
                    const next = root.nextEmpty();
                    if (next < 0) {
                        Toasts.show(qsTr("ALL 7 PLACES MEASURED: LONG PRESS ONE TO MEASURE IT AGAIN"));
                        return;
                    }
                    root.measure(next, true);
                }
            }

            Text {
                width: parent.width
                text: qsTr("PLACES: tap = in / out of the average; empty: measure; long press = measure again")
                color: Style.subText
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            Row {
                spacing: 4

                Repeater {
                    model: 7

                    delegate: DButton {
                        id: placeButton

                        required property int index
                        readonly property var point: root.room ? root.room.points[index] : null

                        objectName: "roomPlace" + (index + 1)
                        width: (content.width - 6 * 4) / 7
                        height: 52
                        mode: ""
                        text: root.measuring === index ? "..." : String(index + 1) + (point ? (point.use ? "\n✓" : "\n–") : "")
                        fontSize: 14
                        lit: point !== null && point.use
                        litColor: root.placeColours[index]
                        onClicked: {
                            if (!point) {
                                root.measure(index, false);
                                return;
                            }
                            root.setRoom((r) => { r.points[index].use = !r.points[index].use; });
                            Toasts.show(qsTr("PLACE ") + (index + 1) + (root.room.points[index].use ? qsTr(" IN THE AVERAGE") : qsTr(" LEFT OUT")));
                        }
                        onLongPressed: root.measure(index, false)
                    }
                }
            }

            Row {
                spacing: 6

                DButton {
                    objectName: "roomApply"
                    width: (content.width - 6) * 0.62
                    height: 50
                    text: qsTr("AVERAGE & APPLY")
                    lit: root.averageOf("thirds", 31) !== null
                    onClicked: root.apply()
                }

                DButton {
                    objectName: "roomFlat"
                    width: (content.width - 6) * 0.38
                    height: 50
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

    // --- the dropdowns ---------------------------------------------------------------

    Rectangle {
        // tap outside: closed
        anchors.fill: parent
        visible: root.panel === "more" || root.panel === "rooms"
        color: "#99000000"

        MouseArea {
            anchors.fill: parent
            onClicked: root.panel = ""
        }
    }

    Rectangle {
        id: morePanel

        visible: root.panel === "more"
        x: 10
        y: 56
        width: parent.width - 20
        height: Math.min(parent.height - 66, moreColumn.height + 20)
        radius: Style.radius
        color: Style.surface
        border.color: Style.accent

        MouseArea {
            anchors.fill: parent
        }

        Flickable {
            anchors.fill: parent
            anchors.margins: 10
            contentHeight: moreColumn.height
            clip: true

            Column {
                id: moreColumn

                width: parent.width
                spacing: 6

                component Choice: Row {
                    id: choice

                    property string title
                    property var options // [[value, label]]
                    property var value
                    signal chosen(var value)

                    spacing: 4
                    Text {
                        width: 92
                        height: 38
                        verticalAlignment: Text.AlignVCenter
                        text: choice.title
                        color: Style.subText
                        font.pixelSize: 11
                        font.bold: true
                        wrapMode: Text.WordWrap
                    }
                    Repeater {
                        model: choice.options

                        delegate: DButton {
                            required property var modelData

                            width: (moreColumn.width - 96 - 4 * choice.options.length) / choice.options.length
                            height: 38
                            mode: ""
                            text: modelData[1]
                            fontSize: 11
                            lit: choice.value === modelData[0]
                            onClicked: choice.chosen(modelData[0])
                        }
                    }
                }

                Choice {
                    title: qsTr("TARGET CURVE")
                    options: [["flat", qsTr("FLAT")], ["house", qsTr("HOUSE")], ["club", qsTr("CLUB")]]
                    value: root.room ? root.room.target : "flat"
                    onChosen: (v) => {
                        root.setRoom((r) => { r.target = v; });
                        Toasts.show(qsTr("TARGET → ") + v.toUpperCase() + qsTr(": AVERAGE & APPLY again"));
                    }
                }
                Choice {
                    title: qsTr("STRENGTH")
                    options: [[50, "50 %"], [75, "75 %"], [100, "100 %"]]
                    value: root.room ? root.room.strength : 100
                    onChosen: (v) => root.setRoom((r) => { r.strength = v; })
                }
                Choice {
                    title: qsTr("TEST SIGNAL")
                    options: [["sweep", qsTr("SWEEP")], ["noise", qsTr("PINK NOISE")]]
                    value: root.signalKind
                    onChosen: (v) => {
                        root.signalKind = v;
                        Mixxx.DJMantra.setSetting("room_eq_signal", v);
                    }
                }
                Choice {
                    title: qsTr("LENGTH")
                    options: [[5, "5 s"], [8, "8 s"], [12, "12 s"]]
                    value: root.seconds
                    onChosen: (v) => {
                        root.seconds = v;
                        Mixxx.DJMantra.setSetting("room_eq_seconds", v);
                    }
                }
                Choice {
                    title: qsTr("TEST LEVEL")
                    options: [[-30, "−30 dB"], [-24, "−24 dB"], [-18, "−18 dB"], [-12, "−12 dB"]]
                    value: root.level
                    onChosen: (v) => {
                        root.level = v;
                        Mixxx.DJMantra.setSetting("room_eq_level", v);
                    }
                }

                Text {
                    text: qsTr("MICROPHONE")
                    color: Style.subText
                    font.pixelSize: 11
                    font.bold: true
                }
                Repeater {
                    model: root.inputs

                    delegate: DButton {
                        required property var modelData

                        width: moreColumn.width
                        height: 38
                        mode: ""
                        text: modelData.name
                        fontSize: 12
                        lit: root.inputId === modelData.id
                        onClicked: {
                            root.inputId = modelData.id;
                            Mixxx.DJMantra.setSetting("room_eq_input", modelData.id);
                            if (root.analyzerOn) {
                                Mixxx.DJMantra.roomEqLive(false, 0);
                                Mixxx.DJMantra.roomEqLive(true, root.inputId);
                            }
                            Toasts.show(qsTr("MICROPHONE → ") + modelData.name);
                        }
                    }
                }

                Row {
                    spacing: 6

                    DButton {
                        width: (moreColumn.width - 6) / 2
                        height: 42
                        mode: ""
                        text: qsTr("ANALYZER")
                        fontSize: 12
                        lit: root.analyzerOn
                        onClicked: {
                            root.analyzerOn = !root.analyzerOn;
                            Mixxx.DJMantra.roomEqLive(root.analyzerOn, root.inputId);
                            if (!root.analyzerOn) {
                                root.live = [];
                                graph.requestPaint();
                            }
                        }
                    }

                    DButton {
                        objectName: "roomGuide"
                        width: (moreColumn.width - 6) / 2
                        height: 42
                        mode: ""
                        text: qsTr("CORRECTION GUIDE")
                        fontSize: 12
                        onClicked: root.panel = "guide"
                    }
                }
            }
        }
    }

    Rectangle {
        // the rooms
        visible: root.panel === "rooms"
        x: 10
        y: 100
        width: parent.width - 20
        height: Math.min(parent.height - 110, roomsColumn.height + 20)
        radius: Style.radius
        color: Style.surface
        border.color: Style.accent

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: roomsColumn

            x: 10
            y: 10
            width: parent.width - 20
            spacing: 6

            Repeater {
                model: root.rooms

                delegate: DButton {
                    required property var modelData
                    required property int index

                    width: roomsColumn.width
                    height: 40
                    mode: ""
                    text: modelData.name + "   (" + modelData.points.filter((p) => p).length + qsTr(" places)")
                    fontSize: 13
                    lit: index === root.current
                    onClicked: {
                        root.useRoom(index);
                        root.panel = "";
                        Toasts.show(qsTr("ROOM → ") + modelData.name);
                    }
                }
            }

            Rectangle {
                width: roomsColumn.width
                height: 40
                radius: Style.radius
                color: Style.tile

                TextInput {
                    id: roomName

                    objectName: "roomName"
                    anchors.fill: parent
                    anchors.margins: 10
                    verticalAlignment: TextInput.AlignVCenter
                    color: Style.text
                    font.pixelSize: 15
                    clip: true
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onAccepted: {
                        focus = false;
                        Qt.inputMethod.hide();
                    }

                    Text {
                        anchors.fill: parent
                        visible: roomName.text === "" && !roomName.activeFocus
                        text: qsTr("Name (a new room, or the new name)")
                        color: Style.line
                        font.pixelSize: 15
                    }
                }
            }

            Row {
                spacing: 6

                DButton {
                    width: (roomsColumn.width - 12) / 3
                    height: 42
                    mode: ""
                    text: qsTr("NEW ROOM")
                    fontSize: 12
                    onClicked: {
                        Qt.inputMethod.commit(); // r41: the keyboard's composing word ("Test Room" became "Test")
                        const name = roomName.text.trim() || (qsTr("Room ") + (root.rooms.length + 1));
                        roomName.focus = false;
                        Qt.inputMethod.hide();
                        root.rooms = root.rooms.concat([root.emptyRoom(name)]);
                        root.useRoom(root.rooms.length - 1);
                        root.saveRooms();
                        roomName.text = "";
                        root.panel = "";
                        Toasts.show(qsTr("NEW ROOM: ") + name);
                    }
                }

                DButton {
                    width: (roomsColumn.width - 12) / 3
                    height: 42
                    mode: ""
                    text: qsTr("RENAME")
                    fontSize: 12
                    onClicked: {
                        Qt.inputMethod.commit();
                        const name = roomName.text.trim();
                        roomName.focus = false;
                        Qt.inputMethod.hide();
                        if (name === "") {
                            Toasts.show(qsTr("TYPE THE NEW NAME FIRST"));
                            return;
                        }
                        Mixxx.DJMantra.roomDelete(root.room.name);
                        root.setRoom((r) => { r.name = name; });
                        roomName.text = "";
                        Toasts.show(qsTr("ROOM → ") + name);
                    }
                }

                DButton {
                    width: (roomsColumn.width - 12) / 3
                    height: 42
                    mode: ""
                    property bool armed: false

                    text: armed ? qsTr("SURE? TAP") : qsTr("DELETE")
                    fontSize: 12
                    lit: armed
                    Timer {
                        id: disarm

                        interval: 3000
                        onTriggered: parent.armed = false
                    }
                    onClicked: {
                        // r41: no confirmation: the first tap asks, the second (within 3 s) deletes
                        if (!armed) {
                            armed = true;
                            disarm.restart();
                            Toasts.show(qsTr("TAP AGAIN TO DELETE ") + root.room.name);
                            return;
                        }
                        armed = false;
                        Mixxx.DJMantra.roomDelete(root.room.name);
                        if (root.rooms.length <= 1) {
                            root.rooms = [root.emptyRoom(qsTr("My room"))];
                        } else {
                            const copy = root.rooms.slice();
                            copy.splice(root.current, 1);
                            root.rooms = copy;
                        }
                        root.useRoom(0);
                        root.saveRooms();
                        root.panel = "";
                        Toasts.show(qsTr("ROOM DELETED"));
                    }
                }
            }
        }
    }

    Rectangle {
        // the CORRECTION GUIDE
        anchors.fill: parent
        visible: root.panel === "guide"
        color: Style.background

        MouseArea {
            anchors.fill: parent
        }

        Item {
            id: guideHead

            x: 10
            width: parent.width - 20
            height: 54

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("CORRECTION GUIDE")
                color: Style.accent
                font.pixelSize: 17
                font.bold: true
            }

            CloseButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: root.panel = ""
            }
        }

        Flickable {
            anchors.top: guideHead.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 10
            contentHeight: guideText.height + 20
            clip: true

            Text {
                id: guideText

                width: parent.width
                wrapMode: Text.WordWrap
                textFormat: Text.StyledText
                color: Style.text
                font.pixelSize: 14
                lineHeight: 1.15
                text: qsTr("<b>WHAT IT DOES</b><br>"
                    + "Every room adds its own colour: bass that booms on some notes, corners that swallow others. "
                    + "Room correction measures what the room does to a known test sound and puts the opposite "
                    + "on the main output (31 bands, a third of an octave each). The correction only ever cuts: "
                    + "the speakers are never pushed harder. The recording of your mix stays uncorrected.<br><br>"
                    + "<b>BEFORE YOU START</b><br>"
                    + "1. Pause the music. Make the room as it will be (doors, curtains).<br>"
                    + "2. The PA at the level you will play. TEST LEVEL in MORE sets the test signal on the mixer: "
                    + "start at −18 dB. Too loud: the result says the microphone was overloaded.<br>"
                    + "3. MICROPHONE in MORE: the phone's own, or a measurement microphone on USB-C "
                    + "(better: flat from 20 Hz to 20 kHz). The phone's microphone is good from about 100 Hz "
                    + "to 10 kHz; below and above, trust the result less.<br>"
                    + "4. The room quiet while it measures: no talking, no air conditioning if you can.<br><br>"
                    + "<b>WHICH TEST SIGNAL</b><br>"
                    + "SWEEP (recommended): a tone gliding from 20 Hz to 20 kHz. The app turns it into the room's "
                    + "impulse response and keeps its first half second, so background noise and late echoes "
                    + "matter much less. Use 8 s; 12 s in a noisy room.<br>"
                    + "PINK NOISE: a hiss with the same energy in every octave, analysed as an RTA. Use it when "
                    + "a sweep sounds odd through a limiter or a Bluetooth link.<br><br>"
                    + "<b>WHERE TO MEASURE (the places)</b><br>"
                    + "Hold the phone at ear height, microphone pointing up to the ceiling, arm stretched, "
                    + "body still. Measure where people listen, not at the speakers:<br>"
                    + "• 1: the middle of the dance floor (the main place).<br>"
                    + "• 2-5: about 1 m to the left, right, front and back of it.<br>"
                    + "• 6-7: two more spots where people dance or sit, not closer than 1 m to a wall.<br>"
                    + "Spread the places apart: a single point shows that point's quirks; several places show "
                    + "the room. Avoid corners and right next to the speakers.<br><br>"
                    + "<b>AVERAGING (your own choice of places)</b><br>"
                    + "Each place you measure is kept with the room. Tap a place to put it in or leave it out of "
                    + "the average, then AVERAGE & APPLY and listen. If one place sounds best on its own, use only "
                    + "that one. The dashed white line is the average of the places in it; the coloured lines are "
                    + "the places.<br><br>"
                    + "<b>THE ALGORITHM (MORE)</b><br>"
                    + "TARGET CURVE: FLAT – the room made neutral (reference, monitoring). HOUSE – the "
                    + "gentle curve most listeners prefer: a few dB more bass, a slightly softer top. CLUB – "
                    + "more sub-bass and a softer top, for dance music on a PA.<br>"
                    + "STRENGTH: 100 % corrects fully; 50-75 % is gentler and often sounds more natural.<br>"
                    + "What the algorithm does: it averages the places' power, makes it relative to the mids "
                    + "(200 Hz - 2 kHz), compares it with the target, and corrects at most −12 / +6 dB per band. "
                    + "It does not correct below the speakers' lowest note (where the room is more than 12 dB "
                    + "down: boosting there only overloads them) nor above 16 kHz, it smooths neighbouring bands, "
                    + "and finally lowers everything so the highest band is 0 dB.<br><br>"
                    + "<b>ONE BUTTON</b><br>"
                    + "AUTO CALIBRATE measures the next free place and applies the average at once. Press it, move "
                    + "to the next place, press again. BYPASS compares with and without the correction.<br><br>"
                    + "<b>ROOMS</b><br>"
                    + "Every venue can have its own room: ROOM at the top chooses, adds, renames or deletes one. "
                    + "A room keeps its places, target, strength and correction, saved by itself as a file of its own: "
                    + "Documents/DJ Mantra/Room_Corrections/&lt;room&gt;.json (SAVE also saves and shows the file). Choosing a "
                    + "room brings its correction back.<br><br>"
                    + "<b>BY HAND</b><br>"
                    + "Drag any band up or down on the graph; double tap it for 0 dB. FLAT clears the correction.")
            }
        }
    }
}
