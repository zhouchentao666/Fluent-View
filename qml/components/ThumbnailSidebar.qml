import QtQuick
import QtQuick.Controls.FluentWinUI3

// Vertical thumbnail strip. Thumbnails are generated asynchronously by the
// "thumb" QQuickAsyncImageProvider (off GUI thread, cached).
Pane {
    id: sidebar
    padding: 6
    property color panelColor: ThemeManager.isDark() ? "#232323" : "#e7e7e7"
    background: Rectangle { color: sidebar.panelColor }

    ListView {
        id: list
        anchors.fill: parent
        spacing: 6
        clip: true
        model: ImageModel
        currentIndex: ImageModel.currentIndex

        delegate: Item {
            width: list.width
            height: 96
            property bool current: index === ImageModel.currentIndex

            Rectangle {
                anchors.fill: parent
                radius: 6
                color: current
                    ? (ThemeManager.isDark() ? "#0f7bcd" : "#0f7bcd")
                    : (hovered ? (ThemeManager.isDark() ? "#333" : "#dcdcdc") : "transparent")
                opacity: current ? 0.25 : 1
            }
            Rectangle {
                anchors.fill: parent
                radius: 6
                border.color: current ? "#0f7bcd" : "transparent"
                border.width: 2
                color: "transparent"
            }
            Image {
                id: thumb
                anchors.fill: parent
                anchors.margins: 4
                source: "image://thumb/" + encodeURIComponent(model.path)
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            MouseArea {
                anchors.fill: parent
                onClicked: ImageModel.currentIndex = index
                onDoubleClicked: ImageModel.currentIndex = index
            }
        }
    }
}
