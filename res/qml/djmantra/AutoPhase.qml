import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// AUTO PHASE (Marko, 12.10.2026: "out of phase alignment is the function of
// this application, the special one"): while both decks play at the same
// tempo, their phrases are kept aligned by themselves: beat 1 with beat 1,
// beat 8 with beat 8. When they differ, one deck jumps on its next beat to the
// other's beat (seamless, quantized). Which deck moves: the other one when
// Marko just moved a deck himself (his jump leads), else the one heard less
// (crossfader), so the deck on air keeps playing untouched.
Item {
    id: root

    property bool active: true
    property var deck1 // MatrixDeck
    property var deck2
    property real lastAlign: 0

    Mixxx.ControlProxy { id: fader; group: "[Master]"; key: "crossfader" }
    Mixxx.ControlProxy { id: volume1; group: "[Channel1]"; key: "volume" }
    Mixxx.ControlProxy { id: volume2; group: "[Channel2]"; key: "volume" }
    // round 24: a CUE preview (held) plays the deck: not a deck to align with
    Mixxx.ControlProxy {
        id: cue1
        group: "[Channel1]"; key: "cue_default"
        onValueChanged: root.previewEnded = Date.now()
    }
    Mixxx.ControlProxy {
        id: cue2
        group: "[Channel2]"; key: "cue_default"
        onValueChanged: root.previewEnded = Date.now()
    }
    property real previewEnded: 0

    // heard on the dance floor: volume up, not cut by the crossfader
    function onAir(deck) {
        if (deck === deck1)
            return volume1.value > 0.05 && fader.value < 0.98;
        return volume2.value > 0.05 && fader.value > -0.98;
    }

    function check() {
        if (!active || !deck1 || !deck2 || !deck1.playing || !deck2.playing)
            return;
        if (deck1.phraseBeat < 0 || deck2.phraseBeat < 0 || deck1.cells !== deck2.cells)
            return;
        // the same tempo (synced) only: else the beats do not line up anyway
        if (deck1.bpmNow <= 0 || Math.abs(deck1.bpmNow - deck2.bpmNow) / deck1.bpmNow > 0.01)
            return;
        if (deck1.phraseBeat === deck2.phraseBeat || deck1.queuedBeat >= 0 || deck2.queuedBeat >= 0)
            return;
        if (cue1.value > 0 || cue2.value > 0 || Date.now() - previewEnded < 1000)
            return; // a CUE preview: never jump a deck for it
        // after an alignment, give it two beats to land
        if (Date.now() - lastAlign < 2 * 60000 / deck1.bpmNow)
            return;
        let follower;
        const now = Date.now();
        const air1 = onAir(deck1), air2 = onAir(deck2);
        if (air1 !== air2)
            follower = air1 ? deck2 : deck1; // round 24: the deck on air never moves
        else if (now - deck1.userMovedAt < 4000 || now - deck2.userMovedAt < 4000)
            follower = deck1.userMovedAt > deck2.userMovedAt ? deck2 : deck1;
        else
            follower = fader.value <= 0 ? deck2 : deck1; // the one heard less
        lastAlign = now;
        Mixxx.DJMantra.log("auto phase: " + follower.group + " (beat " + (follower.phraseBeat + 1) +
                ") to the other deck's beat");
        follower.align();
    }

    Timer {
        // a moment after a beat change, when both decks have moved on
        id: settle

        interval: 120
        onTriggered: root.check()
    }

    Connections {
        target: root.deck1
        function onPhraseBeatChanged() { settle.restart(); }
    }

    onActiveChanged: Mixxx.DJMantra.log("auto phase " + (active ? "on" : "off"))
}
