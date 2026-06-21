import QtQuick

import DaemonApp.Theme

// Per-turn role header: a small avatar chip + the role name ("You" / "Daemon"),
// shown once at the first block of a user or assistant message. Replaces the old
// per-block user bubble as the role cue, leaving the message body as clean
// markdown. The user variant exposes a hover/touch edit affordance.
Item {
    id: root

    // "user" | "assistant".
    property string role: "user"
    readonly property bool isUser: role === "user"

    signal editRequested()

    implicitHeight: rowItem.implicitHeight

    HoverHandler { id: hover }

    Row {
        id: rowItem
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: Theme.smallSpacing

        Rectangle {
            id: avatar
            width: 20
            height: 20
            radius: 5
            anchors.verticalCenter: parent.verticalCenter
            color: root.isUser ? Theme.roleAvatarUser : Theme.roleAvatarAssistant

            Text {
                anchors.centerIn: parent
                text: root.isUser ? FontIcons.fa_user : FontIcons.fa_robot
                font.family: FontIcons.faSolid
                font.pixelSize: Theme.captionFontSize - 2
                color: root.isUser ? Theme.roleAvatarUserIcon : Theme.roleAvatarAssistantIcon
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.isUser ? qsTr("You") : qsTr("Daemon")
            color: Theme.roleName
            font.family: FontIcons.display
            font.pixelSize: Theme.labelSize
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.labelTracking
            font.capitalization: Font.AllUppercase
        }
    }

    // User-only edit affordance, right-aligned. Hover-revealed on desktop; always
    // visible on touch (no hover).
    Rectangle {
        id: editButton
        visible: root.isUser && (hover.hovered || Theme.touch)
        anchors.right: parent.right
        anchors.verticalCenter: rowItem.verticalCenter
        width: 22
        height: 22
        radius: Theme.radius
        color: editHover.hovered ? Theme.messageFooterHover : Theme.transparent

        Text {
            anchors.centerIn: parent
            text: FontIcons.fa_pen_to_square
            font.family: FontIcons.faSolid
            font.pixelSize: Theme.captionFontSize - 2
            color: Theme.messageFooterText
        }

        HoverHandler { id: editHover }
        TapHandler { onTapped: root.editRequested() }
    }
}
