// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Profiles/agents manager: a master list of profiles on the left (with New) and
// the editor + Skills/tools curator on the right.
Item {
    id: root

    property string selectedId: Profiles.defaultProfileId
    // Transient export/import/revert feedback (PRO-7 / PRO-8).
    property string notice: ""

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

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: Profiles.profiles
                    spacing: 4
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        required property var entry
                        width: ListView.view.width
                        height: 46
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
                                Text {
                                    visible: entry.isDefault === true
                                    text: FontIcons.fa_star
                                    font.family: FontIcons.faSolid; font.pixelSize: 10
                                    color: Theme.accent
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
                            onClicked: root.selectedId = entry.id
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

    // New-profile dialog: a name/id plus an optional clone source (PRO-2). Empty clone source
    // creates from scratch; a chosen source clones that profile's spec (a copy, not a live link).
    QQC.Dialog {
        id: createDialog
        anchors.centerIn: parent
        modal: true
        title: qsTr("New profile")
        standardButtons: QQC.Dialog.Ok | QQC.Dialog.Cancel
        onAboutToShow: {
            nameField.text = "";
            cloneBox.currentIndex = 0;
        }
        onAccepted: {
            if (nameField.text.length === 0)
                return;
            var src = cloneBox.currentIndex > 0 ? Profiles.profileNames()[cloneBox.currentIndex - 1] : "";
            root.selectedId = src.length > 0 ? Profiles.cloneProfile(src, nameField.text)
                                             : Profiles.createProfile(nameField.text);
        }

        ColumnLayout {
            spacing: 10
            Kit.TextField {
                id: nameField
                Layout.fillWidth: true
                Layout.preferredWidth: 280
                placeholderText: qsTr("Profile id (e.g. work)")
            }
            Kit.Dropdown {
                id: cloneBox
                Layout.fillWidth: true
                model: [qsTr("Empty profile")].concat(Profiles.profileNames())
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
