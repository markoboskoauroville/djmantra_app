import "."
import QtQuick 2.12

// The ✕ that closes whatever lies over the main screen (Marko 13.10.2026: "add X
// button to close any dialogue opening over the main user interface ... I'm stuck")
DButton {
    width: 48
    height: 40
    text: "✕"
    fontSize: 20
    mode: ""
}
