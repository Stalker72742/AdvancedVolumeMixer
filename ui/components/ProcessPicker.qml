import QtQuick
import QtQuick.Controls
import AdvancedVolumeMixer.ThemeModule

// Editable combobox for executable names: free text with suggestions from
// running processes (typing filters them, the arrow shows all of them).
// Any name is accepted, a rule may target a process that isn't running yet.
Item {
    id: root

    property var processes: []    // [{ name, hasAudio }]
    property bool multiple: false // group scope: complete the name after the last ';'
    property alias text: field.text
    property alias placeholderText: field.placeholderText

    signal accepted()

    implicitWidth: 200
    implicitHeight: field.implicitHeight

    property var filtered: []

    // Part of the text being completed
    readonly property string token: {
        if (!multiple)
            return field.text.trim()
        const parts = field.text.split(/[;,]/)
        return parts[parts.length - 1].trim()
    }

    function focusField() {
        field.forceActiveFocus()
    }

    function refilter(showAll) {
        const t = showAll ? "" : token.toLowerCase()
        const prefix = []
        const contains = []
        for (const p of processes) {
            const n = p.name.toLowerCase()
            if (t.length === 0 || n.startsWith(t))
                prefix.push(p)
            else if (n.indexOf(t) !== -1)
                contains.push(p)
        }
        filtered = prefix.concat(contains)
        suggestions.currentIndex = filtered.length > 0 ? 0 : -1
        suggestions.positionViewAtBeginning()
    }

    function showSuggestions(showAll) {
        refilter(showAll)
        if (filtered.length > 0)
            popup.open()
        else
            popup.close()
    }

    function pick(name) {
        if (multiple) {
            const parts = field.text.split(/[;,]/).map(s => s.trim()).filter(s => s.length > 0)
            if (token.length > 0)
                parts.pop() // replace the half-typed name
            if (parts.indexOf(name) === -1)
                parts.push(name)
            field.text = parts.join("; ") + "; "
        } else {
            field.text = name
        }
        popup.close()
        field.forceActiveFocus()
        field.cursorPosition = field.text.length
    }

    AppTextField {
        id: field
        width: parent.width
        rightPadding: 38

        onTextEdited: {
            if (root.token.length > 0)
                root.showSuggestions(false)
            else
                popup.close()
        }

        Keys.onDownPressed: {
            if (!popup.opened)
                root.showSuggestions(root.token.length === 0)
            else
                suggestions.incrementCurrentIndex()
        }
        Keys.onUpPressed: if (popup.opened) suggestions.decrementCurrentIndex()

        Keys.onTabPressed: (event) => {
            if (popup.opened && suggestions.currentIndex >= 0)
                root.pick(root.filtered[suggestions.currentIndex].name)
            else
                event.accepted = false
        }

        Keys.onEscapePressed: (event) => {
            if (popup.opened)
                popup.close()
            else
                event.accepted = false // let the dialog close
        }

        // Enter takes the highlighted suggestion if the list is open,
        // otherwise the typed text goes as is
        onAccepted: {
            if (popup.opened && suggestions.currentIndex >= 0)
                root.pick(root.filtered[suggestions.currentIndex].name)
            else
                root.accepted()
        }

        onActiveFocusChanged: if (!activeFocus && !chevron.hovered) popup.close()
    }

    IconButton {
        id: chevron
        readonly property bool hovered: chevronHover.hovered
        anchors.right: field.right
        anchors.rightMargin: 4
        anchors.verticalCenter: field.verticalCenter
        implicitWidth: 28
        implicitHeight: 28
        iconSize: 10
        iconText: popup.opened ? "" : "" // ChevronUp / ChevronDown
        iconColor: Theme.textSecondary
        onClicked: {
            if (popup.opened) {
                popup.close()
            } else {
                root.showSuggestions(true)
                field.forceActiveFocus()
            }
        }
        HoverHandler { id: chevronHover }
    }

    Popup {
        id: popup
        y: field.height + 4
        width: root.width
        height: Math.min(suggestions.contentHeight, 240) + topPadding + bottomPadding
        padding: 4
        closePolicy: Popup.CloseOnPressOutsideParent

        background: Rectangle {
            color: Theme.bgSecondary
            radius: Theme.radiusSmall
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ListView {
            id: suggestions
            clip: true
            model: root.filtered
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: 0
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index

                width: ListView.view.width
                height: 30
                radius: Theme.radiusSmall
                color: index === suggestions.currentIndex ? Theme.bgHover : "transparent"

                Text {
                    id: audioIcon
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 14
                    text: row.modelData.hasAudio ? Theme.iconVolume : ""
                    color: Theme.success
                    font.family: Theme.iconFont
                    font.pixelSize: 11
                }

                Text {
                    anchors.left: audioIcon.right
                    anchors.leftMargin: 8
                    anchors.right: playingLabel.visible ? playingLabel.left : parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.name
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeNormal
                    elide: Text.ElideRight
                }

                Text {
                    id: playingLabel
                    visible: row.modelData.hasAudio
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: "playing here"
                    color: Theme.textDisabled
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: suggestions.currentIndex = row.index
                    onClicked: root.pick(row.modelData.name)
                }
            }
        }
    }
}
