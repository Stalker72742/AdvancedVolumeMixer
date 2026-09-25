import QtQuick
import AdvancedVolumeMixer.ThemeModule

// kind: "primary" | "secondary" | "ghost" | "danger"
Rectangle {
    id: root
    property string text: ""
    property string iconText: ""
    property string kind: "secondary"

    signal clicked()

    readonly property color baseColor: kind === "primary" ? Theme.accent
                                     : kind === "danger" ? "transparent"
                                     : kind === "ghost" ? "transparent"
                                     : Theme.bgHover
    readonly property color hoverColor: kind === "primary" ? Theme.accentHover
                                      : kind === "danger" ? Theme.danger
                                      : Theme.bgPressed
    readonly property color contentColor: kind === "primary" ? "white"
                                        : kind === "danger" ? (mouseArea.containsMouse ? "white" : Theme.danger)
                                        : Theme.textPrimary

    implicitWidth: content.implicitWidth + 28
    implicitHeight: 32
    radius: Theme.radiusSmall
    color: mouseArea.containsMouse && enabled ? hoverColor : baseColor
    border.width: kind === "danger" ? 1 : 0
    border.color: Theme.danger
    opacity: enabled ? 1 : 0.45

    Behavior on color { ColorAnimation { duration: 100 } }

    Row {
        id: content
        anchors.centerIn: parent
        spacing: 8

        Text {
            visible: root.iconText.length > 0
            anchors.verticalCenter: parent.verticalCenter
            text: root.iconText
            color: root.contentColor
            font.family: Theme.iconFont
            font.pixelSize: 12
        }

        Text {
            visible: root.text.length > 0
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.contentColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeNormal
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled) root.clicked()
    }
}
