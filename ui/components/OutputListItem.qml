import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

Rectangle {
    id: root
    property string outputName: ""
    property string outputSubtitle: ""
    property bool isOnline: true
    property bool selected: false
    property bool canForget: false

    signal clicked()
    signal forgetRequested()

    implicitHeight: 52
    radius: Theme.radiusSmall
    color: selected ? Theme.bgElevated : (rowHover.hovered ? Theme.bgHover : "transparent")
    border.width: selected ? 1 : 0
    border.color: Theme.accent

    Behavior on color { ColorAnimation { duration: 100 } }

    HoverHandler { id: rowHover }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    Rectangle {
        visible: root.selected
        width: 3
        height: parent.height - 16
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        radius: 2
        color: Theme.accent
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingMedium
        anchors.rightMargin: Theme.spacingSmall
        spacing: Theme.spacingSmall

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                id: nameText
                Layout.fillWidth: true
                text: root.outputName
                color: root.isOnline ? Theme.textPrimary : Theme.textDisabled
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeNormal
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                visible: root.outputSubtitle.length > 0

                Rectangle {
                    implicitWidth: 6
                    implicitHeight: 6
                    radius: 3
                    color: root.isOnline ? Theme.success : Theme.textDisabled
                }

                Text {
                    Layout.fillWidth: true
                    text: root.outputSubtitle
                    color: root.isOnline ? Theme.textSecondary : Theme.textDisabled
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    elide: Text.ElideRight
                }
            }
        }

        IconButton {
            visible: root.canForget && rowHover.hovered
            implicitWidth: 24
            implicitHeight: 24
            iconSize: 10
            iconText: Theme.iconCancel
            iconColor: Theme.textSecondary
            danger: true
            onClicked: root.forgetRequested()

            ToolTip.visible: hoverHandler.hovered
            ToolTip.delay: 500
            ToolTip.text: "Forget this output and its profiles"
            HoverHandler { id: hoverHandler }
        }
    }

    ToolTip.visible: rowHover.hovered && nameText.truncated && !hoverHandler.hovered
    ToolTip.delay: 600
    ToolTip.text: root.outputName
}
