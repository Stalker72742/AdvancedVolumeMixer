pragma Singleton
import QtQuick

// Central style tokens. Everything else pulls colors/sizes from here
// so the whole app can be re-themed by editing one file.
QtObject {
    readonly property color bgPrimary: "#1a1b1e"
    readonly property color bgSecondary: "#222327"
    readonly property color bgElevated: "#2a2b30"
    readonly property color bgHover: "#323339"
    readonly property color bgPressed: "#3a3b42"

    readonly property color accent: "#5b8def"
    readonly property color accentHover: "#6f9bf2"
    readonly property color danger: "#e5534b"
    readonly property color success: "#4caf7d"
    readonly property color warning: "#e0a84a"
    readonly property color warningBg: "#2e2818"

    readonly property color textPrimary: "#e6e6e8"
    readonly property color textSecondary: "#9a9ba3"
    readonly property color textDisabled: "#5c5d64"

    readonly property color border: "#34353b"

    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12

    readonly property int titleBarHeight: 36
    readonly property int controlHeight: 36
    readonly property int spacingSmall: 6
    readonly property int spacingMedium: 12
    readonly property int spacingLarge: 20
    readonly property int pagePadding: 20

    readonly property string fontFamily: "Segoe UI"
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeNormal: 13
    readonly property int fontSizeMedium: 14
    readonly property int fontSizeLarge: 16

    // Segoe MDL2 Assets ships with Windows 10/11
    readonly property string iconFont: "Segoe MDL2 Assets"
    readonly property string iconMinimize: ""
    readonly property string iconClose: ""
    readonly property string iconCancel: ""
    readonly property string iconAdd: ""
    readonly property string iconEdit: ""
    readonly property string iconDelete: ""
    readonly property string iconAccept: ""
    readonly property string iconVolume: ""
    readonly property string iconMute: ""
    readonly property string iconSpeaker: ""
    readonly property string iconWarning: ""
}
