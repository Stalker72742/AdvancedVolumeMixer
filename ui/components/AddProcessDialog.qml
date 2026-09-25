import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

Popup {
    id: root
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: 440
    padding: Theme.spacingLarge

    property string scope: "single"
    property var processes: []

    readonly property string processText: processPicker.text.trim().replace(/[;,\s]+$/, "")
    readonly property bool canAdd: scope === "all" || processText.length > 0

    signal ruleAdded(var rule)

    function submit() {
        if (!canAdd) return
        ruleAdded({
            scope: root.scope,
            processName: root.scope === "all" ? "All processes" : root.processText,
            guid: guidField.text.trim()
        })
        close()
    }

    onOpened: {
        // Snapshot on open: the list is only a hint, the process may not run yet
        processes = Mixer.runningProcesses()
        processPicker.text = ""
        guidField.text = ""
        scopeSelector.selected = "single"
        root.scope = "single"
        processPicker.focusField()
    }

    Overlay.modal: Rectangle { color: "#99000000" }

    background: Rectangle {
        color: Theme.bgElevated
        radius: Theme.radiusMedium
        border.width: 1
        border.color: Theme.border
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMedium

        Text {
            text: "Add process rule"
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
            font.weight: Font.DemiBold
        }

        TemplateSelector {
            id: scopeSelector
            onScopeChanged: (s) => root.scope = s
        }

        ColumnLayout {
            visible: root.scope !== "all"
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSmall
            spacing: Theme.spacingSmall

            SectionLabel {
                text: root.scope === "group" ? "Processes (separated by ;)" : "Process name or executable"
            }

            ProcessPicker {
                id: processPicker
                Layout.fillWidth: true
                processes: root.processes
                multiple: root.scope === "group"
                placeholderText: root.scope === "group" ? "e.g. Discord.exe; Telegram.exe"
                                                        : "Start typing or pick from the list"
                onAccepted: root.submit()
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            SectionLabel { text: "Fixed identifier / GUID (optional)" }

            AppTextField {
                id: guidField
                Layout.fillWidth: true
                placeholderText: "Matches the process before it opens a session"
                onAccepted: root.submit()
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.scope === "all"
                  ? "Applies to every process playing on this output that has no more specific rule."
                  : "The process doesn't have to be running: the rule is kept and applied as soon as it starts playing audio."
            color: Theme.textDisabled
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSmall
            spacing: Theme.spacingSmall

            Item { Layout.fillWidth: true }

            AppButton {
                kind: "ghost"
                text: "Cancel"
                onClicked: root.close()
            }

            AppButton {
                kind: "primary"
                text: "Add"
                enabled: root.canAdd
                onClicked: root.submit()
            }
        }
    }
}
