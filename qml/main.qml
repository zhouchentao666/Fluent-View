import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.FluentWinUI3
import QtQuick.Dialogs

Window {
    id: root
    width: 1100
    height: 720
    minimumWidth: 480
    minimumHeight: 360
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"
    visible: true
    title: "Fluent View"

    property bool fullscreen: root.visibility === Window.FullScreen
    property bool showInfo: true
    property color bg: ThemeManager.isDark() ? "#1f1f1f" : "#f3f3f3"

    function updateColors() { root.bg = ThemeManager.isDark() ? "#1f1f1f" : "#f3f3f3" }
    function toggleFullscreen() {
        root.fullscreen ? root.showNormal() : root.showFullScreen()
    }
    function zoomBy(f) { imageView.zoomAt(f, imageView.width / 2, imageView.height / 2) }

    Component.onCompleted: {
        if (Settings.rememberWindow && !Settings.windowGeometry.isEmpty())
            root.geometry = Settings.windowGeometry
        updateColors()
    }
    Component.onDestruction: {
        if (Settings.rememberWindow)
            Settings.windowGeometry = Qt.rect(root.x, root.y, root.width, root.height)
    }
    Connections {
        target: ThemeManager
        function onThemeApplied() { updateColors() }
    }

    // ---- Background --------------------------------------------------------
    Rectangle { anchors.fill: parent; color: root.bg; radius: 0 }

    // ---- Layout ------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar { id: titleBar; visible: !root.fullscreen; Layout.fillWidth: true }
        ToolBar {
            id: toolbar
            visible: !root.fullscreen
            Layout.fillWidth: true
            onRequestOpen: openDlg.open()
            onRequestFit: imageView.fit()
            onRequestActual: imageView.actualSize()
            onRequestFullscreen: toggleFullscreen()
            onRequestInfo: showInfo = !showInfo
            onRequestSettings: settingsDlg.open()
            onRequestThemeToggle: {
                Settings.theme = (ThemeManager.effectiveTheme() === "dark") ? "light" : "dark"
                ThemeManager.apply()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            ImageView { id: imageView; Layout.fillWidth: true; Layout.fillHeight: true }

            ThumbnailSidebar {
                id: sidebar
                visible: Settings.thumbnailSidebar && !root.fullscreen
                Layout.fillHeight: true
                Layout.preferredWidth: 176
            }
        }

        InfoBar { id: infoBar; visible: showInfo && !root.fullscreen; Layout.fillWidth: true }
    }

    // ---- Overlays ----------------------------------------------------------
    ResizeHandles { }
    BusyOverlay { active: imageView.loading }

    // ---- Context menu ------------------------------------------------------
    Menu {
        id: ctxMenu
        MenuItem { text: qsTr("Copy image"); enabled: ImageCtl.path !== ""; onTriggered: Platform.copyImageToClipboard(ImageCtl.path) }
        MenuItem { text: qsTr("Set as wallpaper"); enabled: ImageCtl.path !== "" && Platform.isDesktop; onTriggered: Platform.setAsWallpaper(ImageCtl.path) }
        MenuItem { text: qsTr("Open containing folder"); enabled: ImageCtl.path !== ""; onTriggered: Platform.openContainingFolder(ImageCtl.path) }
        MenuSeparator { }
        MenuItem { text: qsTr("Toggle info bar"); onTriggered: showInfo = !showInfo }
        MenuItem { text: qsTr("Toggle thumbnails"); onTriggered: Settings.thumbnailSidebar = !Settings.thumbnailSidebar }
        MenuItem { text: qsTr("Settings"); onTriggered: settingsDlg.open() }
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: ctxMenu.popup(position.x, position.y)
    }

    // ---- Dialogs -----------------------------------------------------------
    FileDialog {
        id: openDlg
        title: qsTr("Open image(s)")
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp *.avif *.bmp *.gif *.heic *.heif *.tiff *.tif *.svg)", "All files (*)"]
        onAccepted: ImageModel.setFiles(openDlg.selectedFiles.map(u => u.toString()))
    }
    SettingsDialog { id: settingsDlg }

    // ---- Shortcuts ---------------------------------------------------------
    Shortcut { sequence: "Ctrl+O"; onActivated: openDlg.open() }
    Shortcut { sequence: "Left"; onActivated: ImageModel.previous() }
    Shortcut { sequence: "Right"; onActivated: ImageModel.next() }
    Shortcut { sequence: "J"; onActivated: ImageModel.next() }
    Shortcut { sequence: "K"; onActivated: ImageModel.previous() }
    Shortcut { sequence: "Space"; onActivated: ImageModel.next() }
    Shortcut { sequence: "Backspace"; onActivated: ImageModel.previous() }
    Shortcut { sequence: "F"; onActivated: toggleFullscreen() }
    Shortcut { sequence: "Escape"; onActivated: if (root.fullscreen) toggleFullscreen() }
    Shortcut { sequence: "0"; onActivated: imageView.fit() }
    Shortcut { sequence: "1"; onActivated: imageView.actualSize() }
    Shortcut { sequence: "R"; onActivated: ImageCtl.rotateRight() }
    Shortcut { sequence: "I"; onActivated: showInfo = !showInfo }
    Shortcut { sequence: "Ctrl+C"; onActivated: if (ImageCtl.path) Platform.copyImageToClipboard(ImageCtl.path) }
    Shortcut { sequence: "Ctrl++"; onActivated: zoomBy(1.2) }
    Shortcut { sequence: "Ctrl+-"; onActivated: zoomBy(1 / 1.2) }
}
