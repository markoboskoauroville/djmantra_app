import "." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick
import QtQuick.Shapes 1.12
import QtQuick.Window
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
    property real heightGain: 0.85 // djay's zoomed lane: tall arrows (Marko's screenshots)
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
        if (root.beatSamples > 0 && samplesControl.value > 0) {
            const one = Math.round(playPositionControl.value * samplesControl.value
                    - beatNumberControl.value * root.beatSamples);
            // only a real change: every beat's few-sample jitter re-evaluated all the
            // beat lines' bindings (CPU, round 34)
            if (root.oneSample < 0 || Math.abs(one - root.oneSample) > root.beatSamples / 4)
                root.oneSample = one;
        }
    }

    onBeatSamplesChanged: updateOne()

    // V17 (render thread, round 34: 18-21 % CPU): every beat of the song was a
    // Rectangle in the scene (500-1000 per deck, with bar numbers), all moved each
    // frame. Only the beats near the screen are shown now; the window moves only
    // when the play position has gone half a screen, so the bindings rarely run.
    property real windowCenter: -1
    readonly property real windowHalf: waveform.width > 0
            ? waveformContainer.width / waveform.width * samplesControl.value : 0

    function updateWindow(force) {
        const s = playPositionControl.value * samplesControl.value;
        if (force || Math.abs(s - root.windowCenter) > root.windowHalf / 2)
            root.windowCenter = s;
    }

    onWindowHalfChanged: updateWindow(true)

    // Marko 13.10.2026 ("the zoomed view is stuttering"): the engine moves the play
    // position once per audio buffer (~21 ms); at the deep zoom (~2500 px a second)
    // that is a jump of ~50 px each time. The lane now moves every screen frame from
    // the deck's own speed, and each new engine position only corrects it gently
    // (a seek or a jump > 0.25 s snaps).
    property real smoothPosition: 0
    property real pendingCorrection: 0 // in track fractions, applied over a few frames

    function takeEnginePosition() {
        const target = playPositionControl.value;
        const duration = waveformContainer.duration;
        if (!(duration > 0) || !smoothMotion.running
                || Math.abs(target - root.smoothPosition) * duration > 0.25) {
            root.smoothPosition = target;
            root.pendingCorrection = 0;
        } else {
            // V38 (Marko: "as smooth as possible ... no shaking"): the engine's
            // positions arrive with a few ms of jitter; a gap under 4 ms of song is
            // that jitter, not drift, and is left alone (each correction is a
            // speed wobble you can see at the deep zoom)
            const gap = target - root.smoothPosition;
            root.pendingCorrection = Math.abs(gap) * duration < 0.004 ? 0 : gap;
        }
    }

    Mixxx.ControlProxy {
        id: playControl

        group: root.group
        key: "play"
    }

    FrameAnimation {
        id: smoothMotion

        running: root.visible && playControl.value > 0 && waveformContainer.duration > 0
        onTriggered: {
            const dt = Math.min(frameTime, 0.05);
            const speed = rateRatioControl.value / waveformContainer.duration;
            // a slow pull (8 % a frame) instead of 25 %: drift is fixed within a few
            // frames without the speed jumping when an engine update lands
            const step = root.pendingCorrection * 0.08;
            root.pendingCorrection -= step;
            root.smoothPosition = Math.max(0, Math.min(1, root.smoothPosition + speed * dt + step));
        }
        onRunningChanged: root.takeEnginePosition()
    }

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
            onValueChanged: {
                root.updateWindow(false);
                root.takeEnginePosition();
            }
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
            // V38: on whole device pixels: a fractional position made the shader sample the
            // waveform differently every frame (the wave shimmered) and the 1 px beat lines
            // blur between two pixels (flicker); whole pixels move the picture as one
            x: Math.round((playMarker.screenPosition * waveformContainer.width - root.smoothPosition * width)
                    * Screen.devicePixelRatio) / Screen.devicePixelRatio
            visible: root.deckPlayer.isLoaded

            Skin.DjayWaveformShader {
                id: djayShader

                group: root.group
                anchors.fill: parent
                levelGain: root.levelGain
                heightGain: root.heightGain
                // round 31: the lanes stayed empty and a release build says nothing
                // about a shader: its status goes to the log, and on an error the
                // classic waveform takes over
                onStatusChanged: Mixxx.DJMantra.log("waveform shader " + root.group + ": "
                        + (status === ShaderEffect.Compiled ? "compiled"
                           : status === ShaderEffect.Error ? "ERROR " + log : "status " + status))
                Component.onCompleted: Mixxx.DJMantra.log("waveform shader " + root.group
                        + " made, status " + status + ", level " + root.levelGain)
            }

            WaveformShader {
                // the classic one, only if djay's shader does not compile on this GPU
                group: root.group
                anchors.fill: parent
                visible: djayShader.status === ShaderEffect.Error
                visualGain: Math.max(1, root.levelGain) * 1.5
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

                    visible: Math.abs(framePosition * 2 - root.windowCenter) <= root.windowHalf * 1.6
                    width: Math.max(1, Math.round(Screen.devicePixelRatio)) / Screen.devicePixelRatio // whole device pixels
                    height: waveform.height
                    x: Math.round((framePosition * 2 / samplesControl.value) * waveform.width
                            * Screen.devicePixelRatio) / Screen.devicePixelRatio
                    color: bar ? "#c8922c" : "#444444"

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
        // Marko 14.10.2026: dragging the zoomed wave was the wrong way round (fast when
        // zoomed in, slow when zoomed out: pixels per second were used as samples per
        // pixel). Now the wave follows the finger (zoomed out = fast through the song,
        // zoomed in = slower), but never slower than 0.25 s per 100 px, so a deep zoom
        // still scrolls quickly instead of sample by sample. In samples (stereo) per pixel:
        readonly property real samplesPerPixel: {
            const zoomSeconds = Math.max(0.25, root.zoom > 0 ? root.zoom : 3);
            return 2 * sampleRateControl.value * rateRatioControl.value * zoomSeconds / 100;
        }

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: {
            mouseAnchor = Qt.point(mouse.x, mouse.y);
            if (mouse.button == Qt.LeftButton) {
                if (mouseStatus == WaveformRow.MouseStatus.Bending)
                    wheelControl.parameter = 0.5;

                mouseStatus = WaveformRow.MouseStatus.Scratching;
                scratchPositionEnableControl.value = 1;
                scratchPositionControl.value = -mouse.x * samplesPerPixel;
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
                    scratchPositionControl.value = -mouse.x * samplesPerPixel;
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
