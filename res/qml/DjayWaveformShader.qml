// DJ Mantra: djay Pro's scrolling waveform (res/shaders/djaysignal_qml.frag)
import Mixxx 1.0 as Mixxx
import QtQuick 2.12
import QtQuick.Window 2.12

ShaderEffect {
    id: root

    property string group // required
    property var deckPlayer: Mixxx.PlayerManager.getPlayer(group)
    property size framebufferSize: Qt.size(width, height)
    property int waveformLength: root.deckPlayer.waveformLength
    property int textureSize: root.deckPlayer.waveformTextureSize
    property int textureStride: root.deckPlayer.waveformTextureStride
    property real firstVisualIndex: 1
    property real lastVisualIndex: root.deckPlayer.waveformLength / 2
    property color axesColor: "#40FFFFFF"
    property real highGain: filterWaveformEnableControl.value ? (filterHighKillControl.value ? 0 : filterHighControl.value) : 1
    property real midGain: filterWaveformEnableControl.value ? (filterMidKillControl.value ? 0 : filterMidControl.value) : 1
    property real lowGain: filterWaveformEnableControl.value ? (filterLowKillControl.value ? 0 : filterLowControl.value) : 1
    // the bands / the song's loudest level (the colour's inputs, as fitted)
    property real levelGain: 1.0
    // how far the loud parts reach (1 = the lane's edge)
    property real heightGain: 1.0
    // sharp bar edges to the device pixel (V77)
    property real pixelRatio: Screen.devicePixelRatio
    // V78: 0 = 3-BAND (sections), 1 = ICON (one colour per bar); V80: 2 = DJAY (the bands'
    // colours mixed, as djay draws them; the default); Settings -> WAVEFORM
    readonly property string waveStyleName: Mixxx.DJMantra.setting("wave_style", "djay")
    property int waveStyle: waveStyleName === "icon" ? 1 : (waveStyleName === "3band" ? 0 : 2)
    property Image waveformTexture
    // Marko 13.10.2026: the real sound wave at deep zoom (qml/finewave.h); 0 = not built
    property int fineLength: root.deckPlayer.fineWaveLength
    property int fineStride: 4096
    property Image fineTexture: Image {
        visible: false
        layer.enabled: false
        source: root.deckPlayer.fineWaveTexture
    }

    fragmentShader: "qrc:/shaders/djaysignal_qml.frag.qsb"

    Mixxx.ControlProxy {
        id: pregainControl

        group: root.group
        key: "pregain"
    }

    Mixxx.ControlProxy {
        id: filterWaveformEnableControl

        group: root.group
        key: "filterWaveformEnable"
    }

    Mixxx.ControlProxy {
        id: filterHighControl

        group: "[EqualizerRack1_" + root.group + "_Effect1]"
        key: "parameter3"
    }

    Mixxx.ControlProxy {
        id: filterHighKillControl

        group: "[EqualizerRack1_" + root.group + "_Effect1]"
        key: "button_parameter3"
    }

    Mixxx.ControlProxy {
        id: filterMidControl

        group: "[EqualizerRack1_" + root.group + "_Effect1]"
        key: "parameter2"
    }

    Mixxx.ControlProxy {
        id: filterMidKillControl

        group: "[EqualizerRack1_" + root.group + "_Effect1]"
        key: "button_parameter2"
    }

    Mixxx.ControlProxy {
        id: filterLowControl

        group: "[EqualizerRack1_" + root.group + "_Effect1]"
        key: "parameter1"
    }

    Mixxx.ControlProxy {
        id: filterLowKillControl

        group: "[EqualizerRack1_" + root.group + "_Effect1]"
        key: "button_parameter1"
    }

    waveformTexture: Image {
        visible: false
        layer.enabled: false
        // each point exactly as stored (V77: smoothing blended neighbouring points)
        smooth: false
        mipmap: false
        source: root.deckPlayer.waveformTexture
    }
}
