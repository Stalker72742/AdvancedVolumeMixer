import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

// Left panel: known audio outputs, or the removed ones (Mixer.showRemoved).
Rectangle {
    id: root
    color: Theme.bgSecondary

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.pagePadding - 6
        anchors.bottomMargin: Theme.spacingMedium
        anchors.leftMargin: Theme.spacingMedium
        anchors.rightMargin: Theme.spacingMedium
        spacing: Theme.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 4
            Layout.preferredHeight: 28
            spacing: Theme.spacingSmall

            SectionLabel {
                Layout.fillWidth: true
                text: Mixer.showRemoved ? "Removed outputs" : "Audio outputs"
                color: Mixer.showRemoved ? Theme.warning : Theme.textSecondary
            }

            // Switch between the regular and the removed list
            Rectangle {
                visible: Mixer.removedCount > 0 || Mixer.showRemoved
                implicitWidth: toggleRow.implicitWidth + 16
                implicitHeight: 24
                radius: Theme.radiusSmall
                color: toggleArea.containsMouse ? Theme.bgHover : "transparent"
                border.width: 1
                border.color: Theme.border

                Row {
                    id: toggleRow
                    anchors.centerIn: parent
                    spacing: 5

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: Mixer.showRemoved ? Theme.iconBack : Theme.iconDelete
                        color: Theme.textSecondary
                        font.family: Theme.iconFont
                        font.pixelSize: 10
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: Mixer.showRemoved ? "Back" : "Removed " + Mixer.removedCount
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }

                MouseArea {
                    id: toggleArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Mixer.showRemoved = !Mixer.showRemoved
                }
            }
        }

        Text {
            visible: Mixer.showRemoved
            Layout.fillWidth: true
            Layout.leftMargin: 4
            text: "Removed outputs are ignored: no profiles, no hotkeys. They stay here when the device reconnects."
            color: Theme.textDisabled
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.WordWrap
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds

            model: Mixer.outputs

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: OutputListItem {
                // idx = index in Mixer's full list, the model itself is filtered
                required property int idx
                required property string name
                required property bool online
                required property bool isDefault
                required property bool removed
                required property string activeProfileName

                width: ListView.view.width
                outputName: name
                isOnline: online
                isRemoved: removed
                actionsEnabled: !Mixer.editMode
                outputSubtitle: removed ? (online ? "Removed · connected" : "Removed · offline")
                              : !online ? "Offline"
                              : (isDefault ? "System default · " : "") + activeProfileName
                selected: !removed && idx === Mixer.currentOutputIndex

                onClicked: Mixer.selectOutput(idx)
                onRemoveRequested: Mixer.removeOutput(idx)
                onRestoreRequested: Mixer.restoreOutput(idx)
                onForgetRequested: Mixer.forgetOutput(idx)
            }

            Text {
                anchors.centerIn: parent
                width: parent.width - 16
                visible: listView.count === 0
                text: Mixer.showRemoved ? "No removed outputs" : "No audio outputs found"
                color: Theme.textDisabled
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }
}
