import QtQuick
import QtQuick.Controls.FluentWinUI3

// Fluent-styled icon button. Keeps the framework's hover/press background while
// showing our glyph icon (and optional text).
Button {
    id: btn
    property string iconName
    property int iconSize: 18
    property bool showText: false

    implicitWidth: showText
        ? Math.max(72, contentRow.implicitWidth + 20)
        : 40
    implicitHeight: 40

    icon.source: ""   // disable default icon; we draw our own

    contentItem: Row {
        id: contentRow
        spacing: 6
        padding: 0
        leftPadding: 0
        rightPadding: 0
        height: btn.height
        anchors.centerIn: parent
        Icon {
            name: btn.iconName
            size: btn.iconSize
            color: btn.enabled ? (btn.hovered ? "#0f7bcd" : (ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F"))
                                : "#888888"
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: btn.showText && btn.text
            text: btn.text
            font: btn.font
            color: btn.enabled ? (ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F") : "#888888"
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
