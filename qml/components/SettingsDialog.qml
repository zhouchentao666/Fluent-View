import QtQuick
import QtQuick.Controls.FluentWinUI3

Dialog {
    id: dlg
    title: qsTr("Settings")
    modal: true
    width: 420
    height: 420
    standardButtons: Dialog.Ok | Dialog.Cancel

    property string tempTheme: Settings.theme
    property int tempZoom: Settings.zoomMode
    property bool tempRemember: Settings.rememberWindow
    property bool tempSidebar: Settings.thumbnailSidebar
    property int tempCache: Settings.maxCacheMB

    onAboutToShow: {
        tempTheme = Settings.theme
        tempZoom = Settings.zoomMode
        tempRemember = Settings.rememberWindow
        tempSidebar = Settings.thumbnailSidebar
        tempCache = Settings.maxCacheMB
    }
    onAccepted: {
        Settings.theme = tempTheme
        Settings.zoomMode = tempZoom
        Settings.rememberWindow = tempRemember
        Settings.thumbnailSidebar = tempSidebar
        Settings.maxCacheMB = tempCache
        ThemeManager.apply()
    }

    Column {
        spacing: 14
        width: parent.width - 40
        x: 20; y: 16

        Label { text: qsTr("Appearance"); font.weight: Font.Medium }
        Row {
            width: parent.width
            spacing: 12
            Label { text: qsTr("Theme"); width: 120; anchors.verticalCenter: parent.verticalCenter }
            ComboBox {
                width: parent.width - 132
                textRole: "text"; valueRole: "value"
                model: [
                    { text: qsTr("System"), value: "system" },
                    { text: qsTr("Light"), value: "light" },
                    { text: qsTr("Dark"), value: "dark" }
                ]
                currentIndex: { for (var i=0;i<count;i++) if (model[i].value===tempTheme) return i; return 0 }
                onActivated: tempTheme = model[index].value
            }
        }

        Label { text: qsTr("Viewing"); font.weight: Font.Medium }
        Row {
            width: parent.width
            spacing: 12
            Label { text: qsTr("Default zoom"); width: 120; anchors.verticalCenter: parent.verticalCenter }
            ComboBox {
                width: parent.width - 132
                model: [qsTr("Fit to window"), qsTr("Actual size (1:1)"), qsTr("Stretch")]
                currentIndex: tempZoom
                onActivated: tempZoom = index
            }
        }

        CheckBox { text: qsTr("Remember window size and position"); checked: tempRemember; onToggled: tempRemember = checked }
        CheckBox { text: qsTr("Show thumbnail sidebar"); checked: tempSidebar; onToggled: tempSidebar = checked }

        Label { text: qsTr("Performance"); font.weight: Font.Medium }
        Row {
            width: parent.width
            spacing: 12
            Label { text: qsTr("Max cache (MB)"); width: 120; anchors.verticalCenter: parent.verticalCenter }
            SpinBox { from: 64; to: 8192; stepSize: 64; value: tempCache; onValueChanged: tempCache = value }
        }
    }
}
