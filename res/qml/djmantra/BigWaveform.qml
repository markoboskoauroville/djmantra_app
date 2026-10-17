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
    property real zoom: 3
    readonly property var player: Mixxx.PlayerManager.getPlayer(group)
    // the song's level fills the lane; asked again until the analysis is done
    property real gain: 1
    property bool measured: false
    property bool mirrored: Mixxx.DJMantra.setting("lane_style", "mirrored") !== "half"
    // Marko 13.10.2026 ("the looks of the waveform the same as the reference ones"):
    // every djay reference (testing/djay/screenshots, the round 35/39 pairs) draws the
    // lane thin, ~20-25 % of its height, one arrow per hit: that is the default now;
    // Settings -> LANE HEIGHT "tall" keeps the big wave
    property bool thin: Mixxx.DJMantra.setting("lane_height", "djay") !== "tall"

    clip: true

    Timer {
        interval: 1500
        repeat: true
        running: root.player !== null && root.player.isLoaded && !root.measured
        triggeredOnStart: true
        onTriggered: {
            const g = Mixxx.DJMantra.waveformLevel(root.group);
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
        // V9: djay Pro's look (a thin wave, WaveformRow.heightGain)
        levelGain: root.gain
        // Marko's djay screenshots (zoomed lane): the wave fills ~90 % of the lane
        // round 35 (djay vs the app at the same bars): djay draws each deck's wave
        // centred in its own lane, mirrored up and down; Settings -> LANE "half"
        // keeps round 18's big half wave (deck 1 up, deck 2 down from the middle)
        // r42 V77 (the icon's look; the tester: "the bars use only a quarter to a third of the
        // lane"): the loud parts reach about two thirds of the lane by default
        heightGain: root.thin ? (root.mirrored ? 0.62 : 0.66) : (root.mirrored ? 0.8 : 0.9)
        zoom: root.zoom
        showRoll: false
        width: root.width
        height: root.mirrored ? root.height : root.height * 2
        y: root.mirrored || root.upper ? 0 : -root.height
    }

    Connections {
        // V87: LANE and LANE HEIGHT follow Settings at once
        target: Mixxx.DJMantra
        function onSettingChanged(name, value) {
            if (name === "lane_style")
                root.mirrored = value !== "half";
            else if (name === "lane_height")
                root.thin = value !== "tall";
        }
    }
}
