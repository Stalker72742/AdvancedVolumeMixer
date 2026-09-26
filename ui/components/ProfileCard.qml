import QtQuick
import AdvancedVolumeMixer.ThemeModule

// Profile tile in the profile strip, or the "New profile" tile.
Rectangle {
    id: root
    property string profileName: ""
    property bool selected: false
    // Profile currently applied to the output (differs from `selected` only in edit mode)
    property bool active: false
    property bool isAddCard: false

    signal clicked()

    implicitWidth: 120
    implicitHeight: 84
    radius: Theme.radiusMedium
    color: isAddCard
           ? (mouseArea.containsMouse ? Theme.bgSecondary : "transparent")
           : (selected ? Theme.bgElevated : (mouseArea.containsMouse ? Theme.bgHover : Theme.bgSecondary))
    border.width: selected ? 2 : 1
    border.color: selected ? Theme.accent : Theme.border

    Behavior on color { ColorAnimation { duration: 100 } }

    // "Applied" marker
    Rectangle {
        visible: root.active && !root.isAddCard
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        width: 8
        height: 8
        radius: 4
        color: Theme.success
    }

    Column {
        anchors.centerIn: parent
        spacing: 8

        Text {
            visible: root.isAddCard
            text: Theme.iconAdd
            color: Theme.textSecondary
            font.family: Theme.iconFont
            font.pixelSize: 16
            height: 28
            verticalAlignment: Text.AlignVCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            visible: !root.isAddCard
            width: 28
            height: 28
            radius: 14
            color: root.selected ? Theme.accent : Theme.bgHover
            anchors.horizontalCenter: parent.horizontalCenter

            Text {
                anchors.centerIn: parent
                text: root.profileName.length > 0 ? root.profileName.charAt(0).toUpperCase() : "?"
                color: "white"
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeNormal
                font.bold: true
            }
        }

        Text {
            text: root.isAddCard ? "New profile" : root.profileName
            color: root.isAddCard ? Theme.textSecondary : Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            anchors.horizontalCenter: parent.horizontalCenter
            elide: Text.ElideRight
            width: root.width - 16
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
