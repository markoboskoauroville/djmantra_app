import "."
import ".." as Skin
import QtQuick 2.12

// Mixxx's scrolling waveform (drawn by the GPU: a shader over the whole
// track's waveform texture), turned upright for the phone: what comes next is
// above the play line in the middle and moves down to it
Item {
    id: root

    property string group

    clip: true

    Skin.WaveformRow {
        group: root.group
        width: root.height
        height: root.width
        anchors.centerIn: parent
        rotation: -90
    }
}
