// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme

// A small themed ComboBox: bordered, transparent
// fill, a custom caret, and an accent-highlighted current item in the popup.
QQC.ComboBox {
    id: root

    implicitHeight: 30
    font.family: FontIcons.display
    font.pixelSize: 11

    // A combo box's value (displayText) is not a name; when the dropdown has no
    // adjacent caption the call site sets this so a screen reader can announce
    // what the selection controls.
    property string accessibleName: ""
    Accessible.name: accessibleName

    // Disabled reads as muted + non-interactive (mirrors Kit.Switch), so a row
    // rendered read-only (e.g. a node-owned setting the app can't change) is
    // unambiguously not editable.
    opacity: root.enabled ? 1.0 : 0.4

    contentItem: Text {
        leftPadding: 6
        rightPadding: root.indicator.width
        text: root.displayText
        font: root.font
        color: Theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Text {
        x: root.width - width - 6
        y: (root.height - height) / 2
        text: FontIcons.fa_chevron_down
        font.family: FontIcons.faSolid
        font.pixelSize: 8
        color: Theme.text
    }

    background: Rectangle {
        radius: 3
        color: "transparent"
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Theme.accent : Theme.border
    }

    delegate: QQC.ItemDelegate {
        width: ListView.view.width
        required property int index
        required property var modelData

        contentItem: Text {
            text: modelData
            font.family: FontIcons.display
            font.pixelSize: 12
            color: index === root.currentIndex ? Theme.accent : Theme.text
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: parent.hovered ? Theme.hover : "transparent"
        }
    }

    popup: QQC.Popup {
        y: root.height + 2
        width: Math.max(root.width * 2.2, 120)
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.delegateModel
            currentIndex: root.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }
    }
}
