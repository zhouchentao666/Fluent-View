import QtQuick
import QtQuick.Window
import QtQuick.Controls.FluentWinUI3

// Central image viewer.
//  - Decode happens off the GUI thread via the "viewer" QQuickAsyncImageProvider.
//  - sourceSize is capped to the viewport so even huge images are never fully
//    loaded into CPU memory; the GPU texture is what gets scaled.
//  - Zoom is a transform on the Image element; Flickable handles panning.
Item {
    id: view
    clip: true
    property color bg: ThemeManager.isDark() ? "#0e0e0e" : "#fafafa"

    property real zoom: 1.0
    property real actualScale: 1.0
    property bool loading: img.status === Image.Loading

    signal fit()
    signal actualSize()

    function resetView() {
        zoom = 1.0
        img.scale = zoom
        flick.contentWidth = img.width * zoom
        flick.contentHeight = img.height * zoom
        center()
    }
    function center() {
        flick.contentX = Math.max(0, (flick.contentWidth - flick.width) / 2)
        flick.contentY = Math.max(0, (flick.contentHeight - flick.height) / 2)
    }
    function recompute() {
        if (ImageCtl.imageWidth <= 0 || ImageCtl.imageHeight <= 0 || view.width <= 0)
            return
        var aspect = ImageCtl.imageWidth / ImageCtl.imageHeight
        var fw = view.width, fh = view.width / aspect
        if (fh > view.height) { fh = view.height; fw = fh * aspect }
        img.width = fw
        img.height = fh
        img.scale = zoom
        actualScale = Math.max(ImageCtl.imageWidth / fw, ImageCtl.imageHeight / fh)
        flick.contentWidth = img.width * zoom
        flick.contentHeight = img.height * zoom
        if (zoom <= 1.001) center()
    }
    function zoomAt(factor, cx, cy) {
        var nz = Math.min(16, Math.max(0.05, zoom * factor))
        if (nz === zoom) return
        var ratio = nz / zoom
        var px = flick.contentX + cx
        var py = flick.contentY + cy
        zoom = nz
        img.scale = zoom
        flick.contentWidth = img.width * zoom
        flick.contentHeight = img.height * zoom
        flick.contentX = px * ratio - cx
        flick.contentY = py * ratio - cy
    }

    Component.onCompleted: { fit.connect(resetView); actualSize.connect(() => { zoom = actualScale; img.scale = zoom; flick.contentWidth = img.width*zoom; flick.contentHeight = img.height*zoom; center() }) }

    Rectangle { anchors.fill: parent; color: view.bg }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: img.width * img.scale
        contentHeight: img.height * img.scale
        interactive: zoom > 1.02
        clip: true

        Image {
            id: img
            source: ImageCtl.source
            asynchronous: true
            cache: false
            fillMode: Image.PreserveAspectFit
            transformOrigin: Item.TopLeft
            smooth: true
            sourceSize: Qt.size(view.width * Screen.devicePixelRatio,
                                view.height * Screen.devicePixelRatio)
            opacity: status === Image.Ready ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
        }

        WheelHandler {
            id: wheel
            onWheel: (event) => {
                var f = Math.exp(event.angleDelta.y / 360)
                zoomAt(f, event.position.x, event.position.y)
                event.accepted = true
            }
        }
    }

    Connections {
        target: ImageCtl
        function onPathChanged() { view.recompute(); view.resetView() }
        function onTransformChanged() { /* source already updated; keep view */ }
    }
    onWidthChanged: recompute()
    onHeightChanged: recompute()

    // Double-click: toggle fit <-> 1:1
    TapHandler {
        onTapped: {
            if (tapCount === 2 && button === Qt.LeftButton)
                zoom > 1.02 ? resetView() : actualSize()
        }
    }

    // Drag to pan when zoomed (Flickable already does this); drag to switch
    // images when at fit is handled by keyboard / toolbar.
    Label {
        anchors.centerIn: parent
        text: qsTr("Open an image (Ctrl+O) or drop files here")
        visible: ImageCtl.path === ""
        color: ThemeManager.isDark() ? "#888" : "#666"
        font.pixelSize: 15
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (drop.hasUrls) {
                var paths = drop.urls.map(u => u.toString())
                ImageModel.setFiles(paths)
            }
            drop.accepted = true
        }
    }
}
