// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [acct-mgmt] Directory people-picker (DirectorySearch → directoryResults). The query field drives
// Contacts.searchDirectory(transport, query); the node returns the matches (rendered verbatim), each
// offering Add (RosterAdd, gated on rosterOps.add) and DM (the conv-create seam with the contact as
// participant, gated on conversationOps.create). Node-first — the app never invents directory rows.
Kit.Dialog {
    id: dialog

    property string transport: ""
    property string accountLabel: ""
    property bool canAdd: false
    property bool canDm: false
    property var results: []

    // dotColor mirrors ChannelsPage (presence → status dot); passed in so the picker matches.
    property var dotColor: null

    title: accountLabel.length > 0 ? qsTr("Find people — %1").arg(accountLabel)
                                   : qsTr("Find people")
    acceptText: qsTr("Done")
    showCancel: false

    function openFor(t, label, addable, dmable) {
        dialog.transport = t;
        dialog.accountLabel = label;
        dialog.canAdd = addable;
        dialog.canDm = dmable;
        dialog.results = [];
        queryField.text = "";
        dialog.open();
        queryField.forceActiveFocus();
    }

    // Populate from the seam's directoryResults (the parent forwards it here).
    function setResults(list) { dialog.results = list; }

    function runSearch() {
        if (transport.length > 0)
            Contacts.searchDirectory(transport, queryField.text.trim());
    }

    contentItem: ColumnLayout {
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 360
            spacing: 6
            Kit.TextField {
                id: queryField
                Layout.fillWidth: true
                placeholderText: qsTr("Search the directory…")
                onAccepted: dialog.runSearch()
            }
            Kit.TextButton {
                text: qsTr("Search")
                onClicked: dialog.runSearch()
            }
        }

        Text {
            visible: dialog.results.length === 0
            text: qsTr("No results.")
            font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
        }

        QQC.ScrollView {
            visible: dialog.results.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, dialog.results.length * 34)
            clip: true
            ColumnLayout {
                width: dialog.width > 0 ? dialog.width - 48 : 360
                spacing: 3
                Repeater {
                    model: dialog.results
                    delegate: RowLayout {
                        id: personRow
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            text: FontIcons.fa_circle
                            font.family: FontIcons.faSolid; font.pixelSize: 8
                            color: dialog.dotColor
                                   ? dialog.dotColor(personRow.modelData.presence)
                                   : Theme.iconMuted
                        }
                        Text {
                            text: personRow.modelData.displayName !== undefined
                                  && personRow.modelData.displayName.length > 0
                                  ? personRow.modelData.displayName : personRow.modelData.id
                            font.family: FontIcons.display; font.pixelSize: 12
                            color: Theme.text; elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: personRow.modelData.id
                            font.family: FontIcons.mono; font.pixelSize: 10
                            color: Theme.textMuted; elide: Text.ElideMiddle
                            Layout.maximumWidth: 150
                        }
                        Kit.TextButton {
                            visible: dialog.canAdd
                            text: qsTr("Add")
                            onClicked: Contacts.addContact(dialog.transport,
                                                           personRow.modelData.id)
                        }
                        Kit.TextButton {
                            visible: dialog.canDm
                            text: qsTr("DM")
                            onClicked: Transports.createRoom(
                                dialog.transport,
                                { "participants": [personRow.modelData.id] })
                        }
                    }
                }
            }
        }
    }
}
