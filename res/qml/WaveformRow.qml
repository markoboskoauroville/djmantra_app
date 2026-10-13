import "." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.14
import QtQuick.Shapes 1.12
import "Theme"

Item {
    id: root

    enum MouseStatus {
        Normal,
        Bending,
        Scratching
    }

    property string group // required
    property var deckPlayer: Mixxx.PlayerManager.getPlayer(group)
    // DJ Mantra: display gain of the waveform (WaveformShader.visualGain)
    property real visualGain: 1.0
    // DJ Mantra: its own zoom (seconds of track per 100 px, as waveform_zoom;
    // 0 = the deck's waveform_zoom control, which the Qt Quick interface does
    // not have: round 18's zoom did nothing) and no pre / post roll marks
    // (round 18: diagonal lines in landscape)
    property real zoom: 0
    property bool showRoll: true
    // V9 djay Pro's look (DjayWaveformShader): the bands / the song's loudest level
    // (Mixxx.DJMantra.waveformLevel) and how far the loud parts reach
    property real levelGain: 1.0
    property real heightGain: 0.9
    // V9: bar lines orange with the bar number (djay), beat lines grey. The bar of
    // each beat comes from the deck's beat_number (beats since "1") at the last
    // beat: oneSample only changes when the count does, not every frame
    property real oneSample: -1
    readonly property real beatSamples: bpmControl.value > 0
            ? 60 / bpmControl.value * sampleRateControl.value * 2 : 0

    Mixxx.ControlProxy {
        id: beatNumberControl

        group: root.group
        key: "beat_number"
        onValueChanged: root.updateOne()
        Component.onCompleted: root.updateOne()
    }

    function updateOne() {
        if (root.beatSamples > 0 && samplesControl.value > 0)
            root.oneSample = Math.round(playPositionControl.value * samplesControl.value
                    - beatNumberControl.value * root.beatSamples);
    }

    onBeatSamplesChanged: updateOne()

    Mixxx.ControlProxy {
        id: beatsPerBarControl

        group: root.group
        key: "beats_per_bar"
    }

    Mixxx.ControlProxy {
        id: bpmControl

        group: root.group
        key: "file_bpm"
    }

    Item {
        id: waveformContainer

        property real duration: samplesControl.value / sampleRateControl.value

        anchors.fill: parent
        clip: true

        Rectangle {
            // djay Pro's lane background
            anchors.fill: parent
            color: "#141414"
        }

        Mixxx.ControlProxy {
            id: samplesControl

            group: root.group
            key: "track_samples"
        }

        Mixxx.ControlProxy {
            id: sampleRateControl

            group: root.group
            key: "track_samplerate"
        }

        Mixxx.ControlProxy {
            id: playPositionControl

            group: root.group
            key: "playposition"
            // DJ Mantra round 25: the engine moves it with every audio buffer;
            // the screen draws 60 frames a second, more is wasted work
            maxRate: 60
        }

        Mixxx.ControlProxy {
            id: rateRatioControl

            group: root.group
            key: "rate_ratio"
        }

        Mixxx.ControlProxy {
            id: zoomControl

            group: root.group
            key: "waveform_zoom"
        }

        Mixxx.ControlProxy {
            id: introStartPosition

            group: root.group
            key: "intro_start_position"
        }

        Mixxx.ControlProxy {
            id: introEndPosition

            group: root.group
            key: "intro_end_position"
        }

        Mixxx.ControlProxy {
            id: outroStartPosition

            group: root.group
            key: "outro_start_position"
        }

        Mixxx.ControlProxy {
            id: outroEndPosition

            group: root.group
            key: "outro_end_position"
        }

        Mixxx.ControlProxy {
            id: loopStartPosition

            group: root.group
            key: "loop_start_position"
        }

        Mixxx.ControlProxy {
            id: loopEndPosition

            group: root.group
            key: "loop_end_position"
        }

        Mixxx.ControlProxy {
            id: loopEnabled

            group: root.group
            key: "loop_enabled"
        }

        Mixxx.ControlProxy {
            id: mainCuePosition

            group: root.group
            key: "cue_point"
        }

        Item {
            id: waveform

            property real effectiveZoomFactor: (1 / rateRatioControl.value) * (100 / (root.zoom > 0 ? root.zoom : zoomControl.value))

            width: waveformContainer.duration * effectiveZoomFactor
            height: parent.height
            x: playMarker.screenPosition * waveformContainer.width - playPositionControl.value * width
            visible: root.deckPlayer.isLoaded

            Skin.DjayWaveformShader {
                group: root.group
                anchors.fill: parent
                levelGain: root.levelGain
                heightGain: root.heightGain
            }

            Shape {
                id: preroll

                visible: root.showRoll

                property real triangleHeight: waveform.height
                property real triangleWidth: 0.25 * waveform.effectiveZoomFactor
                property int numTriangles: Math.ceil(width / triangleWidth)

                anchors.top: waveform.top
                anchors.right: waveform.left
                width: Math.max(0, waveform.x)
                height: waveform.height

                ShapePath {
                    strokeColor: Theme.waveformPrerollColor
                    strokeWidth: 1
                    fillColor: "transparent"

                    PathMultiline {
                        paths: {
                            let p = [];
                            for (let i = 0; i < preroll.numTriangles; i++) {
                                p.push([
                                        Qt.point(preroll.width - i * preroll.triangleWidth, preroll.triangleHeight / 2),
                                        Qt.point(preroll.width - (i + 1) * preroll.triangleWidth, 0),
                                        Qt.point(preroll.width - (i + 1) * preroll.triangleWidth, preroll.triangleHeight),
                                        Qt.point(preroll.width - i * preroll.triangleWidth, preroll.triangleHeight / 2),
                                ]);
                            }
                            return p;
                        }
                    }
                }
            }

            Shape {
                id: postroll

                visible: root.showRoll

                property real triangleHeight: waveform.height
                property real triangleWidth: 0.25 * waveform.effectiveZoomFactor
                property int numTriangles: Math.ceil(width / triangleWidth)

                anchors.top: waveform.top
                anchors.left: waveform.right
                width: waveformContainer.width / 2
                height: waveform.height

                ShapePath {
                    strokeColor: Theme.waveformPostrollColor
                    strokeWidth: 1
                    fillColor: "transparent"

                    PathMultiline {
                        paths: {
                            let p = [];
                            for (let i = 0; i < postroll.numTriangles; i++) {
                                p.push([
                                        Qt.point(i * postroll.triangleWidth, postroll.triangleHeight / 2),
                                        Qt.point((i + 1) * postroll.triangleWidth, 0),
                                        Qt.point((i + 1) * postroll.triangleWidth, postroll.triangleHeight),
                                        Qt.point(i * postroll.triangleWidth, postroll.triangleHeight / 2),
                                ]);
                            }
                            return p;
                        }
                    }
                }
            }

            Repeater {
                model: root.deckPlayer.beatsModel

                Rectangle {
                    id: beatLine

                    required property real framePosition
                    readonly property int perBar: beatsPerBarControl.value > 0 ? beatsPerBarControl.value : 4
                    readonly property int beat: root.beatSamples > 0 && root.oneSample >= 0
                            ? Math.round((framePosition * 2 - root.oneSample) / root.beatSamples) : 1
                    readonly property bool bar: ((beat % perBar) + perBar) % perBar === 0

                    width: bar ? 2 : 1
                    height: waveform.height
                    x: (framePosition * 2 / samplesControl.value) * waveform.width
                    color: bar ? "#d99a2b" : "#3a3a3a"

                    Text {
                        // djay: ... -2, -1, 1, 2 ... (no bar 0)
                        visible: beatLine.bar && root.oneSample >= 0
                        x: 3
                        y: 1
                        text: {
                            const n = Math.floor(beatLine.beat / beatLine.perBar);
                            return String(n >= 0 ? n + 1 : n);
                        }
                        color: "#bdbdbd"
                        font.pixelSize: 10
                    }
                }
            }

            Skin.WaveformIntroOutro {
                id: intro

                visible: introStartPosition.value != -1 || introEndPosition.value != -1

                height: waveform.height
                x: ((introStartPosition.value != -1 ? introStartPosition.value : introEndPosition.value) / samplesControl.value) * waveform.width
                width: introEndPosition.value == -1 ? 0 : ((introEndPosition.value - introStartPosition.value) / samplesControl.value) * waveform.width
            }

            Skin.WaveformIntroOutro {
                id: outro

                visible: outroStartPosition.value != -1 || outroEndPosition.value != -1
                isIntro: false

                height: waveform.height
                x: ((outroStartPosition.value != -1 ? outroStartPosition.value : outroEndPosition.value) / samplesControl.value) * waveform.width
                width: outroEndPosition.value == -1 || outroStartPosition.value == -1 ? 0 : ((outroEndPosition.value - outroStartPosition.value) / samplesControl.value) * waveform.width
            }

            Skin.WaveformLoop {
                id: loop

                visible: loopStartPosition.value != -1 && loopEndPosition.value != -1

                height: waveform.height
                x: (loopStartPosition.value / samplesControl.value) * waveform.width
                width: ((loopEndPosition.value - loopStartPosition.value) / samplesControl.value) * waveform.width
                enabled: loopEnabled.value
            }

            Repeater {
                model: root.deckPlayer.hotcuesModel

                Item {
                    id: cue

                    required property int startPosition
                    required property int endPosition
                    required property string label
                    required property bool isLoop
                    required property int hotcueNumber

                    Skin.WaveformHotcue {
                        group: root.group
                        hotcueNumber: cue.hotcueNumber + 1
                        label: cue.label
                        isLoop: cue.isLoop

                        x: (startPosition * 2 / samplesControl.value) * waveform.width
                        width: cue.isLoop ? ((endPosition - startPosition) * 2 / samplesControl.value) * waveform.width : null
                        height: waveform.height
                    }
                }
            }

            Skin.WaveformCue {
                id: maincue

                height: waveform.height
                x: (mainCuePosition.value / samplesControl.value) * waveform.width
            }
        }
    }

    Shape {
        id: playMarkerShape

        anchors.fill: parent

        ShapePath {
            id: playMarker

            property real screenPosition: 0.5

            startX: playMarkerShape.width * playMarker.screenPosition
            startY: 0
            strokeColor: Theme.waveformCursorColor
            strokeWidth: 1

            PathLine {
                id: marker

                x: playMarkerShape.width * playMarker.screenPosition
                y: playMarkerShape.height
            }
        }
    }

    Mixxx.ControlProxy {
        id: scratchPositionEnableControl

        group: root.group
        key: "scratch_position_enable"
    }

    Mixxx.ControlProxy {
        id: scratchPositionControl

        group: root.group
        key: "scratch_position"
    }

    Mixxx.ControlProxy {
        id: wheelControl

        group: root.group
        key: "wheel"
    }

    MouseArea {
        property int mouseStatus: WaveformRow.MouseStatus.Normal
        property point mouseAnchor: Qt.point(0, 0)

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: {
            mouseAnchor = Qt.point(mouse.x, mouse.y);
            if (mouse.button == Qt.LeftButton) {
                if (mouseStatus == WaveformRow.MouseStatus.Bending)
                    wheelControl.parameter = 0.5;

                mouseStatus = WaveformRow.MouseStatus.Scratching;
                scratchPositionEnableControl.value = 1;
                // TODO: Calculate position properly
                scratchPositionControl.value = -mouse.x * waveform.effectiveZoomFactor * 2;
                console.log(mouse.x);
            } else {
                if (mouseStatus == WaveformRow.MouseStatus.Scratching)
                    scratchPositionEnableControl.value = 0;

                wheelControl.parameter = 0.5;
                mouseStatus = WaveformRow.MouseStatus.Bending;
            }
        }
        onPositionChanged: {
            switch (mouseStatus) {
                case WaveformRow.MouseStatus.Bending: {
                    const diff = mouse.x - mouseAnchor.x;
                    // Start at the middle of [0.0, 1.0], and emit values based on how far
                    // the mouse has traveled horizontally. Note, for legacy (MIDI) reasons,
                    // this is tuned to 127.
                    const v = 0.5 + (diff / 1270);
                    // clamp to [0.0, 1.0]
                    wheelControl.parameter = Mixxx.MathUtils.clamp(v, 0, 1);
                    break;
                };
                case WaveformRow.MouseStatus.Scratching:
                // TODO: Calculate position properly
                    scratchPositionControl.value = -mouse.x * waveform.effectiveZoomFactor * 2;
                    break;
            }
        }
        onReleased: {
            switch (mouseStatus) {
                case WaveformRow.MouseStatus.Bending:
                    wheelControl.parameter = 0.5;
                    break;
                case WaveformRow.MouseStatus.Scratching:
                    scratchPositionEnableControl.value = 0;
                    break;
            }
            mouseStatus = WaveformRow.MouseStatus.Normal;
        }
    }
}
