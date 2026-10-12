import "."
import ".." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Half of a deck's scrolling waveform, big (round 18, Marko: "it can be also
// only half of the waveform, that's okay, but it needs to be much bigger ...
// so I can really see the frequencies and the different colors"). Mixxx's
// GPU waveform (a shader), clipped to its upper half (`upper`) or lower half,
// so deck 1 grows up and deck 2 down from the line between them.
Item {
    id: root

    property string group
    property bool upper: true
    readonly property var player: Mixxx.PlayerManager.getPlayer(group)
    // the song's level fills the lane; asked again until the analysis is done
    property real gain: 1
    property bool measured: false

    clip: true

    Timer {
        interval: 1500
        repeat: true
        running: root.player !== null && root.player.isLoaded && !root.measured
        triggeredOnStart: true
        onTriggered: {
            const g = Mixxx.DJMantra.waveformGain(root.group, 0);
            if (g > 0) {
                root.gain = g;
                root.measured = true;
            }
        }
    }

    Connections {
        target: root.player
        function onTrackChanged() {
            root.gain = 1;
            root.measured = false;
        }
    }

    Skin.WaveformRow {
        group: root.group
        visualGain: root.gain
        width: root.width
        height: root.height * 2
        y: root.upper ? 0 : -root.height
    }
}
