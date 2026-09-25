import QtQuick
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

// Page header: output name + edit mode switch.
Rectangle {
    id: root
    implicitHeight: 52
    color: Mixer.editMode ? Theme.warningBg : "transparent"

    Behavior on color { ColorAnimation { duration: 150 } }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Mixer.editMode ? Theme.warning : Theme.border
        opacity: Mixer.editMode ? 0.6 : 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.pagePadding
        anchors.rightMargin: Theme.pagePadding
        spacing: Theme.spacingMedium

        Text {
            visible: Mixer.editMode
            text: Theme.iconEdit
            color: Theme.warning
            font.family: Theme.iconFont
            font.pixelSize: 14
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Text {
                Layout.fillWidth: true
                text: Mixer.editMode ? "Edit mode"
                                     : (Mixer.currentOutputName.length > 0 ? Mixer.currentOutputName : "No output selected")
                color: Mixer.editMode ? Theme.warning : Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: Mixer.editMode
                      ? "Changes are applied only after Accept. Clicking a profile opens it without switching."
                      : (Mixer.currentOutputOnline ? "Click a profile to switch to it" : "Output is offline, profile will apply when it reconnects")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                elide: Text.ElideRight
            }
        }

        AppButton {
            visible: !Mixer.editMode
            enabled: Mixer.currentOutputIndex >= 0
            kind: "secondary"
            iconText: Theme.iconEdit
            text: "Edit profiles"
            onClicked: Mixer.beginEdit()
        }

        AppButton {
            visible: Mixer.editMode
            kind: "secondary"
            text: "Cancel"
            onClicked: Mixer.cancelEdit()
        }

        AppButton {
            visible: Mixer.editMode
            kind: "primary"
            iconText: Theme.iconAccept
            text: "Accept"
            onClicked: Mixer.acceptEdit()
        }
    }
}
