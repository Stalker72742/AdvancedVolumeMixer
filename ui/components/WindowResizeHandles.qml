import QtQuick
import QtQuick.Window

// Invisible edge/corner grips for a frameless window.
Item {
    id: root
    property int grip: 5

    anchors.fill: parent

    component Grip: MouseArea {
        required property int edges
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onPressed: root.Window.window.startSystemResize(edges)
    }

    Grip { edges: Qt.LeftEdge;   cursorShape: Qt.SizeHorCursor; x: 0; y: root.grip; width: root.grip; height: root.height - 2 * root.grip }
    Grip { edges: Qt.RightEdge;  cursorShape: Qt.SizeHorCursor; x: root.width - root.grip; y: root.grip; width: root.grip; height: root.height - 2 * root.grip }
    Grip { edges: Qt.TopEdge;    cursorShape: Qt.SizeVerCursor; x: root.grip; y: 0; width: root.width - 2 * root.grip; height: root.grip }
    Grip { edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor; x: root.grip; y: root.height - root.grip; width: root.width - 2 * root.grip; height: root.grip }

    Grip { edges: Qt.TopEdge | Qt.LeftEdge;     cursorShape: Qt.SizeFDiagCursor; x: 0; y: 0; width: root.grip; height: root.grip }
    Grip { edges: Qt.TopEdge | Qt.RightEdge;    cursorShape: Qt.SizeBDiagCursor; x: root.width - root.grip; y: 0; width: root.grip; height: root.grip }
    Grip { edges: Qt.BottomEdge | Qt.LeftEdge;  cursorShape: Qt.SizeBDiagCursor; x: 0; y: root.height - root.grip; width: root.grip; height: root.grip }
    Grip { edges: Qt.BottomEdge | Qt.RightEdge; cursorShape: Qt.SizeFDiagCursor; x: root.width - root.grip; y: root.height - root.grip; width: root.grip; height: root.grip }
}
