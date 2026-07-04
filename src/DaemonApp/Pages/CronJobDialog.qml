// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Create/edit a scheduled job. openFor("") creates a new job; openFor(id) loads
// the existing one. Writes back via ICronStore on Save.
Kit.Dialog {
    id: root

    property string jobId: ""

    title: jobId.length > 0 ? qsTr("Edit job") : qsTr("New job")
    width: 480
    footer: null

    function openFor(id) {
        jobId = id;
        const j = id.length > 0 ? Cron.job(id) : ({});
        nameField.text = j.name !== undefined ? j.name : "";
        schedField.text = j.schedule !== undefined ? j.schedule : "0 9 * * *";
        const wantProfile = j.profile !== undefined ? j.profile : "General Assistant";
        profileCombo.currentIndex = Math.max(0, Profiles.profileNames().indexOf(wantProfile));
        promptArea.text = j.prompt !== undefined ? j.prompt : "";
        open();
    }

    function save() {
        const profileName = profileCombo.currentText;
        if (jobId.length > 0) {
            Cron.updateJob(jobId, {
                "name": nameField.text,
                "schedule": schedField.text,
                "profile": profileName,
                "prompt": promptArea.text,
            });
        } else {
            const id = Cron.createJob(nameField.text, schedField.text, profileName);
            Cron.updateJob(id, { "prompt": promptArea.text });
        }
        close();
    }

    contentItem: ColumnLayout {
        spacing: 12

        SectionLabel { text: qsTr("Name") }
        Kit.TextField { id: nameField; Layout.fillWidth: true; placeholderText: qsTr("Job name") }

        SectionLabel { text: qsTr("Schedule (cron)") }
        Kit.TextField {
            id: schedField; Layout.fillWidth: true
            placeholderText: qsTr("m h dom mon dow  (e.g. 0 8 * * *)")
        }

        SectionLabel { text: qsTr("Profile") }
        // Profile is chosen from the existing profiles, not free text.
        Kit.Dropdown {
            id: profileCombo
            Layout.fillWidth: true
            model: Profiles.profileNames()
        }

        SectionLabel { text: qsTr("Prompt") }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 90
            radius: Theme.radius
            color: "transparent"
            border.color: Theme.border
            QQC.ScrollView {
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                Kit.TextArea {
                    id: promptArea
                    font.pixelSize: 13
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            Item { Layout.fillWidth: true }
            Kit.TextButton { text: qsTr("Cancel"); onClicked: root.reject() }
            Kit.TextButton {
                text: qsTr("Save"); accentFilled: true
                enabled: nameField.text.length > 0 && schedField.text.length > 0
                onClicked: root.save()
            }
        }
    }
}
