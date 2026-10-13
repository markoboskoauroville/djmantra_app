import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's STOP as a turntable (Marko 13.10.2026: "a stop time in the settings: 0 =
// stops at once, 1 s = the turntable effect, the record slowing down"). The speed
// (scratch2, the pitch falls with it as on a record) goes from 1 to 0 over the stop
// time, then the deck stops and plays normally again at the next PLAY. PLAY during
// the slow-down cancels it.
Item {
    id: root

    property string group
    readonly property real stopTime: Number(Mixxx.DJMantra.setting("stop_time", "0"))
    readonly property bool braking: ramp.running
    property real startedAt: 0
    property real seconds: 0
    property real fromSpeed: 1 // the deck's speed when the stop began (a synced deck)

    function stop() {
        const t = Number(Mixxx.DJMantra.setting("stop_time", "0"));
        if (playControl.value <= 0 || ramp.running)
            return;
        if (t <= 0) {
            playControl.value = 0;
            return;
        }
        root.seconds = t;
        root.startedAt = Date.now();
        root.fromSpeed = rateRatio.value > 0 ? rateRatio.value : 1;
        scratchControl.value = root.fromSpeed;
        scratchEnable.value = 1;
        ramp.start();
        Mixxx.DJMantra.log("turntable stop " + root.group + " " + t + " s");
    }

    function cancel() {
        if (!ramp.running)
            return;
        ramp.stop();
        scratchEnable.value = 0;
        Mixxx.DJMantra.log("turntable stop " + root.group + " cancelled");
    }

    Mixxx.ControlProxy { id: playControl; group: root.group; key: "play" }
    Mixxx.ControlProxy { id: scratchControl; group: root.group; key: "scratch2" }
    Mixxx.ControlProxy { id: scratchEnable; group: root.group; key: "scratch2_enable" }
    Mixxx.ControlProxy { id: rateRatio; group: root.group; key: "rate_ratio" }

    Timer {
        id: ramp

        interval: 16
        repeat: true
        onTriggered: {
            const left = 1 - (Date.now() - root.startedAt) / (1000 * root.seconds);
            if (left <= 0) {
                ramp.stop();
                playControl.value = 0;
                scratchEnable.value = 0;
                scratchControl.value = 0;
                return;
            }
            // a turntable's platter slows evenly (its speed falls in a straight line)
            scratchControl.value = root.fromSpeed * left;
        }
    }
}
