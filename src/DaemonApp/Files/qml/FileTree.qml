// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls

import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Files

// Daemon-served file tree: a flat, Sidebar-style tree over FsExplorerModel. The
// model owns expansion/selection/lazy loading; QML only renders the themed row
// and maps user activation to file/folder signals.
Item {
    id: root

    // Bound by the host to the `Fs` context property (the IFsService seam).
    property alias service: explorerModel.service
    property alias showIgnored: explorerModel.showIgnored
    // Folder-pick mode (composer "attach folder").
    property bool selectDirs: false

    // Single click on a file: transient (VS Code-style preview).
    signal fileActivated(string rootId, string path)
    // Double click on a file: pin the tab.
    signal fileOpened(string rootId, string path)
    // Double click on a directory while selectDirs is on: choose that folder.
    signal folderChosen(string rootId, string path)

    function refresh() {
        if (service)
            service.listRoots();
    }
    function collapseAll() {
        explorerModel.collapseAll();
    }

    FsExplorerModel {
        id: explorerModel
        service: Fs
        onFileActivated: (rootId, path) => {
            if (!root.selectDirs)
                root.fileActivated(rootId, path);
        }
        onDirectoryActivated: (rootId, path) => {
            if (root.selectDirs)
                root.folderChosen(rootId, path);
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        model: explorerModel
        boundsBehavior: Flickable.StopAtBounds
        keyNavigationEnabled: false

        ScrollBar.vertical: Kit.ScrollBar {}

        Keys.onUpPressed: currentIndex = Math.max(0, currentIndex - 1)
        Keys.onDownPressed: currentIndex = Math.min(count - 1, currentIndex + 1)
        Keys.onReturnPressed: explorerModel.activate(currentIndex)
        Keys.onEnterPressed: explorerModel.activate(currentIndex)
        Keys.onRightPressed: if (explorerModel.isDirAt(currentIndex)) explorerModel.toggleExpand(currentIndex)
        Keys.onLeftPressed: if (explorerModel.isDirAt(currentIndex)) explorerModel.toggleExpand(currentIndex)

        delegate: Item {
            id: del

            required property int index
            required property string label
            required property bool isDir
            required property bool ignored
            required property string rootId
            required property string path
            required property int depth
            required property bool hasChildren
            required property bool expanded
            required property bool current
            required property bool loading
            required property string error

            width: ListView.view.width
            height: Theme.rowHeight

            readonly property bool hovered: rowMouse.containsMouse

            Kit.TreeRow {
                anchors.fill: parent
                label: del.error !== "" ? del.error : del.label
                depth: del.depth
                current: del.current
                hovered: del.hovered
                ignored: del.ignored
                hasChildren: del.hasChildren
                expanded: del.expanded
                icon: del.loading ? FontIcons.fa_spinner : (del.isDir ? FontIcons.fa_folder : FontIcons.fa_file)
                trailingText: del.error !== "" ? qsTr("Retry") : ""
                onTwistieClicked: if (del.isDir) explorerModel.toggleExpand(del.index)
            }

            MouseArea {
                id: rowMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    list.forceActiveFocus();
                    list.currentIndex = del.index;
                    explorerModel.activate(del.index);
                }
                onDoubleClicked: {
                    list.forceActiveFocus();
                    list.currentIndex = del.index;
                    if (!del.isDir && !root.selectDirs)
                        root.fileOpened(del.rootId, del.path);
                    else
                        explorerModel.activate(del.index);
                }
            }
        }
    }
}
