import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AdvancedVolumeMixer.ThemeModule

// One process rule: session status, name, volume slider, mute and remove.
Rectangle {
    id: root
    color: Theme.bgSecondary
    radius: Theme.radiusSmall
    implicitHeight: 56

    property string processName: ""
    property string guid: ""
    property string scope: "single"
    property real volume: 1.0
    property bool muted: false
    // Process currently has an audio session on this output. When false the
    // rule waits and is applied as soon as the process starts playing.
    property bool sessionActive: false

    signal volumeEdited(real value)
    signal muteToggled(bool muted)
    signal removeRequested()

    readonly property string scopeLabel: scope === "group" ? "Process group"
                                       : scope === "all" ? "All processes"
                                       : "Single process"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingMedium
        anchors.rightMargin: Theme.spacingSmall
        spacing: Theme.spacingMedium

        Rectangle {
            implicitWidth: 8
            implicitHeight: 8
            radius: 4
            color: root.sessionActive ? Theme.success : Theme.textDisabled

            ToolTip.visible: dotHover.hovered
            ToolTip.text: root.sessionActive ? "Audio session is active" : "No active audio session, rule will be applied when it appears"
            HoverHandler { id: dotHover }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 80
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: root.processName
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeNormal
                elide: Text.ElideMiddle
            }
            Text {
                Layout.fillWidth: true
                text: root.guid.length > 0 ? root.scopeLabel + " · " + root.guid : root.scopeLabel
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                elide: Text.ElideRight
            }
        }

        Slider {
            id: slider
            Layout.preferredWidth: 180
            Layout.minimumWidth: 100
            Layout.fillWidth: false
            from: 0
            to: 1
            value: root.volume
            enabled: !root.muted
            onMoved: root.volumeEdited(value)
        }

        Text {
            Layout.preferredWidth: 40
            horizontalAlignment: Text.AlignRight
            text: Math.round(slider.value * 100) + "%"
            color: root.muted ? Theme.textDisabled : Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        IconButton {
            iconText: root.muted ? Theme.iconMute : Theme.iconVolume
            iconSize: 14
            iconColor: root.muted ? Theme.danger : Theme.textPrimary
            onClicked: root.muteToggled(!root.muted)

            ToolTip.visible: muteHover.hovered
            ToolTip.delay: 500
            ToolTip.text: root.muted ? "Unmute" : "Mute"
            HoverHandler { id: muteHover }
        }

        IconButton {
            iconText: Theme.iconCancel
            iconColor: Theme.textSecondary
            danger: true
            onClicked: root.removeRequested()

            ToolTip.visible: removeHover.hovered
            ToolTip.delay: 500
            ToolTip.text: "Remove rule"
            HoverHandler { id: removeHover }
        }
    }
}
