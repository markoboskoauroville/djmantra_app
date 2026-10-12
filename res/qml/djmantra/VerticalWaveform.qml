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

    clip: true

    Skin.WaveformRow {
        group: root.group
        // the song's loudest point fills 90 % of the lane (round 13: 5 %)
        visualGain: root.player ? Mixxx.DJMantra.waveformGain(root.group, root.player.waveformLength) : 1
        width: root.height
        height: root.width
        anchors.centerIn: parent
        rotation: -90
    }
}
