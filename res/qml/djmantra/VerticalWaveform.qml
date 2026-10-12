import "."
import ".." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Mixxx's scrolling waveform (drawn by the GPU: a shader over the whole
// track's waveform texture), turned upright for the phone: what comes next is
// above the play line in the middle and moves down to it
Item {
    id: root

    property string group
    readonly property var player: Mixxx.PlayerManager.getPlayer(group)
    // the song's level fills the lane; asked again until the analysis is done
    property real gain: 1
    property bool measured: false

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

    clip: true

    Skin.WaveformRow {
        group: root.group
        // the song's loudest point fills 90 % of the lane (round 13: 5 %)
        visualGain: root.gain
        width: root.height
        height: root.width
        anchors.centerIn: parent
        rotation: -90
    }
}
