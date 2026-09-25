import QtQuick
import QtQuick.Controls
import AdvancedVolumeMixer.ThemeModule

// A saved, reusable (scope, volume, mute) setting. Process rules point
// at a template by id instead of duplicating volume/scope themselves,
// so editing a template here updates every rule that uses it.
Rectangle {
    id: root
    color: Theme.bgHover
    radius: Theme.radiusSmall
    height: 64

    property string templateName: "New template"
    property string scope: "single"
    property real volume: 1.0
    property bool muted: false

    signal edited()
    signal removed()

    Row {
        anchors.fill: parent
        anchors.margins: Theme.spacingSmall
        spacing: Theme.spacingMedium

        Column {
            width: 140
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2
            Text {
                text: root.templateName
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeNormal
            }
            Text {
                text: root.scope === "single" ? "Single process"
                      : root.scope === "group" ? "Process group"
                      : "All processes"
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }
        }

        Slider {
            id: volumeSlider
            width: 140
            anchors.verticalCenter: parent.verticalCenter
            from: 0
            to: 1
            value: root.volume
            onMoved: {
                root.volume = value
                root.edited()
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: Math.round(volumeSlider.value * 100) + "%"
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            width: 36
        }

        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconText: root.muted ? "\uD83D\uDD07" : "\uD83D\uDD0A"
            onClicked: {
                root.muted = !root.muted
                root.edited()
            }
        }

        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            iconText: "\u2715"
            danger: true
            onClicked: root.removed()
        }
    }
}
