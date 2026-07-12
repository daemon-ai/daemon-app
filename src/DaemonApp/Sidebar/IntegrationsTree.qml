// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Pages

// [integrations wire v38] The integrations tree (work package A2): each configured account is a
// ROOT node whose protocol governs its subtree (Persons, Servers/Spaces -> rooms, standalone
// Channels, Direct Messages, Browse/Join). The structural logic lives in the shared C++
// IntegrationsTreeModel (composed from Transports + Persons); this is the thin GUI renderer — the
// TUI renders the very same model through a DisplayRoleAdapter.
Rectangle {
    id: root

    color: Theme.sidebar

    // A conversation node (room/channel/DM) was activated: open its native chat tab. The shell
    // routes this through the existing conversation-activation seam (A4 owns what activation does).
    signal conversationActivated(string transport, string conversation)

    function rowIcon(rowKind, convType) {
        switch (rowKind) {
        case "account": return FontIcons.fa_server;
        case "space": return FontIcons.fa_sitemap;
        case "person": return FontIcons.fa_user;
        case "action": return FontIcons.fa_globe;
        case "conversation":
            switch (convType) {
            case "channel": return FontIcons.fa_hashtag;
            case "dm": return FontIcons.fa_user;
            case "group_dm": return FontIcons.fa_users;
            default: return FontIcons.fa_comments;
            }
        default: return FontIcons.fa_comments;
        }
    }

    function connectionColor(connection) {
        switch (connection) {
        case "ready":
        case "connected": return Theme.stateRunning;
        case "": return Theme.transparent;
        default: return Theme.stateFinished;
        }
    }

    IntegrationsTreeModel {
        id: treeModel
        objectName: "integrationsTreeModel"
        mirror: (typeof Mirror !== "undefined") ? Mirror : null
        registry: (typeof Transports !== "undefined") ? Transports : null
    }

    AccountManagementController {
        id: accountCtrl
        objectName: "accountManagementController"
        registry: (typeof Transports !== "undefined") ? Transports : null
    }

    AccountFormDialog {
        id: accountForm
        controller: accountCtrl
    }

    // [integrations wire v38] The add-account handoff (A3): a fresh sign-in is interactive, so the
    // schema form's Continue does NOT configure directly — the controller emits authFlowRequested,
    // and we open the shared AuthFlowSheet to run the begin -> browser/challenge -> complete flow
    // (the same generic auth surface the Accounts/Channels pages bind). Edit/remove stay direct.
    AuthFlowSheet {
        id: authSheet
        objectName: "integrationsAuthSheet"
    }

    Connections {
        target: accountCtrl
        function onAuthFlowRequested(family, values) {
            authSheet.openFlow(family, values, "");
        }
    }

    Connections {
        target: treeModel
        function onConversationActivated(transport, conversation) {
            root.conversationActivated(transport, conversation);
        }
        function onEditAccountRequested(transport) {
            accountForm.openEdit(transport);
        }
        function onAddAccountRequested() {
            addMenu.popup();
        }
        function onRemoveAccountRequested(transport) {
            removeConfirm.transport = transport;
            removeConfirm.open();
        }
    }

    // The protocol picker for "+ Add integration": one entry per adapter family. Selecting one
    // seeds the schema form; on Continue the controller hands off to the auth flow (A3).
    Kit.Menu {
        id: addMenu
        Instantiator {
            model: accountCtrl.availableFamilies
            delegate: Kit.MenuItem {
                required property var modelData
                text: modelData.displayName
                onTriggered: accountForm.openAdd(modelData.family)
            }
            onObjectAdded: (index, object) => addMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => addMenu.removeItem(object)
        }
    }

    // Per-account context menu (right-click an account root).
    Kit.Menu {
        id: acctMenu
        property string transport: ""
        property bool acctEnabled: true

        function openFor(t, en) {
            acctMenu.transport = t;
            acctMenu.acctEnabled = en;
            popup();
        }

        Kit.MenuItem {
            text: qsTr("Connect")
            onTriggered: treeModel.connectAccount(acctMenu.transport)
        }
        Kit.MenuItem {
            text: acctMenu.acctEnabled ? qsTr("Disable") : qsTr("Enable")
            onTriggered: treeModel.setAccountEnabled(acctMenu.transport, !acctMenu.acctEnabled)
        }
        Kit.MenuSeparator {}
        Kit.MenuItem {
            text: qsTr("Account settings")
            onTriggered: accountForm.openEdit(acctMenu.transport)
        }
        Kit.MenuItem {
            text: qsTr("Remove account")
            onTriggered: {
                removeConfirm.transport = acctMenu.transport;
                removeConfirm.open();
            }
        }
    }

    Kit.Dialog {
        id: removeConfirm
        property string transport: ""
        title: qsTr("Remove account")
        acceptText: qsTr("Remove")
        destructive: true
        contentItem: QQC.Label {
            text: qsTr("Remove this account and disconnect it? The node tears it down fully.")
            wrapMode: Text.WordWrap
            font.family: FontIcons.display
            font.pixelSize: 13
            color: Theme.text
        }
        onAccepted: accountCtrl.removeAccount(removeConfirm.transport)
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // Header: "Integrations" + expand/collapse-all + "+ Add integration".
        Item {
            width: parent.width
            height: 32

            QQC.Label {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Integrations")
                color: Theme.separatorText
                font.family: FontIcons.display
                font.pixelSize: Theme.labelSize
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: Theme.labelTracking
            }

            Kit.IconButton {
                id: collapseBtn
                anchors.right: addBtn.left
                anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 30
                implicitHeight: 24
                icon: treeModel.anyExpanded ? FontIcons.fa_angles_up : FontIcons.fa_angles_down
                iconPointSize: 11
                iconColor: Theme.iconMuted
                tooltipText: treeModel.anyExpanded ? qsTr("Collapse all") : qsTr("Expand all")
                onClicked: treeModel.anyExpanded ? treeModel.collapseAll() : treeModel.expandAll()
            }

            Kit.IconButton {
                id: addBtn
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 30
                implicitHeight: 24
                icon: FontIcons.fa_plus
                iconPointSize: 12
                iconColor: Theme.addButton
                tooltipText: qsTr("Add integration")
                onClicked: addMenu.popup()
            }
        }

        QQC.ScrollView {
            width: parent.width
            height: parent.height - 32
            clip: true
            QQC.ScrollBar.vertical: Kit.ScrollBar {}

            ListView {
                id: list
                anchors.fill: parent
                model: treeModel
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: del
                    required property int index
                    required property string label
                    required property int depth
                    required property string rowKind
                    required property string convType
                    required property string section
                    required property string transport
                    required property string conversation
                    required property bool accountEnabled
                    required property string connection
                    required property string presence
                    required property int count
                    required property bool hasChildren
                    required property bool expanded
                    required property bool selectable
                    required property bool current

                    width: ListView.view.width
                    readonly property bool isSection: rowKind === "section"
                    readonly property bool isAccount: rowKind === "account"
                    height: isSection ? 26 : Theme.rowHeight

                    // Section header row (Persons / Channels / Direct Messages).
                    Item {
                        anchors.fill: parent
                        visible: del.isSection

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: treeModel.toggleExpand(del.index)
                        }
                        Kit.Glyph {
                            id: secChevron
                            anchors.left: parent.left
                            anchors.leftMargin: 14 + del.depth * Theme.treeIndent
                            anchors.verticalCenter: parent.verticalCenter
                            glyph: FontIcons.fa_chevron_right
                            font.pointSize: 8 + Theme.pointSizeOffset
                            rotation: del.expanded ? 90 : 0
                            color: Theme.separatorText
                        }
                        QQC.Label {
                            anchors.left: secChevron.right
                            anchors.leftMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            text: del.label
                            color: Theme.separatorText
                            font.family: FontIcons.display
                            font.pixelSize: Theme.labelSize
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: Theme.labelTracking
                        }
                    }

                    // Selectable rows (account / space / conversation / person / action).
                    Kit.TreeRow {
                        anchors.fill: parent
                        visible: !del.isSection
                        label: del.label
                        icon: root.rowIcon(del.rowKind, del.convType)
                        depth: del.depth
                        current: del.current
                        hovered: rowMouse.containsMouse
                        ignored: del.isAccount && !del.accountEnabled
                        hasChildren: del.hasChildren
                        expanded: del.expanded
                        trailingText: del.count >= 0 ? String(del.count) : ""
                        onTwistieClicked: treeModel.toggleExpand(del.index)

                        // Connection/presence dot on account roots.
                        Rectangle {
                            visible: del.isAccount && del.connection !== ""
                            width: 7
                            height: 7
                            radius: 3.5
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            color: root.connectionColor(del.connection)
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        enabled: del.selectable && !del.isSection
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                if (del.isAccount)
                                    acctMenu.openFor(del.transport, del.accountEnabled);
                                return;
                            }
                            treeModel.activate(del.index);
                        }
                        onDoubleClicked: if (del.hasChildren) treeModel.toggleExpand(del.index)
                    }
                }
            }
        }
    }
}
