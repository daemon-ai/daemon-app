import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A flat/filled text button themed from tokens. Set `accentFilled` for a primary
// (accent background) button, otherwise it is a subtle hover/pressed button.
QQC.Button {
    id: root

    property bool accentFilled: false

    font.family: FontIcons.display
    font.pixelSize: 14
    leftPadding: Theme.spacing
    rightPadding: Theme.spacing
    topPadding: Theme.spacingSmall
    bottomPadding: Theme.spacingSmall

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.accentFilled ? "white"
             : root.enabled ? Theme.text : Theme.textMuted
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radius
        color: {
            if (root.accentFilled)
                return root.enabled ? (root.down ? Qt.darker(Theme.accent, 1.15) : Theme.accent)
                                    : Theme.hover;
            return root.down ? Theme.pressed : root.hovered ? Theme.hover : "transparent";
        }
        border.width: root.accentFilled ? 0 : 1
        border.color: Theme.border
    }
}
