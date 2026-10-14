pragma Singleton
import QtQuick 2.12

// Marko 14.10.2026: "I need a toast ... the finger covers everything and I cannot see what's
// going on below my finger": every change made by a long press or a double tap (a knob
// reset, GAIN -> FILTER, VOLUME -> PITCH, an effect chosen ...) says what it did. Main.qml
// shows it above the controls.
QtObject {
    signal shown(string text)

    function show(text) {
        shown(text);
    }
}
