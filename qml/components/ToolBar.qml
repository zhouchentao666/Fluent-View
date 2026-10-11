import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3

Item {
    id: toolbar
    height: 52
    property color panelColor: ThemeManager.isDark() ? "#2b2b2b" : "#ebebeb"

    signal requestOpen()
    signal requestFit()
    signal requestActual()
    signal requestFullscreen()
    signal requestInfo()
    signal requestSettings()
    signal requestThemeToggle()

    Rectangle { anchors.fill: parent; color: toolbar.panelColor }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 4

        IconButton { iconName: "open"; width: 44; text: qsTr("Open"); showText: false; onClicked: toolbar.requestOpen() }

        Rectangle { width: 1; height: 24; color: ThemeManager.isDark() ? "#404040" : "#c8c8c8" }

        IconButton { iconName: "prev"; width: 44; enabled: ImageModel.hasPrevious; onClicked: ImageModel.previous() }
        IconButton { iconName: "next2"; width: 44; enabled: ImageModel.hasNext; onClicked: ImageModel.next() }

        Rectangle { width: 1; height: 24; color: ThemeManager.isDark() ? "#404040" : "#c8c8c8" }

        IconButton { iconName: "rotateL"; width: 44; onClicked: ImageCtl.rotateLeft() }
        IconButton { iconName: "rotateR"; width: 44; onClicked: ImageCtl.rotateRight() }
        IconButton { iconName: "flipH"; width: 44; onClicked: ImageCtl.flipHorizontal() }
        IconButton { iconName: "flipV"; width: 44; onClicked: ImageCtl.flipVertical() }

        Rectangle { width: 1; height: 24; color: ThemeManager.isDark() ? "#404040" : "#c8c8c8" }

        IconButton { iconName: "fit"; width: 44; onClicked: toolbar.requestFit() }
        IconButton { iconName: "actual"; width: 44; onClicked: toolbar.requestActual() }

        Item { Layout.fillWidth: true }

        IconButton { iconName: "theme"; width: 44; onClicked: toolbar.requestThemeToggle() }
        IconButton { iconName: "fullscreen"; width: 44; onClicked: toolbar.requestFullscreen() }
        IconButton { iconName: "info"; width: 44; onClicked: toolbar.requestInfo() }
        IconButton { iconName: "settings"; width: 44; onClicked: toolbar.requestSettings() }
    }
}
