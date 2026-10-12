import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The mixing modes (Marko, 12.10.2026): "automix in my case doesn't mean that
// the app will automix the song. It means that we'll do EQ and tempo blend as
// automix. So I'm manually moving the fader."
//   AUTOMIX on: the crossfader drives the blend. Moving it away from the
//     deck it rests on (the outgoing one):
//   EQ: the bass is swapped first, then the highs, then the mids (cut-only
//     EQs: 1 = 0 dB, 0 = kill); moving back undoes it
//   TEMPO BLEND: the tempo glides from the outgoing song's to the incoming
//     song's own along the fader's travel
//   8 / 16 / 32: how many beats the phrase has (the matrix's running lights)
Rectangle {
    id: root

    property bool automix: false
    property bool eq: true
    property bool tempo: true
    property int beats: 8
    // the deck the fader left (outgoing); set while the fader rests at an end
    property string from: "[Channel1]"
    property real startBpm: 0
    property real progress: 0 // 0: all outgoing, 1: all incoming

    implicitHeight: 40
    radius: Style.radius
    color: "transparent"

    function eqGroup(deck) { return "[EqualizerRack1_" + deck + "_Effect1]"; }

    Mixxx.ControlProxy { id: bpm1; group: "[Channel1]"; key: "bpm" }
    Mixxx.ControlProxy { id: bpm2; group: "[Channel2]"; key: "bpm" }
    Mixxx.ControlProxy { id: fileBpm1; group: "[Channel1]"; key: "file_bpm" }
    Mixxx.ControlProxy { id: fileBpm2; group: "[Channel2]"; key: "file_bpm" }
    Mixxx.ControlProxy { id: length1; group: "[Channel1]"; key: "phrase_length" }
    Mixxx.ControlProxy { id: length2; group: "[Channel2]"; key: "phrase_length" }
    Mixxx.ControlProxy { id: lo1; group: root.eqGroup("[Channel1]"); key: "parameter1" }
    Mixxx.ControlProxy { id: mi1; group: root.eqGroup("[Channel1]"); key: "parameter2" }
    Mixxx.ControlProxy { id: hi1; group: root.eqGroup("[Channel1]"); key: "parameter3" }
    Mixxx.ControlProxy { id: lo2; group: root.eqGroup("[Channel2]"); key: "parameter1" }
    Mixxx.ControlProxy { id: mi2; group: root.eqGroup("[Channel2]"); key: "parameter2" }
    Mixxx.ControlProxy { id: hi2; group: root.eqGroup("[Channel2]"); key: "parameter3" }
    Mixxx.ControlProxy {
        id: fader

        group: "[Master]"
        key: "crossfader"
        onValueChanged: root.follow()
    }

    function deckControls(deck) {
        return deck === "[Channel1]"
                ? {bpm: bpm1, fileBpm: fileBpm1, lo: lo1, mi: mi1, hi: hi1}
                : {bpm: bpm2, fileBpm: fileBpm2, lo: lo2, mi: mi2, hi: hi2};
    }

    function setBeats(n) {
        beats = n;
        length1.value = n;
        length2.value = n;
        Mixxx.DJMantra.log("phrase beats " + n);
    }

    function resetEq() {
        for (const deck of ["[Channel1]", "[Channel2]"]) {
            const d = deckControls(deck);
            d.lo.parameter = 1;
            d.mi.parameter = 1;
            d.hi.parameter = 1;
        }
    }

    function rest() {
        // the fader at an end (or AUTOMIX just on): that deck is the outgoing one
        const x = fader.value;
        from = x <= 0 ? "[Channel1]" : "[Channel2]";
        startBpm = deckControls(from).bpm.value;
    }

    function follow() {
        if (!automix)
            return;
        const x = fader.value; // -1 deck 1 only, 1 deck 2 only
        if (x <= -0.98 || x >= 0.98) {
            const was = from;
            rest();
            if (was !== from)
                Mixxx.DJMantra.log("automix: " + from + " is the outgoing deck");
        }
        const p = from === "[Channel1]" ? (x + 1) / 2 : (1 - x) / 2;
        progress = Math.max(0, Math.min(1, p));
        const f = deckControls(from);
        const t = deckControls(from === "[Channel1]" ? "[Channel2]" : "[Channel1]");
        if (eq) {
            const part = (a, b) => Math.max(0, Math.min(1, (progress - a) / (b - a)));
            const bass = part(0.05, 0.35), highs = part(0.35, 0.65), mids = part(0.65, 0.95);
            t.lo.parameter = bass;
            f.lo.parameter = 1 - bass;
            t.hi.parameter = highs;
            f.hi.parameter = 1 - highs;
            t.mi.parameter = mids;
            f.mi.parameter = 1 - mids;
        }
        if (tempo && startBpm > 0 && t.fileBpm.value > 0)
            f.bpm.value = startBpm + (t.fileBpm.value - startBpm) * progress;
    }

    onAutomixChanged: {
        Mixxx.DJMantra.log("automix " + (automix ? "on" : "off") + " eq " + eq + " tempo " + tempo);
        if (automix) {
            rest();
            follow();
        } else {
            resetEq();
        }
    }
    onEqChanged: {
        if (!eq)
            resetEq();
        else
            follow();
    }

    Component.onCompleted: setBeats(beats)

    Row {
        anchors.fill: parent
        spacing: 4

        DButton {
            objectName: "autoMix"
            width: (parent.width - 3 * parent.spacing) * 0.34
            height: parent.height
            text: root.automix && root.progress > 0.01 && root.progress < 0.99
                    ? qsTr("AUTOMIX") + " " + Math.round(root.progress * 100) + "%" : qsTr("AUTOMIX")
            fontSize: 12
            lit: root.automix
            onClicked: root.automix = !root.automix
        }

        DButton {
            objectName: "autoEq"
            width: (parent.width - 3 * parent.spacing) * 0.18
            height: parent.height
            text: qsTr("EQ")
            fontSize: 12
            lit: root.eq
            onClicked: root.eq = !root.eq
        }

        DButton {
            objectName: "tempoBlend"
            width: (parent.width - 3 * parent.spacing) * 0.26
            height: parent.height
            text: qsTr("TEMPO BLEND")
            fontSize: 11
            lit: root.tempo
            onClicked: root.tempo = !root.tempo
        }

        DButton {
            // the phrase length: the matrix's cells
            objectName: "phraseBeats"
            width: (parent.width - 3 * parent.spacing) * 0.22
            height: parent.height
            text: root.beats + qsTr(" BEATS")
            fontSize: 11
            onClicked: root.setBeats(root.beats === 8 ? 16 : root.beats === 16 ? 32 : 8)
        }
    }
}
