// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Pages

// "+ New agent" (A7 + foreign engines): the Daemon-Kit-themed create-agent dialog, extracted from
// Sidebar.qml so the sidebar only instantiates it.
//
// Phase D: the bespoke Name + AgentTypePicker + AgentInferencePicker + foreign-copy layout is
// replaced by the SHARED AgentSetupForm over the ONE AgentSetupModel pipeline — Core and every
// foreign backend (AgentNative / NodeProvider) flow through the same form, and Create commits
// through AgentSetup.commit (never a bespoke create path). Success (committed) makes the new agent
// the default and opens its first chat, exactly as before; a failed provider probe / node
// rejection re-surfaces the dialog with the node's honest reason.
Kit.Dialog {
    id: root
    title: qsTr("New agent")
    width: 440
    acceptText: qsTr("Create")
    closePolicy: QQC.Popup.CloseOnEscape

    // Create is gated on the shared model's completeness (engine + backend + inference + name).
    acceptEnabled: setupForm.commitReady

    // C6/ENG-8: a create the node rejects (unknown/uninstalled agent, probe failure) arrives async
    // via AgentSetup.failed or Profiles.profileOpFailed. Re-surface the dialog with the honest
    // reason so the user can fix the selection instead of a silent close + stray toast.
    property string createError: ""
    property bool commitInFlight: false

    Connections {
        target: (typeof Profiles !== "undefined" && Profiles) ? Profiles : null
        function onProfileOpFailed(message) {
            if (root.commitInFlight) {
                root.commitInFlight = false;
                root.createError = message;
                root.open(); // re-surface (accept already closed it) with the failure named
            }
        }
    }

    function openForm() {
        root.createError = "";
        root.commitInFlight = false;
        setupForm.resetForm(); // fresh Core setup + fresh catalog/verification badges
        setupForm.focusName();
        open();
    }

    onAccepted: {
        if (!setupForm.commitReady)
            return;
        root.createError = "";
        root.commitInFlight = true;
        setupForm.submit(); // async: AgentSetup.commit -> committed(id)/failed(msg)
    }

    contentItem: ColumnLayout {
        spacing: 10

        // C6/ENG-8: the node's honest, agent-named rejection of a create.
        Rectangle {
            visible: root.createError.length > 0
            Layout.fillWidth: true
            radius: Theme.radius
            color: Theme.codeBackground
            border.width: 1
            border.color: Theme.danger
            implicitHeight: createErrorText.implicitHeight + 14
            Text {
                id: createErrorText
                anchors.fill: parent
                anchors.margins: 7
                text: root.createError
                color: Theme.danger
                font.family: FontIcons.display
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }

        AgentSetupForm {
            id: setupForm
            Layout.fillWidth: true

            onCommitted: function(profileId) {
                root.commitInFlight = false;
                if (profileId && profileId.length > 0 && typeof Profiles !== "undefined" && Profiles)
                    Profiles.setDefault(profileId);
                if (typeof App !== "undefined" && App)
                    App.openNewAgentChat();
            }
            onFailed: function(message) {
                root.commitInFlight = false;
                root.createError = message;
                root.open(); // re-surface with the actionable reason
            }
        }
    }
}
