import QtQuick
import DaemonApp.Theme

// A single FontAwesome (solid) glyph as text. Centralizes the icon font so the
// Solid face is always used and a font regression can't silently produce tofu.
// For non-FA glyphs, override `family` (e.g. FontIcons.mtSymbols).
Text {
    id: root

    property alias glyph: root.text
    property string family: FontIcons.faSolid

    font.family: family
    font.pointSize: 13 + Theme.pointSizeOffset
    renderType: Text.NativeRendering
    color: Theme.iconMuted
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
}
