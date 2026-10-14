import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The mixing modes (Marko, 12.10.2026): "automix in my case doesn't mean that
// the app will automix the song. It means that we'll do EQ and tempo blend as
// automix. So I'm manually moving the fader."
//   AUTOMIX on: the crossfader drives the blend. Moving it away from the
//     deck it rests on (the outgoing one):
//   EQ: as DJs mix on the EQs: the incoming highs come in first, the mids
//     follow (never killed), the basses are swapped quickly at the middle
//     (one bassline at a time), the outgoing highs and mids go last; moving
//     back undoes it (see follow())
//   TEMPO BLEND: the tempo glides from the outgoing song's to the incoming
//     song's own along the fader's travel
//   8 / 16 / 32: how many beats the phrase has (the matrix's running lights)
// Marko 14.10.2026: EQ and TEMPO BLEND work whenever they are lit, also when the
// crossfader is moved by hand (AUTO PHASE as before, on its own); AUTOMIX is now an
// ACTION: a tap moves the crossfader by itself from the deck it rests on to the other
// over the time set in Settings (AUTOMIX FADE: beats of the outgoing deck, or seconds),
// with the lit blends applied on the way; a tap while it moves stops it there.
Rectangle {
    id: root

    // the blends follow the crossfader while EQ or TEMPO BLEND is lit
    readonly property bool automix: eq || tempo
    // the AUTOMIX action: the crossfader moving by itself
    property bool fading: false
    property real fadeFrom: -1
    property real fadeTo: 1
    property double fadeStart: 0
    property real fadeMs: 1
    property bool eq: true
    property bool tempo: true
    property int beats: 8
    property var phase // AutoPhase
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

    // AUTO-PLAY (Marko 14.10.2026, djay's "Auto-play when moving crossfader", Settings ->
    // General -> Crossfader FX): the crossfader leaving the end it rests on, towards a
    // paused deck with a song, starts that deck (quantized: on the beat of the playing one)
    // armed once the fader has rested at an end (not by the first touch of a centred fader)
    property bool armed: false
    Mixxx.ControlProxy { id: play1; group: "[Channel1]"; key: "play" }
    Mixxx.ControlProxy { id: play2; group: "[Channel2]"; key: "play" }
    Mixxx.ControlProxy { id: loaded1; group: "[Channel1]"; key: "track_loaded" }
    Mixxx.ControlProxy { id: loaded2; group: "[Channel2]"; key: "track_loaded" }
    Mixxx.ControlProxy { id: quantize1; group: "[Channel1]"; key: "quantize" }
    Mixxx.ControlProxy { id: quantize2; group: "[Channel2]"; key: "quantize" }

    function checkAutoPlay(x) {
        if (Math.abs(x) >= 0.95) {
            armed = true;
            return;
        }
        if (!armed || Mixxx.DJMantra.setting("crossfader_autoplay", "1") !== "1")
            return;
        armed = false;
        // the deck the fader moves towards: away from the end it rested on
        const towards2 = from === "[Channel1]";
        const play = towards2 ? play2 : play1;
        const loaded = towards2 ? loaded2 : loaded1;
        if (play.value > 0 || loaded.value <= 0)
            return;
        (towards2 ? quantize2 : quantize1).value = 1;
        play.value = 1;
        const deck = towards2 ? 2 : 1;
        Toasts.show(qsTr("AUTO-PLAY \u2192 DECK ") + deck);
        Mixxx.DJMantra.log("crossfader auto-play: deck " + deck + " started at " + x.toFixed(2));
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
            d.lo.value = 1;
            d.mi.value = 1;
            d.hi.value = 1;
        }
    }

    function rest() {
        // the fader at an end (or AUTOMIX just on): that deck is the outgoing one
        const x = fader.value;
        from = x <= 0 ? "[Channel1]" : "[Channel2]";
        startBpm = deckControls(from).bpm.value;
    }

    property int loggedStep: -1

    function follow() {
        const x = fader.value; // -1 deck 1 only, 1 deck 2 only
        if (x <= -0.98 || x >= 0.98) {
            const was = from;
            rest();
            if (was !== from)
                Mixxx.DJMantra.log("automix: " + from + " is the outgoing deck");
        }
        const p = from === "[Channel1]" ? (x + 1) / 2 : (1 - x) / 2;
        progress = Math.max(0, Math.min(1, p));
        checkAutoPlay(x);
        if (!automix)
            return;
        const f = deckControls(from);
        const t = deckControls(from === "[Channel1]" ? "[Channel2]" : "[Channel1]");
        if (eq) {
            // Round 22 (Marko: "mine is a little hollow"): the old curves cut
            // whole bands of BOTH songs at once (both basses at -20 dB in the
            // middle of a long bass fade, the incoming mids killed until the
            // last third): a hole in the sound. Now, as DJs mix on the EQs
            // (one bassline at a time, swapped at the middle; the highs come
            // in first; the mids never killed), in linear gain (1 = 0 dB):
            //   highs: incoming from -18 dB to 0 dB over 5-45 %, outgoing from
            //          0 dB to -18 dB over 55-95 %
            //   mids:  incoming from -12 dB to 0 dB over 15-55 %, outgoing
            //          from 0 dB to -12 dB over 45-85 %
            //   bass:  killed on the incoming deck until the swap, a short
            //          amplitude crossfade over 45-55 % (the kicks are in phase,
            //          so in + out = 1 keeps the low end even), then killed on
            //          the outgoing deck
            const q = progress;
            const ramp = (a, b) => {
                const u = Math.max(0, Math.min(1, (q - a) / (b - a)));
                return u * u * (3 - 2 * u); // smoothstep: no corner at the ends
            };
            const db = (d) => Math.pow(10, d / 20);
            const fade = (fromDb, toDb, u) => db(fromDb + (toDb - fromDb) * u);
            const highsIn = ramp(0.05, 0.45), highsOut = ramp(0.55, 0.95);
            const midsIn = ramp(0.15, 0.55), midsOut = ramp(0.45, 0.85);
            const bass = ramp(0.45, 0.55);
            t.hi.value = fade(-18, 0, highsIn);
            f.hi.value = fade(0, -18, highsOut);
            t.mi.value = fade(-12, 0, midsIn);
            f.mi.value = fade(0, -12, midsOut);
            t.lo.value = bass;
            f.lo.value = 1 - bass;
            // round 22: the values in the log, every tenth of the fade
            const step = Math.round(q * 10);
            if (step !== loggedStep) {
                loggedStep = step;
                const v = (c) => c.value.toFixed(2);
                Mixxx.DJMantra.log("automix eq " + Math.round(q * 100) + "%: out hi " + v(f.hi) +
                        " mid " + v(f.mi) + " low " + v(f.lo) + " | in hi " + v(t.hi) +
                        " mid " + v(t.mi) + " low " + v(t.lo));
            }
        }
        if (tempo && startBpm > 0 && t.fileBpm.value > 0)
            f.bpm.value = startBpm + (t.fileBpm.value - startBpm) * progress;
    }

    // the fade time from Settings: beats of the outgoing deck (its tempo now) or seconds
    function fadeTime() {
        const unit = Mixxx.DJMantra.setting("automix_fade_unit", "beats");
        const amount = Number(Mixxx.DJMantra.setting("automix_fade_amount", "16"));
        if (unit === "seconds")
            return {ms: amount * 1000, text: amount + " s"};
        const bpm = deckControls(fader.value <= 0 ? "[Channel1]" : "[Channel2]").bpm.value;
        return {ms: amount * 60000 / (bpm > 0 ? bpm : 120), text: amount + qsTr(" beats")};
    }

    function startFade() {
        if (fading) {
            fading = false;
            Toasts.show(qsTr("AUTOMIX STOPPED ") + Math.round(progress * 100) + "%");
            Mixxx.DJMantra.log("automix: stopped at " + fader.value.toFixed(2));
            return;
        }
        rest();
        const t = fadeTime();
        fadeFrom = fader.value;
        fadeTo = fader.value <= 0 ? 1 : -1;
        fadeMs = Math.max(200, t.ms * Math.abs(fadeTo - fadeFrom) / 2);
        fadeStart = Date.now();
        fading = true;
        Toasts.show(qsTr("AUTOMIX \u2192 DECK ") + (fadeTo > 0 ? 2 : 1) + "  (" + t.text + ")");
        Mixxx.DJMantra.log("automix: crossfader " + fadeFrom.toFixed(2) + " -> " + fadeTo + " in "
                + Math.round(fadeMs) + " ms (" + t.text + ")");
    }

    Timer {
        // the crossfader's glide, every frame
        interval: 16
        repeat: true
        running: root.fading
        onTriggered: {
            const u = Math.min(1, (Date.now() - root.fadeStart) / root.fadeMs);
            fader.value = root.fadeFrom + (root.fadeTo - root.fadeFrom) * u;
            if (u >= 1) {
                root.fading = false;
                Mixxx.DJMantra.log("automix: done, " + root.from + " out");
            }
        }
    }

    onAutomixChanged: {
        Mixxx.DJMantra.log("blend " + (automix ? "on" : "off") + " eq " + eq + " tempo " + tempo);
        if (automix) {
            rest();
            follow();
        } else {
            resetEq();
        }
    }
    onEqChanged: {
        Toasts.show(eq ? qsTr("EQ BLEND ON") : qsTr("EQ BLEND OFF"));
        if (!eq)
            resetEq();
        else
            follow();
    }
    onTempoChanged: Toasts.show(tempo ? qsTr("TEMPO BLEND ON") : qsTr("TEMPO BLEND OFF"))

    Component.onCompleted: setBeats(beats)

    Row {
        anchors.fill: parent
        spacing: 4

        DButton {
            objectName: "autoMix"
            width: (parent.width - 4 * parent.spacing) * 0.22
            height: parent.height
            text: root.fading ? qsTr("AUTOMIX") + " " + Math.round(root.progress * 100) + "%" : qsTr("AUTOMIX")
            fontSize: 12
            lit: root.fading
            onClicked: root.startFade()
        }

        DButton {
            objectName: "autoEq"
            width: (parent.width - 4 * parent.spacing) * 0.12
            height: parent.height
            text: qsTr("EQ")
            fontSize: 12
            lit: root.eq
            onClicked: root.eq = !root.eq
        }

        DButton {
            objectName: "tempoBlend"
            width: (parent.width - 4 * parent.spacing) * 0.22
            height: parent.height
            text: qsTr("TEMPO BLEND")
            fontSize: 11
            lit: root.tempo
            onClicked: root.tempo = !root.tempo
        }

        DButton {
            // the decks' phrases kept aligned by themselves (AutoPhase.qml)
            objectName: "autoPhase"
            width: (parent.width - 4 * parent.spacing) * 0.22
            height: parent.height
            text: qsTr("AUTO PHASE")
            fontSize: 11
            lit: root.phase ? root.phase.active : false
            onClicked: {
                if (!root.phase)
                    return;
                root.phase.active = !root.phase.active;
                Toasts.show(root.phase.active ? qsTr("AUTO PHASE ON") : qsTr("AUTO PHASE OFF"));
            }
        }

        DButton {
            // the phrase length: the matrix's cells
            objectName: "phraseBeats"
            width: (parent.width - 4 * parent.spacing) * 0.22
            height: parent.height
            text: root.beats + qsTr(" BEATS")
            fontSize: 11
            onClicked: {
                root.setBeats(root.beats === 8 ? 16 : root.beats === 16 ? 32 : 8);
                Toasts.show(qsTr("PHRASE \u2192 ") + root.beats + qsTr(" BEATS"));
            }
        }
    }
}
