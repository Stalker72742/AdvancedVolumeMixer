import QtQuick
import AdvancedVolumeMixer.ThemeModule

// Flat square button with a Segoe MDL2 icon glyph.
Rectangle {
    id: root
    property string iconText: ""
    property bool danger: false
    property color iconColor: Theme.textPrimary
    property int iconSize: 12

    signal clicked()

    implicitWidth: 32
    implicitHeight: 32
    radius: Theme.radiusSmall
    color: mouseArea.containsMouse
           ? (danger ? Theme.danger : Theme.bgHover)
           : "transparent"
    opacity: enabled ? 1 : 0.4

    Behavior on color { ColorAnimation { duration: 100 } }

    Text {
        anchors.centerIn: parent
        text: root.iconText
        color: mouseArea.containsMouse && root.danger ? "white" : root.iconColor
        font.family: Theme.iconFont
        font.pixelSize: root.iconSize
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
