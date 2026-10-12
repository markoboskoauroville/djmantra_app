pragma Singleton
import QtQuick 2.12

// DJ Mantra in the style of Marko's horoscope app (astro_dos, modern/Vg.kt,
// Theme.NIGHT): deep navy, navy cards, one yellow accent, element colours
QtObject {
    readonly property color background: "#0B1026"
    readonly property color surface: "#151C3D"   // cards, bars
    readonly property color tile: "#1F2850"      // buttons, raised cards
    readonly property color pressed: "#2C3870"
    readonly property color text: "#EEF0FF"      // ink
    readonly property color subText: "#9AA3C7"
    readonly property color line: "#6B76A8"
    readonly property color accent: "#FFD54A"    // the one accent: selected, primary
    readonly property color onAccent: "#1A1A1A"
    // the element colours: fire, earth, air, water
    readonly property color fire: "#FF6B5E"
    readonly property color earth: "#6CCB72"
    readonly property color air: "#F2D35B"
    readonly property color water: "#5AA2F0"
    readonly property color deck1: earth
    readonly property color deck2: water
    readonly property color orange: fire        // CUE
    readonly property color red: fire
    readonly property int gap: 6
    readonly property int radius: 10
}
