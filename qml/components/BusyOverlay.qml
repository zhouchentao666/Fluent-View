import QtQuick
import QtQuick.Controls.FluentWinUI3

// Shown while the main image is decoding (async provider).
Item {
    id: overlay
    property bool active: false
    anchors.fill: parent
    visible: active
    opacity: active ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 120 } }

    Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.15 }

    BusyIndicator {
        anchors.centerIn: parent
        running: overlay.active
        width: 48; height: 48
    }
}
