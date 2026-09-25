import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            SectionLabel { text: "Process rules" }

            Text {
                visible: rulesView.count > 0
                text: rulesView.count
                color: Theme.textDisabled
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }

            Item { Layout.fillWidth: true }
        }

        ListView {
            id: rulesView
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingSmall
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: Mixer.rules

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ProcessRuleItem {
                // Roles land straight in ProcessRuleItem's own properties
                required property int index
                required processName
                required guid
                required scope
                required volume
                required muted
                required sessionActive

                width: ListView.view.width

                onVolumeEdited: (v) => Mixer.setRuleVolume(index, v)
                onMuteToggled: (m) => Mixer.setRuleMuted(index, m)
                onRemoveRequested: Mixer.removeRule(index)
            }

            // Add button follows the last rule instead of sticking to the bottom
            footer: Column {
                width: ListView.view.width
                topPadding: rulesView.count > 0 ? Theme.spacingSmall : 0
                spacing: Theme.spacingMedium

                Text {
                    visible: rulesView.count === 0
                    width: parent.width
                    text: "No rules yet. Add a process to control its volume in this profile."
                    color: Theme.textDisabled
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WordWrap
                }

                Rectangle {
                    width: parent.width
                    height: 40
                    radius: Theme.radiusSmall
                    color: addMouseArea.containsMouse ? Theme.bgHover : "transparent"
                    border.width: 1
                    border.color: addMouseArea.containsMouse ? Theme.accent : Theme.border

                    Behavior on color { ColorAnimation { duration: 100 } }

                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: Theme.iconAdd
                            color: Theme.textSecondary
                            font.family: Theme.iconFont
                            font.pixelSize: 11
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Add process rule"
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeNormal
                        }
                    }

                    MouseArea {
                        id: addMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: addDialog.open()
                    }
                }
            }
        }
    }

    AddProcessDialog {
        id: addDialog
        onRuleAdded: (rule) => Mixer.addRule(rule.processName, rule.guid, rule.scope)
    }
}
