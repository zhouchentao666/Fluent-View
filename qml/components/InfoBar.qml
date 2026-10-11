import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3

// Bottom status bar: resolution, format, file size, path.
Item {
    id: info
    height: 30
    property color panelColor: ThemeManager.isDark() ? "#2b2b2b" : "#ebebeb"
    Rectangle { anchors.fill: parent; color: info.panelColor }

    function humanSize(bytes) {
        if (bytes <= 0) return "—"
        var u = ["B", "KB", "MB", "GB"]
        var i = 0, v = bytes
        while (v >= 1024 && i < u.length - 1) { v /= 1024; i++ }
        return v.toFixed(i ? 1 : 0) + " " + u[i]
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 14
        Label { text: ImageCtl.fileName || "—"; elide: Text.ElideMiddle; Layout.fillWidth: true; color: ThemeManager.isDark() ? "#E6E6E6" : "#1F1F1F" }
        Label { text: (ImageCtl.imageWidth > 0 ? ImageCtl.imageWidth + " × " + ImageCtl.imageHeight : "—"); color: "#888" }
        Label { text: ImageCtl.format || "—"; color: "#888" }
        Label { text: info.humanSize(ImageCtl.fileSize); color: "#888" }
        Label { text: ImageCtl.path || ""; elide: Text.ElideLeft; color: "#666"; Layout.maximumWidth: 360 }
    }
}
