// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Pages

// Per-agent Profile tab: renders an agent's ProfileSpec (agent == profile). Bound
// to a ProfileRef via the `profile` property (set from the tab's profile). Reads
// the reshaped profiles seam (daemon-aligned fields: provider/model/baseUrl/
// toolAllowlist/memoryProvider/contextEngine/credentialRef; the persona comes
// from the store's soul()/requestSoul() persona seam, not the spec row). Display
// surface for now; the existing Profiles manager page remains the editor.
Item {
    id: root

    // The agent identity (ProfileRef) this tab is bound to.
    property string profile: ""

    readonly property var spec: (typeof Profiles !== "undefined" && Profiles && profile.length > 0)
        ? Profiles.profile(profile)
        : ({})

    // Persona (SOUL.md), read-only: seeded from the seam's last-known value, refreshed
    // when a (possibly async) requestSoul answer lands via onSoulChanged.
    property string personaText: ""
    readonly property bool foreignEngine: root.field("engine", "Core") === "Foreign"

    function refreshPersona() {
        if (typeof Profiles === "undefined" || !Profiles || profile.length === 0) {
            personaText = "";
            return;
        }
        personaText = Profiles.soul(profile);
        Profiles.requestSoul(profile);
    }

    onProfileChanged: refreshPersona()
    Component.onCompleted: refreshPersona()

    Connections {
        target: (typeof Profiles !== "undefined" && Profiles) ? Profiles : null
        function onSoulChanged(id) {
            if (id === root.profile)
                root.personaText = Profiles.soul(id);
        }
    }

    function field(key, fallback) {
        return (root.spec && root.spec[key] !== undefined && String(root.spec[key]).length > 0)
            ? root.spec[key]
            : (fallback === undefined ? "" : fallback);
    }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Profile - %1").arg(root.field("name", root.profile))
        icon: FontIcons.fa_robot
    }

    component KV: RowLayout {
        property string k: ""
        property string v: ""
        property color tone: Theme.text
        Layout.fillWidth: true
        spacing: 10
        Text {
            text: k
            font.family: FontIcons.display
            font.pixelSize: 12
            color: Theme.textMuted
            Layout.preferredWidth: 130
        }
        Text {
            text: v
            font.family: FontIcons.mono
            font.pixelSize: 12
            color: tone
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    component Section: Text {
        font.family: FontIcons.display
        font.pixelSize: 12
        font.weight: Font.DemiBold
        color: Theme.accent
        topPadding: 10
    }

    component Chips: Flow {
        property var items: []
        Layout.fillWidth: true
        spacing: 6
        Repeater {
            model: parent.items
            delegate: Rectangle {
                required property string modelData
                radius: 4
                implicitWidth: chipText.implicitWidth + 14
                implicitHeight: 20
                color: Theme.hover
                Text {
                    id: chipText
                    anchors.centerIn: parent
                    text: modelData
                    font.family: FontIcons.mono
                    font.pixelSize: 10
                    color: Theme.text
                }
            }
        }
    }

    QQC.ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        contentWidth: availableWidth
        QQC.ScrollBar.vertical: Kit.ScrollBar {}

        ColumnLayout {
            width: parent.width
            spacing: 6

            Text {
                visible: root.profile.length === 0
                text: qsTr("No agent selected.")
                font.family: FontIcons.display
                font.pixelSize: 13
                color: Theme.textMuted
            }

            Section { text: qsTr("Identity"); visible: root.profile.length > 0 }
            KV { visible: root.profile.length > 0; k: qsTr("profile id"); v: root.profile }
            KV { visible: root.profile.length > 0; k: qsTr("name"); v: root.field("name", root.profile) }
            KV {
                visible: root.profile.length > 0
                k: qsTr("description")
                v: root.field("description", "-")
            }

            Section { text: qsTr("Engine"); visible: root.profile.length > 0 }
            KV { visible: root.profile.length > 0; k: qsTr("provider"); v: root.field("provider", "-") }
            KV { visible: root.profile.length > 0; k: qsTr("model"); v: root.field("model", "-") }
            KV {
                visible: root.profile.length > 0
                k: qsTr("base url")
                v: root.field("baseUrl", qsTr("(provider default)"))
            }
            KV {
                visible: root.profile.length > 0
                k: qsTr("context engine")
                v: root.field("contextEngine", "lcm")
            }

            Section { text: qsTr("Memory"); visible: root.profile.length > 0 }
            KV {
                visible: root.profile.length > 0
                k: qsTr("memory provider")
                v: root.field("memoryProvider", "mnemosyne")
                tone: Theme.accent
            }

            // Persona is Core-engine only: a foreign agent owns its own prompt, so the
            // section is hidden for foreign profiles.
            Section {
                text: qsTr("Persona")
                visible: root.profile.length > 0 && !root.foreignEngine
            }
            Text {
                visible: root.profile.length > 0 && !root.foreignEngine
                text: root.personaText.length > 0 ? root.personaText : "-"
                font.family: FontIcons.display
                font.pixelSize: 12
                color: Theme.text
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Section { text: qsTr("Tool allowlist"); visible: root.profile.length > 0 }
            Chips {
                visible: root.profile.length > 0
                items: root.spec && root.spec.toolAllowlist !== undefined
                    ? root.spec.toolAllowlist : []
            }

            Section { text: qsTr("Skills"); visible: root.profile.length > 0 }
            Chips {
                visible: root.profile.length > 0
                items: root.spec && root.spec.skills !== undefined ? root.spec.skills : []
            }

            Section { text: qsTr("Credentials"); visible: root.profile.length > 0 }
            KV {
                visible: root.profile.length > 0
                k: qsTr("credential ref")
                v: root.field("credentialRef", root.profile + qsTr(" (default)"))
            }
        }
    }
}
