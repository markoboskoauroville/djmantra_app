pragma Singleton
import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Recording the mix (Marko 14.10.2026: djay records, ours must too, so the two can be
// compared): Menu -> RECORD MIX starts / stops it, lossless FLAC (or WAV, Settings) into
// Music/DJ Mantra Recordings as "<name> - <date> <time>" (name in Settings, default
// "DJ Mantra Mix"); also a long press on the clock, which is red while it records.
QtObject {
    id: root

    property Mixxx.ControlProxy status: Mixxx.ControlProxy {
        group: "[Recording]"
        key: "status"
    }
    readonly property bool recording: status.value > 0
    property string file: ""
    property double startedAt: 0

    function toggle() {
        if (recording) {
            Mixxx.DJMantra.stopRecording();
            Toasts.show(qsTr("RECORDING SAVED: ") + file.substring(file.lastIndexOf("/") + 1));
            return;
        }
        const path = Mixxx.DJMantra.startRecording(Mixxx.DJMantra.setting("rec_format", "FLAC"),
                Mixxx.DJMantra.setting("rec_name", "DJ Mantra Mix"));
        file = path;
        startedAt = Date.now();
        Toasts.show(qsTr("● RECORDING ") + path.substring(path.lastIndexOf("/") + 1));
    }
}
