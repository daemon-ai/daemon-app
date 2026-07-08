// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// [acct-mgmt] Read-only contact profile (ContactGetProfile → profileReady). The node renders the
// profile as a single string; the app shows it verbatim (never re-derives it). Opened when the
// Contacts seam fires profileReady for the contact the operator selected.
Kit.Dialog {
    id: dialog

    property string contactId: ""
    property string profile: ""

    title: qsTr("Contact profile")
    acceptText: qsTr("Close")
    showCancel: false

    function openFor(id, text) {
        dialog.contactId = id;
        dialog.profile = text;
        dialog.open();
    }

    contentItem: ColumnLayout {
        spacing: 6
        Text {
            Layout.fillWidth: true
            Layout.maximumWidth: 380
            text: dialog.contactId
            font.family: FontIcons.mono
            font.pixelSize: 11
            color: Theme.textMuted
            elide: Text.ElideMiddle
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
        Text {
            Layout.fillWidth: true
            Layout.maximumWidth: 380
            text: dialog.profile.length > 0 ? dialog.profile : qsTr("No profile details.")
            font.family: FontIcons.display
            font.pixelSize: 13
            color: Theme.text
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
        }
    }
}
