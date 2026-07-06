// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The checkpoint rewind confirm (E4/TOOL-9): rewinding is destructive — the node
// drops everything after the restore point — so both the timeline strip and the
// checkpoints popover route Restore through this dialog before calling
// Checkpoints.restore(id).
Kit.Dialog {
    id: root

    property string checkpointId: ""
    property string checkpointLabel: ""

    function confirmFor(id, label) {
        root.checkpointId = id;
        root.checkpointLabel = label || "";
        root.open();
    }

    title: qsTr("Rewind to this checkpoint?")
    acceptText: qsTr("Rewind")
    destructive: true

    onAccepted: {
        if (root.checkpointId.length > 0)
            Checkpoints.restore(root.checkpointId);
        root.checkpointId = "";
    }
    onRejected: root.checkpointId = ""

    contentItem: ColumnLayout {
        spacing: 6

        Text {
            visible: root.checkpointLabel.length > 0
            Layout.fillWidth: true
            Layout.maximumWidth: 320
            text: root.checkpointLabel
            font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            Layout.maximumWidth: 320
            text: qsTr("This drops the session's turns after the selected point.")
            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            wrapMode: Text.WordWrap
        }
    }
}
