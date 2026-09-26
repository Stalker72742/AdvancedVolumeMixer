import QtQuick
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

// Name and hotkey of the shown profile, Delete in edit mode.
Item {
    id: root
    implicitHeight: layout.implicitHeight

    RowLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.spacingLarge

        ColumnLayout {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 3
            Layout.preferredWidth: 1
            spacing: Theme.spacingSmall

            SectionLabel { text: "Profile name" }

            AppTextField {
                id: nameField
                Layout.fillWidth: true
                text: Mixer.currentProfileName
                placeholderText: "Profile name"
                onEditingFinished: Mixer.renameCurrentProfile(text)
                Keys.onEscapePressed: {
                    text = Mixer.currentProfileName
                    focus = false
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.horizontalStretchFactor: 2
            Layout.preferredWidth: 1
            Layout.minimumWidth: 200
            spacing: Theme.spacingSmall

            SectionLabel { text: "Switch hotkey" }

            HotkeyRecorder {
                Layout.fillWidth: true
                combo: Mixer.currentProfileHotkey
                errorText: Mixer.currentProfileHotkeyError
                onComboCommitted: (combo) => Mixer.setCurrentProfileHotkey(combo)
                onClearRequested: Mixer.setCurrentProfileHotkey([])
            }
        }

        // Deleting is structural, so it is only offered in edit mode
        ColumnLayout {
            visible: Mixer.editMode
            Layout.alignment: Qt.AlignBottom
            spacing: Theme.spacingSmall

            AppButton {
                implicitHeight: Theme.controlHeight
                kind: "danger"
                iconText: Theme.iconDelete
                text: "Delete"
                enabled: Mixer.profiles.count > 1
                onClicked: Mixer.removeProfile(Mixer.currentProfileIndex)
            }
        }
    }
}
