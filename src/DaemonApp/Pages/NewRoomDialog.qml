// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [acct-mgmt] New-room dialog: renders the node-described CreateConversationDetails form for one
// transport (fetched via Transports.conversationCreateDetails → createDetailsReady). Participants
// are create-time only (typed contact ids — the directory-backed picker lands with the contacts UI
// in Phase D); max members honors the node's hint (0 = unlimited); extras render through the shared
// SettingsSchemaForm. On accept it sends Transports.createRoom(transport, form); the room list
// refreshes off the ConversationsChanged event.
Kit.Dialog {
    id: dialog

    property string transport: ""
    property string accountLabel: ""
    property var form: ({})
    // Chosen participant contact ids (create-time only).
    property var participants: []

    title: accountLabel.length > 0 ? qsTr("New room — %1").arg(accountLabel) : qsTr("New room")
    acceptText: qsTr("Create")
    acceptEnabled: schemaForm.complete

    onAccepted: {
        if (transport.length > 0) {
            Transports.createRoom(transport, {
                "maxParticipants": maxSpin.value,
                "participants": dialog.participants,
                "extras": schemaForm.values
            });
        }
    }

    function openFor(t, label, f) {
        dialog.transport = t;
        dialog.accountLabel = label;
        dialog.form = f || ({});
        dialog.participants = [];
        maxSpin.value = (f && f.maxParticipants !== undefined) ? f.maxParticipants : 0;
        participantField.text = "";
        dialog.open();
    }

    function addParticipant() {
        var id = participantField.text.trim();
        if (id.length === 0)
            return;
        var next = dialog.participants.slice();
        if (next.indexOf(id) < 0)
            next.push(id);
        dialog.participants = next;
        participantField.text = "";
    }

    function removeParticipant(id) {
        dialog.participants = dialog.participants.filter(function (p) { return p !== id; });
    }

    contentItem: ColumnLayout {
        spacing: 8

        // --- Participants (create-time only) --------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 320
            spacing: 3
            Text {
                text: qsTr("Participants")
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            }
            Flow {
                Layout.fillWidth: true
                spacing: 4
                visible: dialog.participants.length > 0
                Repeater {
                    model: dialog.participants
                    delegate: Kit.Chip {
                        required property var modelData
                        text: modelData
                        closable: true
                        onCloseRequested: dialog.removeParticipant(modelData)
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Kit.TextField {
                    id: participantField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Contact id (e.g. @bob:matrix.org)")
                    onAccepted: dialog.addParticipant()
                }
                Kit.TextButton {
                    text: qsTr("Add")
                    enabled: participantField.text.trim().length > 0
                    onClicked: dialog.addParticipant()
                }
            }
        }

        // --- Max members ----------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: qsTr("Max members")
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            }
            Kit.SpinBox {
                id: maxSpin
                from: 0
                to: 100000
                value: 0
            }
            Text {
                text: qsTr("(0 = unlimited)")
                font.family: FontIcons.display; font.pixelSize: 10; color: Theme.textMuted
            }
        }

        Rectangle {
            visible: schemaForm.fields.length > 0
            Layout.fillWidth: true; height: 1; color: Theme.border
        }
        Kit.SettingsSchemaForm {
            id: schemaForm
            Layout.fillWidth: true
            fields: dialog.form.extras !== undefined ? dialog.form.extras : []
        }
    }
}
