// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Files

// The right side panel: the file Explorer. (The permanently-empty Participants section died
// with the legacy store members — AD 1a.4.) Re-exposes the Explorer's activation/collapse
// signals so the shell can wire them exactly as before.
Rectangle {
    id: root

    property bool selectDirs: false

    signal fileActivated(string rootId, string path)
    signal fileOpened(string rootId, string path)
    signal folderChosen(string rootId, string path)
    signal collapseRequested()

    function openFinder() { explorer.openFinder(); }

    color: Theme.sidebar

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- File Explorer (fills the rest) -------------------------------
        FileExplorer {
            id: explorer
            Layout.fillWidth: true
            Layout.fillHeight: true
            selectDirs: root.selectDirs
            onFileActivated: (rootId, path) => root.fileActivated(rootId, path)
            onFileOpened: (rootId, path) => root.fileOpened(rootId, path)
            onFolderChosen: (rootId, path) => root.folderChosen(rootId, path)
            onCollapseRequested: root.collapseRequested()
        }
    }
}
