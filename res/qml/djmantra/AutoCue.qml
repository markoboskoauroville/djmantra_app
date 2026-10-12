import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Automatic pre-listen (Marko, 12.10.2026: when the deck in the headphones
// goes on air, the headphones switch by themselves to the other deck; one
// headphone deck at a time). As djay Pro's "Auto select" (Settings, Advanced:
// "automatically selects the deck that isn't playing as your pre-cue
// source"): when exactly one deck is on air (playing, its volume up, the
// crossfader not cutting it), the headphones get the other deck. Both on air
// (a blend): nothing changes. On by default; Settings switch.
Item {
    id: root

    property bool active: Mixxx.DJMantra.setting("auto_cue", true)
    onActiveChanged: {
        Mixxx.DJMantra.setSetting("auto_cue", active);
        Mixxx.DJMantra.log("auto cue " + (active ? "on" : "off"));
        if (active)
            settle.restart();
    }

    Mixxx.ControlProxy { id: play1; group: "[Channel1]"; key: "play"; onValueChanged: settle.restart() }
    Mixxx.ControlProxy { id: play2; group: "[Channel2]"; key: "play"; onValueChanged: settle.restart() }
    Mixxx.ControlProxy { id: volume1; group: "[Channel1]"; key: "volume"; onValueChanged: settle.restart() }
    Mixxx.ControlProxy { id: volume2; group: "[Channel2]"; key: "volume"; onValueChanged: settle.restart() }
    Mixxx.ControlProxy { id: loaded1; group: "[Channel1]"; key: "track_loaded" }
    Mixxx.ControlProxy { id: loaded2; group: "[Channel2]"; key: "track_loaded" }
    Mixxx.ControlProxy { id: pfl1; group: "[Channel1]"; key: "pfl" }
    Mixxx.ControlProxy { id: pfl2; group: "[Channel2]"; key: "pfl" }
    Mixxx.ControlProxy { id: fader; group: "[Master]"; key: "crossfader"; onValueChanged: settle.restart() }

    function onAir(deck) {
        const playing = (deck === 1 ? play1 : play2).value > 0;
        const volume = (deck === 1 ? volume1 : volume2).value;
        const heard = deck === 1 ? fader.value < 0.95 : fader.value > -0.95;
        return playing && volume > 0.05 && heard;
    }

    function evaluate() {
        if (!active)
            return;
        const air1 = onAir(1), air2 = onAir(2);
        let cue = 0;
        if (air1 && !air2 && loaded2.value > 0)
            cue = 2;
        else if (air2 && !air1 && loaded1.value > 0)
            cue = 1;
        if (cue === 0)
            return;
        const want1 = cue === 1 ? 1 : 0;
        const want2 = cue === 2 ? 1 : 0;
        if (pfl1.value !== want1 || pfl2.value !== want2) {
            pfl1.value = want1;
            pfl2.value = want2;
            Mixxx.DJMantra.log("auto cue: headphones to [Channel" + cue + "]");
        }
    }

    Timer {
        // a moment after the change (a fader in motion: once it rests)
        id: settle

        interval: 250
        onTriggered: root.evaluate()
    }

}
