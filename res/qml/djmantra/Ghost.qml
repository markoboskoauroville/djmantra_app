import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The GHOST of a controller knob that has not picked up its value yet (V77, Marko 15.10.2026:
// "like in Ableton Live like a ghost fader until it picks up the real fader. So half transparent
// one shows the current position of the knob, and then when it picks up, they are all one. So
// there is no jumping around the scene. And that principle applies to any pickup fader or knob
// in the user interface"). The Mix Ultra script writes [DJMantra] ghost_<deck>_<code> = the
// knob's position + 1 while it is not picked up, 0 once it is. Shown while the knob moves and
// for a few seconds after (a knob left elsewhere does not keep a ghost for ever).
QtObject {
    id: ghost

    // the [DJMantra] control, "" = no ghost
    property string name: ""
    // the knob's position 0..1, or -1: no ghost to show
    readonly property real position: shown ? proxy.value - 1 : -1
    property bool shown: false

    property Mixxx.ControlProxy proxy: Mixxx.ControlProxy {
        group: "[DJMantra]"
        key: ghost.name !== "" ? ghost.name : "fx_edit"
        onValueChanged: {
            if (ghost.name === "")
                return;
            ghost.shown = value > 0;
            if (ghost.shown)
                ghost.timer.restart();
        }
    }
    property Timer timer: Timer {
        interval: 3000
        onTriggered: ghost.shown = false
    }
}
