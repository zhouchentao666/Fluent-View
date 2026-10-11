import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Controls.FluentWinUI3

Item {
    id: titleBar
    height: 40
    property color panelColor: ThemeManager.isDark() ? "#2b2b2b" : "#ebebeb"

    Rectangle {
        anchors.fill: parent
        color: titleBar.panelColor
    }

    // Drag region for moving the frameless window.
    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: 132   // leave space for window controls
        onDoubleClicked: Window.window.visibility === Window.Maximized
                         ? Window.window.showNormal()
                         : Window.window.showMaximized()
        onPressed: (mouse) => {
            titleBar.forceActiveFocus()
            Window.window.startSystemMove()
        }
    }

    RowLayout {
        id: titleRow
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8
        Icon { name: "menu"; size: 16; color: ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F" }
        Label {
            text: "Fluent View"
            font.pixelSize: 13
            font.weight: Font.Medium
            color: ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F"
        }
    }

    // Window controls (minimize / maximize / close)
    Row {
        id: winControls
        height: parent.height
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 0

        IconButton {
            width: 44; height: 40
            iconName: "min"
            onClicked: Window.window.showMinimized()
            background: Rectangle { color: parent.hovered ? (ThemeManager.isDark() ? "#3a3a3a" : "#dcdcdc") : "transparent" }
        }
        IconButton {
            width: 44; height: 40
            iconName: Window.window.visibility === Window.Maximized ? "restore" : "max"
            onClicked: Window.window.visibility === Window.Maximized
                      ? Window.window.showNormal()
                      : Window.window.showMaximized()
            background: Rectangle { color: parent.hovered ? (ThemeManager.isDark() ? "#3a3a3a" : "#dcdcdc") : "transparent" }
        }
        IconButton {
            width: 44; height: 40
            iconName: "close"
            onClicked: Window.window.close()
            background: Rectangle {
                color: parent.hovered ? "#c42b1c" : "transparent"
            }
            contentItem: Icon {
                name: "close"; size: 16
                color: parent.hovered ? "#ffffff" : (ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F")
                anchors.centerIn: parent
            }
        }
    }
}
