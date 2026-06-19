pragma Singleton
import QtQuick

QtObject {
    // Light palette inspired by the source app; a dark variant can switch these.
    readonly property color background: "#ffffff"
    readonly property color surface: "#f7f7f5"
    readonly property color sidebar: "#fbfbfa"
    readonly property color border: "#e6e6e3"
    readonly property color text: "#2f3437"
    readonly property color textMuted: "#8a8f98"
    readonly property color accent: "#2383e2"
    readonly property color selection: "#eaf2fb"

    readonly property int spacingSmall: 6
    readonly property int spacing: 12
    readonly property int spacingLarge: 20
    readonly property int radius: 8

    readonly property int sidebarWidth: 240
    readonly property int listWidth: 300
}
