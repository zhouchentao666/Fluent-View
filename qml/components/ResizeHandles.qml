import QtQuick
import QtQuick.Window

// Edge/corner resize handles for a frameless window. Uses the native
// startSystemResize so behavior matches the OS. Disabled in FullScreen.
Item {
    anchors.fill: parent
    visible: Window.window.visibility !== Window.FullScreen

    function make(edges, x, y, w, h, cursor) {
        return resizeComp.createObject(resizeLayer, {
            "x": x, "y": y, "width": w, "height": h,
            "edges": edges, "cursorShape": cursor
        });
    }

    id: resizeLayer
    Component {
        id: resizeComp
        MouseArea {
            property int edges
            cursorShape: Qt.ArrowCursor
            onPressed: (mouse) => Window.window.startSystemResize(edges)
        }
    }

    Component.onCompleted: {
        const m = 6;
        make(Qt.LeftEdge, 0, m, m, parent.height - 2*m, Qt.SizeHorCursor);
        make(Qt.RightEdge, parent.width - m, m, m, parent.height - 2*m, Qt.SizeHorCursor);
        make(Qt.TopEdge, m, 0, parent.width - 2*m, m, Qt.SizeVerCursor);
        make(Qt.BottomEdge, m, parent.height - m, parent.width - 2*m, m, Qt.SizeVerCursor);
        make(Qt.TopEdge | Qt.LeftEdge, 0, 0, m, m, Qt.SizeFDiagCursor);
        make(Qt.TopEdge | Qt.RightEdge, parent.width - m, 0, m, m, Qt.SizeBDiagCursor);
        make(Qt.BottomEdge | Qt.LeftEdge, 0, parent.height - m, m, m, Qt.SizeBDiagCursor);
        make(Qt.BottomEdge | Qt.RightEdge, parent.width - m, parent.height - m, m, m, Qt.SizeFDiagCursor);
    }
}
