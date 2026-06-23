import QtQuick
import DaemonApp.Theme

// An uppercase muted section subheader used inside settings/manager content.
Text {
    font.family: FontIcons.display
    font.pixelSize: Theme.labelSize
    font.weight: Font.DemiBold
    font.letterSpacing: Theme.labelTracking
    font.capitalization: Font.AllUppercase
    color: Theme.textMuted
}
