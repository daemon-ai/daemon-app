// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [acct-mgmt] Join-room dialog: renders the node-described ChannelJoinDetails form for one
// transport (fetched via Transports.conversationJoinDetails → joinDetailsReady). Node-first — the
// form is entirely data: the channel name is always shown; nickname/password rows appear only when
// the node reports *_supported; *_max_length constrains input; extras render through the shared
// SettingsSchemaForm. On accept it sends Transports.joinRoom(transport, form) — the node validates
// and the room list refreshes off the ConversationsChanged event (no optimistic insert).
Kit.Dialog {
    id: dialog

    property string transport: ""
    property string accountLabel: ""
    // The node-described form descriptor (joinDetailsReady payload).
    property var form: ({})

    title: accountLabel.length > 0 ? qsTr("Join room — %1").arg(accountLabel) : qsTr("Join room")
    acceptText: qsTr("Join")
    acceptEnabled: nameField.text.trim().length > 0 && schemaForm.complete

    onAccepted: {
        if (transport.length > 0) {
            Transports.joinRoom(transport, {
                "name": nameField.text.trim(),
                "nickname": nickField.text,
                "password": passField.text,
                "extras": schemaForm.values
            });
        }
    }

    function openFor(t, label, f) {
        dialog.transport = t;
        dialog.accountLabel = label;
        dialog.form = f || ({});
        nameField.text = "";
        nickField.text = "";
        passField.text = "";
        dialog.open();
    }

    contentItem: ColumnLayout {
        spacing: 8

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 320
            spacing: 3
            Text {
                text: qsTr("Channel")
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            }
            Kit.TextField {
                id: nameField
                Layout.fillWidth: true
                maximumLength: (dialog.form.nameMaxLength !== undefined
                                && dialog.form.nameMaxLength > 0) ? dialog.form.nameMaxLength : 32767
                placeholderText: qsTr("#room name")
            }
        }

        ColumnLayout {
            visible: dialog.form.nicknameSupported === true
            Layout.fillWidth: true
            spacing: 3
            Text {
                text: qsTr("Nickname")
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            }
            Kit.TextField {
                id: nickField
                Layout.fillWidth: true
                maximumLength: (dialog.form.nicknameMaxLength !== undefined
                                && dialog.form.nicknameMaxLength > 0)
                               ? dialog.form.nicknameMaxLength : 32767
            }
        }

        ColumnLayout {
            visible: dialog.form.passwordSupported === true
            Layout.fillWidth: true
            spacing: 3
            Text {
                text: qsTr("Password")
                font.family: FontIcons.display; font.pixelSize: 11; color: Theme.textMuted
            }
            Kit.TextField {
                id: passField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                maximumLength: (dialog.form.passwordMaxLength !== undefined
                                && dialog.form.passwordMaxLength > 0)
                               ? dialog.form.passwordMaxLength : 32767
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
