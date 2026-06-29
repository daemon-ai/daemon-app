// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Files

// The VS Code/Hermes-style file Explorer side panel: a themed header strip + the
// daemon-served FileTree, plus a Ctrl+P fuzzy finder overlay. Re-emits the tree's
// activation signals so the shell can open files as editor tabs. Single click ->
// fileActivated (preview tab); double click / finder Enter -> fileOpened (pinned).
// In selectDirs mode a directory activation emits folderChosen (folder attach).
Rectangle {
    id: root

    // When true, directories are selectable (folderChosen) instead of file-open.
    property bool selectDirs: false

    signal fileActivated(string rootId, string path)
    signal fileOpened(string rootId, string path)
    signal folderChosen(string rootId, string path)
    signal collapseRequested()

    function openFinder() { finder.visible = true; finder.focusInput(); }
    function openSearch() { search.visible = true; search.focusInput(); }

    color: Theme.sidebar

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header strip --------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 32
            color: Theme.sidebar

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 4
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Explorer")
                    color: Theme.textMuted
                    font.family: FontIcons.display
                    font.pixelSize: Theme.labelSize
                    font.weight: Font.DemiBold
                    font.letterSpacing: Theme.labelTracking
                    font.capitalization: Font.AllUppercase
                    elide: Text.ElideRight
                }

                Kit.IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: FontIcons.fa_rotate
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    backgroundRadius: width / 2
                    tooltipText: qsTr("Refresh")
                    onClicked: tree.refresh()
                }
                Kit.IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: FontIcons.fa_angles_up
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    backgroundRadius: width / 2
                    tooltipText: qsTr("Collapse all")
                    onClicked: tree.collapseAll()
                }
                Kit.IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: FontIcons.fa_file
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    backgroundRadius: width / 2
                    visible: !root.selectDirs
                    tooltipText: qsTr("Go to file (Ctrl+P)")
                    onClicked: root.openFinder()
                }
                Kit.IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: FontIcons.fa_magnifying_glass
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    backgroundRadius: width / 2
                    visible: !root.selectDirs
                    tooltipText: qsTr("Search in files")
                    onClicked: root.openSearch()
                }
                Kit.IconButton {
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: FontIcons.fa_xmark
                    iconColor: Theme.iconMuted
                    iconPointSize: 11
                    backgroundRadius: width / 2
                    tooltipText: qsTr("Hide explorer")
                    onClicked: root.collapseRequested()
                }
            }
        }

        Rectangle { Layout.fillWidth: true; implicitHeight: Theme.hairline; color: Theme.border }

        // --- Tree ----------------------------------------------------------
        FileTree {
            id: tree
            Layout.fillWidth: true
            Layout.fillHeight: true
            selectDirs: root.selectDirs
            onFileActivated: (rootId, path) => root.fileActivated(rootId, path)
            onFileOpened: (rootId, path) => root.fileOpened(rootId, path)
            onFolderChosen: (rootId, path) => root.folderChosen(rootId, path)
        }
    }

    // --- Ctrl+P finder overlay --------------------------------------------
    Rectangle {
        id: finder
        visible: false
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 6
        height: Math.min(parent.height - 12, 360)
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius

        function focusInput() { fileFinder.focusInput(); }

        FileFinder {
            id: fileFinder
            anchors.fill: parent
            anchors.margins: 4
            onFileChosen: (rootId, path) => {
                finder.visible = false;
                root.fileOpened(rootId, path);
            }
            onDismissed: finder.visible = false
        }
    }

    // --- Search-in-files overlay ------------------------------------------
    Rectangle {
        id: search
        visible: false
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 6
        height: Math.min(parent.height - 12, 420)
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius

        function focusInput() { searchPanel.focusInput(); }

        SearchResults {
            id: searchPanel
            anchors.fill: parent
            anchors.margins: 8
            // A hit opens the file (a pinned tab); line-jump on open is a follow-up.
            onHitChosen: (rootId, path, line, column) => {
                search.visible = false;
                root.fileOpened(rootId, path);
            }
            onDismissed: search.visible = false
        }
    }
}
