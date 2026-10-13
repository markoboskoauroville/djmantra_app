#!/usr/bin/env python3
"""Load the new interface (res/qml/djmantra) in a headless Qt Quick engine
against a stub of the Mixxx QML module, so a QML error fails CI instead of a
phone round (round 16: Style.onAccent was read as a signal handler and the
whole interface stayed black).

Every component is compiled on its own, then Main.qml is created. Fails on a
component error and on the load warnings that break an interface ("Cannot
assign", "unavailable", "is not a type", "Failed to load"). Needs PySide6:
    pip install PySide6 && QT_QPA_PLATFORM=offscreen python3 tools/qml/check_djmantra_qml.py
"""
import os
import pathlib
import sys
import tempfile

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PySide6.QtCore import QUrl, qInstallMessageHandler  # noqa: E402
from PySide6.QtGui import QGuiApplication  # noqa: E402
from PySide6.QtQml import QQmlApplicationEngine, QQmlComponent  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[2]
UI = ROOT / "res" / "qml" / "djmantra"
FATAL = ("Cannot assign", "unavailable", "is not a type", "Failed to load",
         "is not installed", "Non-existent attached object", "Invalid property assignment",
         "Duplicate", "Expected token", "Syntax error")

STUB = {
    "qmldir": "module Mixxx\nControlProxy 1.0 ControlProxy.qml\n"
              "singleton DJMantra 1.0 DJMantra.qml\nsingleton PlayerManager 1.0 PlayerManager.qml\n"
              "WaveformOverview 1.0 WaveformOverview.qml\n",
    "WaveformOverview.qml": "import QtQuick 2.12\nItem {\n enum Channels { LeftChannel, RightChannel, BothChannels }\n"
                            " enum Renderer { RGB, Filtered, HSV, Djay }\n property var player\n property int channels\n"
                            " property int renderer\n property color colorHigh\n property color colorMid\n"
                            " property color colorLow\n property bool normalize\n}\n",
    "ControlProxy.qml": "import QtQuick 2.12\nQtObject {\n property string group\n property string key\n"
                        " property real value: 0\n property real parameter: 0\n property int maxRate: 0\n}\n",
    "PlayerManager.qml": "pragma Singleton\nimport QtQuick 2.12\nQtObject {\n"
                         " function getPlayer(group) { return null; }\n}\n",
    "DJMantra.qml": "pragma Singleton\nimport QtQuick 2.12\nQtObject {\n"
                    " property int insetLeft: 0\n property int insetTop: 0\n property int insetRight: 0\n"
                    " property int insetBottom: 0\n property string version: \"0\"\n property int cornerRadius: 40\n property int analysisLeft: 0\n signal analysisChanged()\n"
                    " function camelot(k) { return \"8A\"; }\n function musicalName(c) { return \"\"; }\n"
                    " function keyMatch(a, b) { return ({}); }\n function keyMatchShift(a, b) { return 0; }\n function formatTime(s) { return \"0:00\"; }\n"
                    " function barBeat(n, p) { return \"1.1\"; }\n function barsBeatsLeft(b, p) { return \"-0.0\"; }\n"
                    " function waveformGain(g, l) { return 0; }\n function isPlaying(g) { return false; }\n"
                    " function freeDeck() { return \"[Channel1]\"; }\n function noteLoaded(g) {}\n"
                    " function musicRoots() { return []; }\n function listFolder(f) { return [{name: \"a\", path: \"/m/a.mp3\", isFolder: false}]; }\n function analyzeAhead(p) {}\n function analyzeDecksAgain() { return 0; }\n function cqConnect(h) {}\n function cqDisconnect() {}\n function cqSetDelay(o) {}\n function cqTapNow() {}\n property string cqStatus: \"not connected\"\n property bool cqConnected: true\n property bool cqDelayOn: false\n function waveformLevel(g) { return 2; }\n function audioOutputs() { return [{key: \"\", label: \"Phone\"}, {key: \"usb:X\", label: \"USB: X\", channels: 4}]; }\n function audioOutputsVersion() { return 1; }\n function audioRoute() { return ({master: \"usb:X\", masterPair: 0, headphones: \"usb:X\", headphonesPair: 1}); }\n function setAudioRoute(m, mp, h, hp) { return \"\"; }\n function setting(n, f) { return f; }\n function setSetting(n, v) {}\n function songInfo(p) { return ({artist: \"Artist\", title: \"Title\", bpm: 124, key: \"8A\", cover: \"\"}); }\n"
                    " function hideBars() {}\n function showBars() {}\n function barsVisible() { return false; }\n function usage() { return ({cpu: 34, gpu: -1}); }\n function sections(g) { return []; }\n function setQuickInterface(on) {}\n"
                    " function quickInterface() { return true; }\n function log(t) {}\n}\n",
}

problems = []


def handler(mode, context, message):
    print(message)
    if any(f in message for f in FATAL):
        problems.append(message)


def main():
    qInstallMessageHandler(handler)
    app = QGuiApplication(sys.argv)
    with tempfile.TemporaryDirectory() as tmp:
        module = pathlib.Path(tmp) / "Mixxx"
        module.mkdir()
        for name, text in STUB.items():
            (module / name).write_text(text)
        engine = QQmlApplicationEngine()
        engine.addImportPath(tmp)
        for qml in sorted(UI.glob("*.qml")):
            component = QQmlComponent(engine, QUrl.fromLocalFile(str(qml)))
            for error in component.errors():
                problems.append(error.toString())
                print(error.toString())
        engine.load(QUrl.fromLocalFile(str(UI / "Main.qml")))
        if not engine.rootObjects():
            problems.append("Main.qml: no root object")
        app.processEvents()
    if problems:
        print("\nQML check FAILED:\n  " + "\n  ".join(problems))
        return 1
    print("QML check: %d components and Main.qml load" % len(list(UI.glob("*.qml"))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
