import QtQuick
import QtQuick.Controls.Basic as Basic
import AdvancedVolumeMixer.ThemeModule

// Basic style on purpose: Material's floating placeholder and underline
// fight with a custom background.
Basic.TextField {
    id: root

    implicitHeight: Theme.controlHeight
    leftPadding: 10
    rightPadding: 10
    topPadding: 0
    bottomPadding: 0
    verticalAlignment: TextInput.AlignVCenter

    color: Theme.textPrimary
    placeholderTextColor: Theme.textDisabled
    selectionColor: Theme.accent
    selectedTextColor: "white"
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeNormal
    selectByMouse: true

    background: Rectangle {
        color: Theme.bgHover
        radius: Theme.radiusSmall
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : "transparent"
    }
}
