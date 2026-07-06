// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.SessionsList

// Archived chats over the shared ISessionStore: a live list (scope =
// Archived) with unarchive / delete actions, mirroring the GUI's other
// store-backed views.
ColumnLayout {
    id: root
    spacing: 12

    SessionsListModel {
        id: archived
        store: SessionStore
        // NodeType::Archived == 1 (see domain/sidebar_node.h).
        Component.onCompleted: setScope(1, -1, "")
    }

    // F6: the TopLevel roster excludes archived rows, so entering this section fetches the
    // SessionScope::Archived listing (no-op on the in-memory store; the daemon store merges
    // additively and the model re-projects on changed()).
    Component.onCompleted: SessionStore.refreshArchivedSessions()

    RowLayout {
        Layout.fillWidth: true
        SectionLabel { text: qsTr("Archived chats"); Layout.fillWidth: true }
        Text {
            text: archived.count
            font.family: FontIcons.mono; font.pixelSize: 12; color: Theme.textMuted
        }
    }

    Text {
        visible: archived.count === 0
        text: qsTr("No archived chats.")
        font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
    }

    Repeater {
        model: archived
        delegate: Rectangle {
            id: rowDelegate
            required property string title
            required property int sessionId
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            radius: 6
            color: hover.hovered ? Theme.hover : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 8
                Text {
                    text: rowDelegate.title.length ? rowDelegate.title : qsTr("(untitled)")
                    font.family: FontIcons.display; font.pixelSize: 13; color: Theme.text
                    elide: Text.ElideRight; Layout.fillWidth: true
                }
                Kit.TextButton {
                    text: qsTr("Unarchive")
                    onClicked: SessionStore.setArchived(rowDelegate.sessionId, false)
                }
                Kit.IconButton {
                    icon: FontIcons.fa_trash; iconColor: Theme.danger; iconPointSize: 13
                    implicitWidth: 32; implicitHeight: 28
                    tooltipText: qsTr("Delete permanently")
                    onClicked: SessionStore.deleteSession(rowDelegate.sessionId)
                }
            }
            HoverHandler { id: hover }
        }
    }

    Item { Layout.fillHeight: true }
}
