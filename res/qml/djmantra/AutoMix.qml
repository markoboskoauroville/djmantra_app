import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Half-automatic transitions for long organic blends (Marko, 12.10.2026).
// AUTO MIX from the playing deck to the other one:
//   - the incoming deck starts on its beat 1 with the playing deck's next
//     beat 1 (sync on)
//   - AUTO EQ: over the chosen bars the bass is swapped first, then the
//     highs, then the mids; in the last quarter the outgoing deck fades out
//   - TEMPO BLEND (as djay Pro): the tempo glides from the outgoing song's to
//     the incoming song's own over the blend
// At the end the outgoing deck stops and gets its EQ and volume back.
Rectangle {
    id: root

    property bool eq: true
    property bool tempo: true
    property int bars: 16
    property bool running: false
    property real progress: 0
    property string from: ""
    property string to: ""
    property real startBpm: 0
    property real targetBpm: 0
    property real startedAt: 0
    property bool waiting: false

    implicitHeight: 52
    radius: Style.radius
    color: Style.surface

    function eqGroup(deck) { return "[EqualizerRack1_" + deck + "_Effect1]"; }

    Mixxx.ControlProxy { id: play1; group: "[Channel1]"; key: "play" }
    Mixxx.ControlProxy { id: play2; group: "[Channel2]"; key: "play" }
    Mixxx.ControlProxy { id: vol1; group: "[Channel1]"; key: "volume" }
    Mixxx.ControlProxy { id: vol2; group: "[Channel2]"; key: "volume" }
    Mixxx.ControlProxy { id: bpm1; group: "[Channel1]"; key: "bpm" }
    Mixxx.ControlProxy { id: bpm2; group: "[Channel2]"; key: "bpm" }
    Mixxx.ControlProxy { id: fileBpm1; group: "[Channel1]"; key: "file_bpm" }
    Mixxx.ControlProxy { id: fileBpm2; group: "[Channel2]"; key: "file_bpm" }
    Mixxx.ControlProxy { id: sync1; group: "[Channel1]"; key: "sync_enabled" }
    Mixxx.ControlProxy { id: sync2; group: "[Channel2]"; key: "sync_enabled" }
    Mixxx.ControlProxy { id: quant1; group: "[Channel1]"; key: "quantize" }
    Mixxx.ControlProxy { id: quant2; group: "[Channel2]"; key: "quantize" }
    Mixxx.ControlProxy { id: jump1; group: "[Channel1]"; key: "phrase_jump" }
    Mixxx.ControlProxy { id: jump2; group: "[Channel2]"; key: "phrase_jump" }
    Mixxx.ControlProxy { id: beat1; group: "[Channel1]"; key: "beat_in_phrase"; onValueChanged: root.checkStart("[Channel1]", value) }
    Mixxx.ControlProxy { id: beat2; group: "[Channel2]"; key: "beat_in_phrase"; onValueChanged: root.checkStart("[Channel2]", value) }
    Mixxx.ControlProxy { id: fader; group: "[Master]"; key: "crossfader" }
    Mixxx.ControlProxy { id: lo1; group: root.eqGroup("[Channel1]"); key: "parameter1" }
    Mixxx.ControlProxy { id: mi1; group: root.eqGroup("[Channel1]"); key: "parameter2" }
    Mixxx.ControlProxy { id: hi1; group: root.eqGroup("[Channel1]"); key: "parameter3" }
    Mixxx.ControlProxy { id: lo2; group: root.eqGroup("[Channel2]"); key: "parameter1" }
    Mixxx.ControlProxy { id: mi2; group: root.eqGroup("[Channel2]"); key: "parameter2" }
    Mixxx.ControlProxy { id: hi2; group: root.eqGroup("[Channel2]"); key: "parameter3" }

    function deckControls(deck) {
        return deck === "[Channel1]"
                ? {play: play1, vol: vol1, bpm: bpm1, fileBpm: fileBpm1, sync: sync1, quant: quant1, jump: jump1, lo: lo1, mi: mi1, hi: hi1}
                : {play: play2, vol: vol2, bpm: bpm2, fileBpm: fileBpm2, sync: sync2, quant: quant2, jump: jump2, lo: lo2, mi: mi2, hi: hi2};
    }

    function start() {
        if (running) {
            stop();
            return;
        }
        // from the playing deck (both play: the one heard more) to the other
        if (play1.value > 0 && play2.value > 0)
            from = vol1.value * (fader.value <= 0 ? 1 : 1 - fader.value) >= vol2.value * (fader.value >= 0 ? 1 : 1 + fader.value) ? "[Channel1]" : "[Channel2]";
        else if (play1.value > 0)
            from = "[Channel1]";
        else if (play2.value > 0)
            from = "[Channel2]";
        else
            return; // nothing plays: nothing to mix from
        to = from === "[Channel1]" ? "[Channel2]" : "[Channel1]";
        const f = deckControls(from);
        const t = deckControls(to);
        startBpm = f.bpm.value;
        targetBpm = t.fileBpm.value > 0 ? t.fileBpm.value : startBpm;
        fader.value = 0;
        t.vol.value = 1;
        if (eq) {
            t.lo.parameter = 0;
            t.mi.parameter = 0;
            t.hi.parameter = 0;
        }
        t.sync.value = 1;
        t.quant.value = 1;
        progress = 0;
        running = true;
        Mixxx.DJMantra.log("auto mix " + from + " > " + to + " " + bars + " bars eq " + eq + " tempo " + tempo);
        if (t.play.value > 0) {
            startedAt = Date.now();
            waiting = false;
        } else {
            // the incoming deck waits on its beat 1 for the outgoing deck's next beat 1
            t.jump.value = 1;
            waiting = true;
        }
    }

    function checkStart(deck, value) {
        if (running && waiting && deck === from && value === 0) {
            deckControls(to).play.value = 1;
            waiting = false;
            startedAt = Date.now();
        }
    }

    function stop() {
        // back to normal: both decks' EQs at the middle, both volumes up
        for (const deck of ["[Channel1]", "[Channel2]"]) {
            const d = deckControls(deck);
            d.lo.parameter = 0.5;
            d.mi.parameter = 0.5;
            d.hi.parameter = 0.5;
        }
        running = false;
        waiting = false;
    }

    function finish() {
        const f = deckControls(from);
        f.play.value = 0;
        stop();
        f.vol.value = 1;
        Mixxx.DJMantra.log("auto mix done: " + to + " plays");
    }

    Timer {
        interval: 50
        repeat: true
        running: root.running && !root.waiting
        onTriggered: {
            const bpm = Math.max(40, root.startBpm);
            const seconds = root.bars * 4 * 60 / bpm;
            const p = Math.min(1, (Date.now() - root.startedAt) / 1000 / seconds);
            root.progress = p;
            const f = root.deckControls(root.from);
            const t = root.deckControls(root.to);
            const part = (a, b) => Math.max(0, Math.min(1, (p - a) / (b - a)));
            if (root.eq) {
                const bass = part(0, 0.25), highs = part(0.25, 0.5), mids = part(0.5, 0.75);
                t.lo.parameter = 0.5 * bass;
                f.lo.parameter = 0.5 * (1 - bass);
                t.hi.parameter = 0.5 * highs;
                f.hi.parameter = 0.5 * (1 - highs);
                t.mi.parameter = 0.5 * mids;
                f.mi.parameter = 0.5 * (1 - mids);
                f.vol.value = 1 - part(0.75, 1);
            } else {
                t.vol.value = part(0, 0.5);
                f.vol.value = 1 - part(0.5, 1);
            }
            if (root.tempo && root.targetBpm > 0)
                f.bpm.value = root.startBpm + (root.targetBpm - root.startBpm) * p;
            if (p >= 1)
                root.finish();
        }
    }

    Row {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        DButton {
            objectName: "autoMix"
            width: 96
            height: parent.height
            text: root.running ? (root.waiting ? qsTr("WAIT 1") : Math.round(root.progress * 100) + " %") : qsTr("AUTO MIX")
            fontSize: 13
            lit: true
            onClicked: root.start()
        }

        DButton {
            objectName: "autoEq"
            width: 58
            height: parent.height
            text: qsTr("EQ")
            fontSize: 12
            lit: root.eq
            onClicked: root.eq = !root.eq
        }

        DButton {
            objectName: "tempoBlend"
            width: 72
            height: parent.height
            text: qsTr("TEMPO\nBLEND")
            fontSize: 11
            lit: root.tempo
            onClicked: root.tempo = !root.tempo
        }

        DButton {
            objectName: "autoBars"
            width: parent.width - 96 - 58 - 72 - 3 * 6
            height: parent.height
            text: root.bars + qsTr(" BARS")
            fontSize: 12
            onClicked: root.bars = root.bars === 8 ? 16 : root.bars === 16 ? 32 : 8
        }
    }
}
