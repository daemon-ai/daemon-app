// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Profiles/agents manager: a master list of profiles on the left (with New) and
// the editor + Skills/tools curator on the right.
Item {
    id: root
    objectName: "profilesPage"

    property string selectedId: Profiles.defaultProfileId
    // Transient export/import/revert feedback (PRO-7 / PRO-8).
    property string notice: ""
    // Provenance filter (phase H): "all" | "operator" | "agent". Filters the list rows by the
    // node-stamped `createdByIsAgent` provenance (agent-authored via profile_manage vs operator).
    // Presentation-only selection state; the node owns the profile set.
    property string provenanceFilter: "all"

    // Surface profile export/import/revert results as a transient toast; jump to a freshly
    // imported profile.
    Connections {
        target: Profiles
        function onExportSaved(fileUrl) { root.notice = qsTr("Exported profile"); noticeClear.restart(); }
        function onImported(newId) {
            root.selectedId = newId;
            root.notice = qsTr("Imported %1").arg(newId);
            noticeClear.restart();
        }
        function onReverted(id) { root.notice = qsTr("Reverted %1").arg(id); noticeClear.restart(); }
        function onProfileOpFailed(message) { root.notice = message; noticeClear.restart(); }
    }
    Timer { id: noticeClear; interval: 5000; onTriggered: root.notice = "" }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Profiles")
        icon: FontIcons.fa_users
    }

    RowLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        // --- Master list ---------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 240
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Kit.TextButton {
                    Layout.fillWidth: true
                    text: qsTr("+ New profile")
                    accentFilled: true
                    onClicked: createDialog.open()
                }

                // Provenance filter (phase H): scope the list to All / Operator / Agent-authored.
                // The three indices map to root.provenanceFilter values; the delegate collapses rows
                // that don't match (no re-modeling — the node still owns Profiles.profiles).
                Kit.Dropdown {
                    id: provenanceBox
                    Layout.fillWidth: true
                    accessibleName: qsTr("Filter profiles")
                    model: [qsTr("All profiles"), qsTr("Operator"), qsTr("Agent-authored")]
                    onCurrentIndexChanged: root.provenanceFilter =
                        currentIndex === 1 ? "operator" : currentIndex === 2 ? "agent" : "all"
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: Profiles.profiles
                    spacing: 4
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        id: profileRow
                        required property var entry
                        // Provenance (phase H), consumed verbatim from the node-stamped row keys.
                        readonly property bool agentAuthored: entry.createdByIsAgent === true
                        readonly property string ownerSession: entry.owner || ""
                        // Filter-collapse: hide rows that don't match the provenance filter without
                        // re-modeling the node-owned list.
                        readonly property bool matchesFilter:
                            root.provenanceFilter === "all"
                            || (root.provenanceFilter === "agent" && agentAuthored)
                            || (root.provenanceFilter === "operator" && !agentAuthored)
                        width: ListView.view.width
                        height: matchesFilter ? 46 : 0
                        visible: matchesFilter
                        radius: 6
                        color: entry.id === root.selectedId ? Theme.hover : "transparent"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 1
                            RowLayout {
                                spacing: 6
                                Text {
                                    text: entry.name
                                    font.family: FontIcons.display; font.pixelSize: 13
                                    color: Theme.text; elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                // Engine identity (C3/ENG-7): Native core vs the foreign agent
                                // this profile is bound to.
                                Kit.Chip {
                                    // "Foreign" is the wire engineKind value, not display text.
                                    readonly property bool foreign: entry.engine === "Foreign"
                                    visible: entry.engine !== undefined
                                    text: foreign ? (entry.acpAgent || qsTr("Foreign"))
                                                  : qsTr("Native")
                                    iconGlyph: foreign ? FontIcons.fa_robot : FontIcons.fa_microchip
                                    tone: foreign ? "accent" : "muted"
                                }
                                // Provenance badge (phase H): agent-authored profiles (created via
                                // the profile_manage tool) carry the owning session id. Rendered
                                // verbatim; tooltip surfaces the owner.
                                Kit.Chip {
                                    visible: profileRow.agentAuthored
                                    text: qsTr("agent-authored")
                                    iconGlyph: FontIcons.fa_robot
                                    tone: "accent"
                                    tooltipText: profileRow.ownerSession.length > 0
                                        ? qsTr("Owner: %1").arg(profileRow.ownerSession)
                                        : qsTr("Authored by an agent")
                                }
                                Text {
                                    visible: entry.isDefault === true
                                    text: FontIcons.fa_star
                                    font.family: FontIcons.faSolid; font.pixelSize: 10
                                    color: Theme.accent
                                }
                                // Operator revoke (phase H): agent-authored profiles only. Reuses the
                                // existing ProfileDelete intent (Profiles.remove) behind a confirm.
                                Kit.IconButton {
                                    visible: profileRow.agentAuthored
                                    icon: FontIcons.fa_trash
                                    iconPointSize: 10
                                    implicitWidth: 28; implicitHeight: 24
                                    tooltipText: qsTr("Revoke this agent-authored profile")
                                    onClicked: {
                                        revokeDialog.targetId = entry.id;
                                        revokeDialog.targetName = entry.name;
                                        revokeDialog.open();
                                    }
                                }
                            }
                            Text {
                                text: entry.model
                                font.family: FontIcons.mono; font.pixelSize: 10
                                color: Theme.textMuted; elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            // Keep the revoke button clickable: the row selects on the body only.
                            z: -1
                            onClicked: root.selectedId = entry.id

                            // Selectable profile row named for the profile.
                            Accessible.role: Accessible.ListItem
                            Accessible.name: entry.name
                            Accessible.selected: root.selectedId === entry.id
                            Accessible.onPressAction: root.selectedId = entry.id
                        }
                    }
                }
            }
        }

        // --- Editor --------------------------------------------------------
        ProfileEditor {
            Layout.fillWidth: true
            Layout.fillHeight: true
            profileId: root.selectedId
            onDeleted: root.selectedId = Profiles.defaultProfileId
        }
    }

    // New-profile dialog (Phase D): the SHARED AgentSetupForm over the ONE AgentSetupModel
    // pipeline (engine + backend + inference), so a Core or foreign profile is created from the
    // same surface as the "+ New agent" dialog and the wizard. An optional clone source (PRO-2) is
    // retained as an alternative: choosing a source copies that profile's spec (a copy, not a live
    // link) under the entered name and ignores the setup selection; "Empty profile" configures a
    // fresh profile through the form's AgentSetup.commit.
    Kit.Dialog {
        id: createDialog
        width: 460
        title: qsTr("New profile")
        acceptText: qsTr("Create")
        readonly property bool cloning: cloneBox.currentIndex > 0
        acceptEnabled: setupForm.nameText.trim().length > 0
                       && (cloning || setupForm.commitReady)
        onAboutToShow: {
            setupForm.resetForm();
            cloneBox.currentIndex = 0;
            setupForm.focusName();
        }
        onAccepted: {
            const name = setupForm.nameText.trim();
            if (name.length === 0)
                return;
            if (createDialog.cloning) {
                var src = Profiles.profileNames()[cloneBox.currentIndex - 1];
                root.selectedId = Profiles.cloneProfile(src, name);
            } else {
                setupForm.submit(); // async: committed(id) selects the new profile
            }
        }

        contentItem: ColumnLayout {
            spacing: 10
            AgentSetupForm {
                id: setupForm
                Layout.fillWidth: true
                // A fresh, configured profile becomes the selection once the node reflects it.
                onCommitted: function(profileId) {
                    if (profileId && profileId.length > 0)
                        root.selectedId = profileId;
                }
                onFailed: function(message) {
                    root.notice = message;
                    noticeClear.restart();
                    createDialog.open(); // re-surface with the reason
                }
            }
            SectionLabel { text: qsTr("Or clone from") }
            Kit.Dropdown {
                id: cloneBox
                Layout.fillWidth: true
                model: [qsTr("Empty profile")].concat(Profiles.profileNames())
            }
        }
    }

    // Operator revoke confirm (phase H): agent-authored profiles are removed with the existing
    // ProfileDelete intent (Profiles.remove). Kept separate from the sibling-owned create dialog.
    Kit.Dialog {
        id: revokeDialog
        property string targetId: ""
        property string targetName: ""
        title: qsTr("Revoke profile")
        acceptText: qsTr("Revoke")
        destructive: true
        onAccepted: {
            if (revokeDialog.targetId.length > 0)
                Profiles.remove(revokeDialog.targetId);
        }

        contentItem: ColumnLayout {
            Text {
                Layout.preferredWidth: 320
                Layout.fillWidth: true
                text: qsTr("Revoke the agent-authored profile \u201c%1\u201d? This deletes it from the node.")
                        .arg(revokeDialog.targetName)
                color: Theme.text
                font.family: FontIcons.display
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
        }
    }

    // Transient feedback toast (export saved / imported / reverted / failures).
    Rectangle {
        visible: root.notice !== ""
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 16
        radius: 8
        color: Theme.surface
        border.color: Theme.border
        implicitWidth: noticeText.implicitWidth + 24
        implicitHeight: noticeText.implicitHeight + 16
        Text {
            id: noticeText
            anchors.centerIn: parent
            text: root.notice
            color: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 12
        }
    }
}
