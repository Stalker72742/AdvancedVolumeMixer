import QtQuick
import QtQuick.Controls
import AdvancedVolumeMixer.ThemeModule

// Discord-style combo capture:
//   - click to arm ("listening")
//   - modifiers (Ctrl/Alt/Shift/Win) are only collected
//   - pressing a regular key commits "modifiers + key" after commitDelayMs
//   - releasing everything before that commits right away
// Keys are named by scan code with US layout names, so the shown and
// stored combo is the same whatever layout is active (Ь -> M).
// Esc or losing focus cancels the capture.
//
// IMPORTANT: Qt's Keys.* only fires while this item has active focus,
// so this can only ever capture a *local* combo suggestion. Making it
// a real *global* hotkey (works while the app isn't focused) still
// needs OS-level registration - RegisterHotKey / a low-level keyboard
// hook - done on the C++ side (Mixer.hotkeysChanged).
Rectangle {
    id: root

    // Committed combo, owned by the caller
    property var combo: []
    property bool listening: false
    property int commitDelayMs: 100
    // Non-empty when the OS refused to register the combo
    property string errorText: ""

    signal comboCommitted(var combo)
    signal clearRequested()

    implicitHeight: Theme.controlHeight
    radius: Theme.radiusSmall
    color: listening ? Theme.bgElevated : (mouseArea.containsMouse ? Theme.bgPressed : Theme.bgHover)
    border.width: listening || errorText.length > 0 ? 1 : 0
    border.color: listening ? Theme.accent : Theme.danger

    ToolTip.visible: errorText.length > 0 && mouseArea.containsMouse && !listening
    ToolTip.text: errorText

    activeFocusOnTab: true

    QtObject {
        id: capture
        property var held: []
        property var lastNonEmpty: []
    }

    Timer {
        id: commitTimer
        interval: root.commitDelayMs
        repeat: false
        onTriggered: root.commit()
    }

    readonly property var modifierOrder: ["Ctrl", "Alt", "Shift", "Win"]

    function isModifier(name) {
        return modifierOrder.indexOf(name) !== -1
    }

    function hasMainKey(keys) {
        return keys.some(k => !isModifier(k))
    }

    // "Shift + Ctrl + M" and "Ctrl + Shift + M" are the same hotkey
    function normalized(keys) {
        const mods = modifierOrder.filter(m => keys.indexOf(m) !== -1)
        return mods.concat(keys.filter(k => !isModifier(k)))
    }

    function commit() {
        if (hasMainKey(capture.lastNonEmpty))
            root.comboCommitted(capture.lastNonEmpty)
        stopListening()
    }

    function stopListening() {
        commitTimer.stop()
        listening = false
        capture.held = []
        capture.lastNonEmpty = []
    }

    function keyName(event) {
        const byScanCode = Mixer.hotkeyKeyName(event.nativeScanCode)
        if (byScanCode.length > 0)
            return byScanCode

        // Unknown scan code (unusual keyboards): fall back to Qt's key code
        switch (event.key) {
            case Qt.Key_Control: return "Ctrl"
            case Qt.Key_Shift: return "Shift"
            case Qt.Key_Alt: return "Alt"
            case Qt.Key_Meta: return "Win"
        }
        if (event.key >= Qt.Key_F1 && event.key <= Qt.Key_F24)
            return "F" + (event.key - Qt.Key_F1 + 1)
        if ((event.key >= Qt.Key_A && event.key <= Qt.Key_Z) || (event.key >= Qt.Key_0 && event.key <= Qt.Key_9))
            return String.fromCharCode(event.key)
        return "Key_" + event.key
    }

    function addHeld(name) {
        let held = capture.held
        // Only one regular key per hotkey, a new one replaces the previous
        if (!isModifier(name))
            held = held.filter(k => isModifier(k))
        if (held.indexOf(name) === -1)
            held = held.concat([name])
        capture.held = normalized(held)
        capture.lastNonEmpty = capture.held

        if (!isModifier(name))
            commitTimer.restart()
    }

    function removeHeld(name) {
        capture.held = capture.held.filter(k => k !== name)
        // Everything released before the delay ran out: no need to wait
        if (capture.held.length === 0 && commitTimer.running)
            commit()
    }

    Keys.onPressed: (event) => {
        if (!root.listening) return
        event.accepted = true
        if (event.key === Qt.Key_Escape && capture.held.length === 0) {
            root.stopListening()
            return
        }
        if (!event.isAutoRepeat)
            addHeld(keyName(event))
    }

    Keys.onReleased: (event) => {
        if (!root.listening || event.isAutoRepeat) return
        removeHeld(keyName(event))
        event.accepted = true
    }

    onActiveFocusChanged: if (!activeFocus && listening) stopListening()

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (root.listening) return
            root.listening = true
            capture.held = []
            capture.lastNonEmpty = []
            root.forceActiveFocus()
        }
    }

    Text {
        anchors.left: parent.left
        anchors.right: clearButton.visible ? clearButton.left : parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: {
            if (root.listening) {
                return capture.held.length > 0 ? capture.held.join(" + ") : "Press keys..."
            }
            return root.combo.length > 0 ? root.combo.join(" + ") : "Not set"
        }
        color: root.listening ? Theme.accent
                              : root.errorText.length > 0 ? Theme.danger
                              : (root.combo.length > 0 ? Theme.textPrimary : Theme.textSecondary)
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeNormal
    }

    IconButton {
        id: clearButton
        visible: !root.listening && root.combo.length > 0
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        implicitWidth: 28
        implicitHeight: 28
        iconSize: 10
        iconText: Theme.iconCancel
        iconColor: Theme.textSecondary
        onClicked: root.clearRequested()
    }
}
