import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

// Row of the output list: name, status line and remove/restore/delete buttons.
Rectangle {
    id: root
    property string outputName: ""
    property string outputSubtitle: ""
    property bool isOnline: true
    property bool selected: false
    // Shown in the "removed" list: restore/delete buttons instead of remove
    property bool isRemoved: false
    // Remove/restore/delete are blocked while profiles are being edited
    property bool actionsEnabled: true

    signal clicked()
    signal removeRequested()
    signal restoreRequested()
    signal forgetRequested()

    implicitHeight: 52
    radius: Theme.radiusSmall
    color: selected ? Theme.bgElevated
                    : (rowHover.hovered && !isRemoved ? Theme.bgHover : "transparent")
    border.width: selected ? 1 : 0
    border.color: Theme.accent

    Behavior on color { ColorAnimation { duration: 100 } }

    HoverHandler { id: rowHover }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: !root.isRemoved
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
                color: root.isOnline && !root.isRemoved ? Theme.textPrimary : Theme.textDisabled
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeNormal
                font.strikeout: root.isRemoved
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
                    color: root.isOnline && !root.isRemoved ? Theme.textSecondary : Theme.textDisabled
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    elide: Text.ElideRight
                }
            }
        }

        // Regular list: remove on hover
        IconButton {
            visible: !root.isRemoved && rowHover.hovered
            // Dimmed instead of disabled, so the tooltip still explains why
            opacity: root.actionsEnabled ? 1 : 0.4
            implicitWidth: 26
            implicitHeight: 26
            iconSize: 11
            iconText: Theme.iconHide
            iconColor: Theme.textSecondary
            danger: true
            onClicked: if (root.actionsEnabled) root.removeRequested()

            ToolTip.visible: removeHover.hovered
            ToolTip.delay: 400
            ToolTip.text: root.actionsEnabled
                          ? "Remove output: hide it, stop applying its profiles and hotkeys"
                          : "Finish editing profiles first"
            HoverHandler { id: removeHover }
        }

        // Removed list: restore / delete for good
        IconButton {
            visible: root.isRemoved
            opacity: root.actionsEnabled ? 1 : 0.4
            implicitWidth: 26
            implicitHeight: 26
            iconSize: 12
            iconText: Theme.iconRestore
            iconColor: Theme.accent
            onClicked: if (root.actionsEnabled) root.restoreRequested()

            ToolTip.visible: restoreHover.hovered
            ToolTip.delay: 400
            ToolTip.text: root.actionsEnabled ? "Restore output with its profiles" : "Finish editing profiles first"
            HoverHandler { id: restoreHover }
        }

        IconButton {
            visible: root.isRemoved
            // An online device would come right back as a new output
            readonly property bool allowed: root.actionsEnabled && !root.isOnline
            opacity: allowed ? 1 : 0.4
            implicitWidth: 26
            implicitHeight: 26
            iconSize: 11
            iconText: Theme.iconDelete
            iconColor: Theme.danger
            danger: true
            onClicked: if (allowed) root.forgetRequested()

            ToolTip.visible: forgetHover.hovered
            ToolTip.delay: 400
            ToolTip.text: !root.actionsEnabled ? "Finish editing profiles first"
                        : root.isOnline ? "Disconnect the device to delete it for good"
                        : "Delete for good, with all its profiles"
            HoverHandler { id: forgetHover }
        }
    }

    ToolTip.visible: rowHover.hovered && nameText.truncated
                     && !removeHover.hovered && !restoreHover.hovered && !forgetHover.hovered
    ToolTip.delay: 600
    ToolTip.text: root.outputName
}
