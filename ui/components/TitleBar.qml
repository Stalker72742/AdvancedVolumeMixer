import QtQuick
import QtQuick.Window
import AdvancedVolumeMixer.ThemeModule

Rectangle {
    id: root
    implicitHeight: Theme.titleBarHeight
    color: Theme.bgSecondary

    property string title: "Audio Profile Switcher"

    // Flip this once the C++ side actually has a QSystemTrayIcon alive.
    // While false, the close button behaves like a normal window close.
    property bool trayModeEnabled: false

    signal minimizeRequested()
    // toTray == true means "hide instead of terminating the process"
    signal closeRequested(bool toTray)

    // Drag-to-move region: whole bar minus the button cluster.
    MouseArea {
        anchors.fill: parent
        anchors.rightMargin: buttonsRow.width
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                root.Window.window.startSystemMove()
            }
        }
    }

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: Theme.spacingMedium
        spacing: Theme.spacingSmall + 2

        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: Theme.accent
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeNormal
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Row {
        id: buttonsRow
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        IconButton {
            width: 46
            height: parent.height
            radius: 0
            iconSize: 10
            iconText: Theme.iconMinimize
            onClicked: root.minimizeRequested()
        }
        IconButton {
            width: 46
            height: parent.height
            radius: 0
            iconSize: 10
            iconText: Theme.iconClose
            danger: true
            onClicked: root.closeRequested(root.trayModeEnabled)
        }
    }
}
