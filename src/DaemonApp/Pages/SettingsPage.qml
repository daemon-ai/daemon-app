// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// The app-level Settings page: a left section rail + a scrollable right content
// pane (the hermes OverlaySplitLayout shape). Sections are deep-linkable via the
// `section` property (set from Nav.section). Each section is its own component;
// not-yet-wired ones fall back to a Placeholder. The per-transcript appearance
// "..." popup is a separate, untouched surface.
Item {
    id: root

    // Deep-link target (a section id); defaults to Appearance.
    property string section: "appearance"

    readonly property var sections: [
        { id: "appearance",   label: qsTr("Appearance"),     icon: FontIcons.fa_sliders },
        { id: "notifications", label: qsTr("Notifications"),  icon: FontIcons.fa_bell },
        { id: "connection",   label: qsTr("Connection"),     icon: FontIcons.fa_link },
        { id: "model",        label: qsTr("Model"),          icon: FontIcons.fa_brain },
        { id: "chat",         label: qsTr("Chat"),           icon: FontIcons.fa_comments },
        { id: "safety",       label: qsTr("Safety"),         icon: FontIcons.fa_circle_exclamation },
        { id: "memory",       label: qsTr("Memory & Context"), icon: FontIcons.fa_brain },
        { id: "workspace",    label: qsTr("Workspace"),      icon: FontIcons.fa_list_ul },
        { id: "voice",        label: qsTr("Voice"),          icon: FontIcons.fa_circle_info },
        { id: "advanced",     label: qsTr("Advanced"),       icon: FontIcons.fa_gear },
        { id: "archived",     label: qsTr("Archived chats"), icon: FontIcons.fa_box_archive },
        { id: "about",        label: qsTr("About"),          icon: FontIcons.fa_circle_info },
    ]

    function _current() {
        return root.section && root.section.length > 0 ? root.section : "appearance";
    }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Settings")
        icon: FontIcons.fa_gear
    }

    RowLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        NavRail {
            Layout.fillHeight: true
            items: root.sections
            current: root._current()
            onSelected: function(id) { root.section = id; }
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            Loader {
                id: content
                width: parent.width
                // The content components are laid out with a margin via this wrapper.
                sourceComponent: {
                    switch (root._current()) {
                    case "appearance": return appearanceComp;
                    case "notifications": return notificationsComp;
                    case "connection": return connectionComp;
                    case "model": return modelComp;
                    case "chat": return chatComp;
                    case "safety": return safetyComp;
                    case "memory": return memoryComp;
                    case "workspace": return workspaceComp;
                    case "voice": return voiceComp;
                    case "advanced": return advancedComp;
                    case "archived": return archivedComp;
                    case "about": return aboutComp;
                    default: return placeholderComp;
                    }
                }
            }
        }
    }

    // Each section wrapped with consistent padding.
    component Padded: Item {
        default property alias body: inner.data
        implicitHeight: inner.implicitHeight + 40
        Item {
            id: inner
            x: 24; y: 20
            width: parent.width - 48
            implicitHeight: childrenRect.height
        }
    }

    Component { id: appearanceComp;    Padded { AppearanceSection { width: parent.width } } }
    Component { id: notificationsComp; Padded { NotificationsSection { width: parent.width } } }
    Component { id: connectionComp;    Padded { ConnectionSection { width: parent.width } } }
    Component { id: modelComp;         Padded { ModelSettingsSection { width: parent.width } } }
    Component { id: chatComp;          Padded { ChatSettingsSection { width: parent.width } } }
    Component { id: safetyComp;        Padded { SafetySettingsSection { width: parent.width } } }
    Component { id: memoryComp;        Padded { MemorySettingsSection { width: parent.width } } }
    Component { id: workspaceComp;     Padded { WorkspaceSection { width: parent.width } } }
    Component { id: voiceComp;         Padded { VoiceSection { width: parent.width } } }
    Component { id: advancedComp;      Padded { AdvancedSection { width: parent.width } } }
    Component { id: archivedComp;      Padded { ArchivedSection { width: parent.width } } }
    Component { id: aboutComp;         Padded { AboutSection { width: parent.width } } }
    Component { id: placeholderComp;   Placeholder { label: qsTr("This section is not wired yet") } }
}
