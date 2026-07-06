// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// The Models hub: a left rail (Discover / Downloads / Installed / Providers) over
// the shared ModelCatalog facade, mirroring the Settings page shape. Sections are
// deep-linkable via `section` (Nav.section).
Item {
    id: root

    property string section: "discover"

    readonly property var sections: [
        { id: "discover",  label: qsTr("Discover"),  icon: FontIcons.fa_magnifying_glass },
        { id: "downloads", label: qsTr("Downloads"), icon: FontIcons.fa_download },
        { id: "installed", label: qsTr("Installed"), icon: FontIcons.fa_box_archive },
        { id: "providers", label: qsTr("Providers"), icon: FontIcons.fa_link },
    ]

    function _current() {
        return root.section && root.section.length > 0 ? root.section : "discover";
    }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Models")
        icon: FontIcons.fa_brain
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

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: {
                switch (root._current()) {
                case "discover": return discoverComp;
                case "downloads": return downloadsComp;
                case "installed": return installedComp;
                case "providers": return providersComp;
                default: return discoverComp;
                }
            }
        }
    }

    Component { id: discoverComp;  ModelsDiscover {} }
    Component { id: downloadsComp; ModelsDownloads {} }
    Component {
        id: installedComp
        // A5 (CON-13): "Re-quantize…" on an installed row opens the shared quant picker in
        // requantize mode (its actions drive ModelQuantize instead of downloads).
        ModelsInstalled {
            onRequantizeRequested: function(repo, sourceFile) {
                requantPicker.openForRequantize(repo, sourceFile);
            }
        }
    }
    Component { id: providersComp; ModelsProviders {} }

    QuantPickerPopup { id: requantPicker }
}
