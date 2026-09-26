import QtQuick
import AdvancedVolumeMixer.ThemeModule

// Rule scope switch: a single process, a group of processes ("a.exe; b.exe"),
// or every process on the output. Emits scopeChanged("single"|"group"|"all").
Row {
    id: root
    spacing: Theme.spacingSmall

    property var options: [
        { key: "single", label: "Single process" },
        { key: "group",  label: "Selected group" },
        { key: "all",    label: "All processes" }
    ]
    property string selected: "single"

    signal scopeChanged(string scope)

    Repeater {
        model: root.options
        delegate: Rectangle {
            required property var modelData
            width: label.implicitWidth + 20
            height: 28
            radius: Theme.radiusSmall
            color: root.selected === modelData.key ? Theme.accent : Theme.bgHover

            Behavior on color { ColorAnimation { duration: 100 } }

            Text {
                id: label
                anchors.centerIn: parent
                text: modelData.label
                color: root.selected === modelData.key ? "white" : Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.selected = modelData.key
                    root.scopeChanged(modelData.key)
                }
            }
        }
    }
}
