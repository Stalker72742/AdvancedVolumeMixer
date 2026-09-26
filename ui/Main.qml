import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import AdvancedVolumeMixer.ComponentsModule
import AdvancedVolumeMixer.ThemeModule

// Main window: title bar, output list on the left, profile page on the right.
ApplicationWindow {
    id: window
    width: 980
    height: 640
    minimumWidth: 820
    minimumHeight: 560
    visible: true
    color: Theme.bgPrimary
    title: "Audio Profile Switcher"

    // Frameless so TitleBar.qml can draw its own chrome.
    flags: Qt.Window | Qt.FramelessWindowHint

    Material.theme: Material.Dark
    Material.accent: Theme.accent
    Material.primary: Theme.accent
    Material.background: Theme.bgElevated
    Material.foreground: Theme.textPrimary

    // Set from main.cpp once the tray icon is up. Then closing the window
    // (title bar button, Alt+F4, taskbar) only hides it; Quit lives in the tray menu.
    property bool trayModeEnabled: false

    // Handled by TrayManager::hideWindow()
    signal hideToTrayRequested()

    onClosing: (close) => {
        if (trayModeEnabled) {
            close.accepted = false
            hideToTrayRequested()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1 // leave room for the window outline
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            title: window.title
            trayModeEnabled: window.trayModeEnabled

            onMinimizeRequested: window.showMinimized()
            onCloseRequested: (toTray) => {
                if (toTray)
                    window.hideToTrayRequested()
                else
                    Qt.quit()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.border
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            OutputListPanel {
                Layout.preferredWidth: 240
                Layout.fillHeight: true
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.border
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                ProfileToolbar {
                    Layout.fillWidth: true
                }

                // Empty state: nothing to show without an output
                Text {
                    visible: Mixer.currentOutputIndex < 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: Mixer.removedCount > 0
                          ? "All outputs are removed. Restore one from the Removed list on the left"
                          : "Connect an audio output to start creating profiles"
                    color: Theme.textDisabled
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeNormal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                ColumnLayout {
                    visible: Mixer.currentOutputIndex >= 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: Theme.pagePadding
                    spacing: Theme.spacingLarge

                    ProfileScroller {
                        Layout.fillWidth: true
                    }

                    ProfileSettingsPanel {
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.border
                    }

                    ProcessRuleList {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }

    // Window outline, frameless windows have none on Windows 10
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: Mixer.editMode ? Theme.warning : Theme.border
        z: 100
    }

    WindowResizeHandles {
        z: 101
    }
}
