// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Participants
import DaemonApp.Files

// The right side panel: the "Participants" section stacked above the file Explorer
// (mirroring how the left sidebar stacks its sections). Re-exposes the Explorer's
// activation/collapse signals so the shell can wire them exactly as before.
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

        // --- Participants section (sized to its content) -------------------
        Participants {
            id: participants
            Layout.fillWidth: true
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: Theme.hairline; color: Theme.border }

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
