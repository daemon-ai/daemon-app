// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Files

// A modal picker over the daemon-served file tree (IFsService), used to attach a
// workspace file/folder by (rootId, path) rather than browsing the local disk
// with a native dialog (which would be wrong once files are daemon-served). It
// reuses the full FileExplorer (tree + Ctrl+P search) for consistency. Emits
// picked(rootId, path) on a file (or folder, in selectDirs mode) choice.
Popup {
    id: picker

    // Folder-pick mode (composer "attach folder").
    property bool selectDirs: false

    signal picked(string rootId, string path)

    modal: true
    focus: true
    width: 480
    height: 540
    anchors.centerIn: Overlay.overlay
    padding: 0

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius
    }

    contentItem: ColumnLayout {
        spacing: 0

        FileExplorer {
            id: explorer
            Layout.fillWidth: true
            Layout.fillHeight: true
            selectDirs: picker.selectDirs
            onFileActivated: (rootId, path) => { picker.picked(rootId, path); picker.close(); }
            onFileOpened: (rootId, path) => { picker.picked(rootId, path); picker.close(); }
            onFolderChosen: (rootId, path) => { picker.picked(rootId, path); picker.close(); }
            onCollapseRequested: picker.close()
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: Theme.hairline; color: Theme.border }

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.smallSpacing
            Item { Layout.fillWidth: true }
            Kit.TextButton {
                text: qsTr("Cancel")
                onClicked: picker.close()
            }
        }
    }
}
