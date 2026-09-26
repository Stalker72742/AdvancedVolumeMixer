import QtQuick
import AdvancedVolumeMixer.ThemeModule

// Small uppercase caption above a section or field.
Text {
    color: Theme.textSecondary
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeSmall
    font.letterSpacing: 1
    font.capitalization: Font.AllUppercase
    elide: Text.ElideRight
}
