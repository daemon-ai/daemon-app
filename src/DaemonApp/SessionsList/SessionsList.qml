// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Dialogs
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Presentation

// Middle column: the notes bar (collapse + title/count + trash + search button,
// with button-revealed search) and the NoteListView port - rows stacked as
// title / snippet / date / folder+tag chips note delegate.
Rectangle {
    id: root

    // Joins the sidebar chrome in dark/sepia/midnight; in Light it goes
    // off-white so it reads with the editor instead of as grey sidebar chrome.
    color: Theme.listBackground

    // Single-click / Enter: activate (opens in the transient preview tab).
    signal sessionActivated(string sessionId)
    // Double-click: open as a permanent (pinned) tab.
    signal sessionOpened(string sessionId)
    signal toggleSidebarRequested()

    property bool searchActive: false

    function setScope(nodeType, id, nodeId) {
        // The model owns selection now and clears it on scope change.
        convModel.setScope(nodeType, id, nodeId);
    }

    // Cosmetic icon only. The kind->icon category is decided once in the shared
    // C++ DisplayPresenter; QML maps the key to the concrete FontIcons glyph.
    function kindIcon(kind) {
        switch (DisplayPresenter.agentKindIconKey(kind)) {
        case "sitemap": return FontIcons.fa_sitemap;
        case "server": return FontIcons.fa_server;
        default: return FontIcons.fa_robot;
        }
    }

    function closeSearch() {
        convModel.search = "";
        root.searchActive = false;
    }

    SessionsListModel {
        id: convModel
        store: SessionStoreMirror
    }

    // --- Row actions (right-click): rename / pin / export / delete ----------
    Kit.Menu {
        id: rowMenu
        property string targetId: ""
        property bool targetPinned: false

        function openFor(sessionId, pinned) {
            rowMenu.targetId = sessionId;
            rowMenu.targetPinned = pinned;
            popup();
        }

        Kit.MenuItem {
            text: qsTr("Rename\u2026")
            onTriggered: renameDialog.openFor(rowMenu.targetId)
        }
        Kit.MenuItem {
            text: rowMenu.targetPinned ? qsTr("Unpin") : qsTr("Pin")
            onTriggered: SessionStoreMirror.setPinned(rowMenu.targetId, !rowMenu.targetPinned)
        }
        Kit.MenuItem {
            text: qsTr("Move up")
            onTriggered: SessionStoreMirror.moveSession(rowMenu.targetId, -1)
        }
        Kit.MenuItem {
            text: qsTr("Move down")
            onTriggered: SessionStoreMirror.moveSession(rowMenu.targetId, 1)
        }
        Kit.MenuItem {
            text: qsTr("Export\u2026")
            onTriggered: exportDialog.openFor(rowMenu.targetId)
        }
        Kit.MenuSeparator {}
        Kit.MenuItem {
            text: qsTr("Delete")
            onTriggered: deleteDialog.openFor(rowMenu.targetId)
        }
    }

    Kit.Dialog {
        id: renameDialog
        property string targetId: ""
        title: qsTr("Rename session")
        width: 380
        acceptText: qsTr("Rename")

        function openFor(sessionId) {
            renameDialog.targetId = sessionId;
            renameField.text = SessionStoreMirror.title(sessionId);
            open();
            renameField.forceActiveFocus();
            renameField.selectAll();
        }
        onAccepted: {
            if (renameDialog.targetId !== "" && renameField.text.trim().length > 0)
                SessionStoreMirror.renameSession(renameDialog.targetId, renameField.text.trim());
        }
        contentItem: Kit.TextField {
            id: renameField
            underline: true
            placeholderText: qsTr("Session title")
            onAccepted: renameDialog.accept()
        }
    }

    Kit.Dialog {
        id: deleteDialog
        property string targetId: ""
        title: qsTr("Delete session")
        width: 360
        acceptText: qsTr("Delete")
        destructive: true

        function openFor(sessionId) {
            deleteDialog.targetId = sessionId;
            open();
        }
        onAccepted: {
            if (deleteDialog.targetId !== "")
                SessionStoreMirror.deleteSession(deleteDialog.targetId);
        }
        contentItem: QQC.Label {
            text: qsTr("Permanently delete this session? This cannot be undone.")
            wrapMode: Text.WordWrap
            color: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 13
        }
    }

    FileDialog {
        id: exportDialog
        property string targetId: ""
        title: qsTr("Export transcript")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
        defaultSuffix: "json"

        function openFor(sessionId) {
            const t = SessionStoreMirror.title(sessionId);
            const base = (t && t.length > 0 ? t : "session");
            if (Qt.platform.os === "wasm") {
                // No shared filesystem with the node in the browser: skip the sandbox-only
                // FileDialog and hand the serialized JSON to the wasm bridge, which triggers a
                // real browser download of "<title-or-id>.json".
                Exporter.exportToBrowser(SessionStoreMirror, sessionId, base + ".json");
                return;
            }
            exportDialog.targetId = sessionId;
            exportDialog.currentFile = "file:" + base + ".json";
            open();
        }
        onAccepted: {
            if (exportDialog.targetId !== "")
                Exporter.writeFile(selectedFile, Exporter.toJson(SessionStoreMirror, exportDialog.targetId));
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Notes bar ------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingSmall
            Layout.rightMargin: Theme.spacingSmall
            Layout.topMargin: Theme.spacing
            Layout.bottomMargin: Theme.spacingSmall
            implicitHeight: 36

            // Default bar: collapse + title/count + trash + search button.
            RowLayout {
                anchors.fill: parent
                visible: !root.searchActive
                spacing: Theme.spacingSmall

                Kit.IconButton {
                    icon: FontIcons.fa_angles_left
                    tooltipText: qsTr("Collapse sidebar")
                    onClicked: root.toggleSidebarRequested()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    QQC.Label {
                        Layout.fillWidth: true
                        text: convModel.scopeTitle
                        color: Theme.listTitle
                        font.family: FontIcons.display
                        font.weight: Font.DemiBold
                        font.pixelSize: Theme.labelSize
                        font.letterSpacing: Theme.labelTracking
                        font.capitalization: Font.AllUppercase
                        elide: Text.ElideRight
                    }

                    QQC.Label {
                        text: qsTr("%n session(s)", "", convModel.count)
                        color: Theme.countText
                        font.family: FontIcons.display
                        font.pixelSize: 11
                    }
                }

                Kit.IconButton {
                    icon: FontIcons.fa_trash
                    tooltipText: qsTr("Trash")
                    onClicked: convModel.setScope(1, -1, "")
                }

                Kit.IconButton {
                    icon: FontIcons.fa_magnifying_glass
                    tooltipText: qsTr("Search")
                    onClicked: {
                        root.searchActive = true;
                        searchField.forceActiveFocus();
                    }
                }
            }

            // Search row: revealed field with leading magnifier + clear button.
            Item {
                anchors.fill: parent
                visible: root.searchActive

                Kit.TextField {
                    id: searchField
                    anchors.fill: parent
                    underline: true
                    placeholderText: qsTr("Search sessions")
                    leftPadding: 21
                    rightPadding: 30
                    text: convModel.search
                    onTextEdited: convModel.search = text
                    Keys.onEscapePressed: root.closeSearch()
                    onActiveFocusChanged: {
                        if (!activeFocus && text === "")
                            root.searchActive = false;
                    }
                }

                Kit.Glyph {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 7
                    glyph: FontIcons.fa_magnifying_glass
                    font.pointSize: 11 + Theme.pointSizeOffset
                    color: Theme.textMuted
                }

                Kit.IconButton {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    implicitWidth: 26
                    implicitHeight: 26
                    icon: FontIcons.fa_xmark
                    iconPointSize: 12
                    tooltipText: qsTr("Close search")
                    onClicked: root.closeSearch()
                }
            }
        }

        // --- List -----------------------------------------------------------
        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            QQC.ScrollBar.vertical: Kit.ScrollBar {}

            ListView {
                id: list
                model: convModel
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: del
                    required property int index
                    required property string title
                    required property string snippet
                    required property var modified
                    required property bool current
                    required property bool pinned

                    // LEFT_OFFSET_X = 20, TOP_OFFSET_Y = 10, LAST_EL_SEP_SPACE = 12.
                    readonly property int leftOffset: 20

                    width: ListView.view.width
                    height: content.implicitHeight + 10 + 12

                    readonly property bool isSelected: del.current

                    // Inset rounded selection: fill-only, no
                    // hairline - a deliberate step below the sidebar nav rows.
                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.rowInset
                        anchors.rightMargin: Theme.rowInset
                        anchors.topMargin: Theme.rowVInset
                        anchors.bottomMargin: Theme.rowVInset
                        radius: Theme.rowRadius
                        // Stable wash color + opacity fade (never interpolate out of
                        // transparent-black, which flashed dark on the way in).
                        color: del.isSelected ? Theme.listSelection : Theme.rowHover
                        opacity: del.isSelected || rowMouse.containsMouse ? 1 : 0

                        Behavior on opacity {
                            NumberAnimation { duration: Theme.motionFast }
                        }
                    }

                    ColumnLayout {
                        id: content
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: 10
                        anchors.leftMargin: del.leftOffset
                        anchors.rightMargin: del.leftOffset
                        spacing: 0

                        // Title - text-forward: secondary by default, brightens to
                        // primary on hover/selection. A leading pin glyph marks
                        // pinned rows (they float to the top of the scope).
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            Kit.Glyph {
                                visible: del.pinned
                                glyph: FontIcons.fa_thumbtack
                                font.pointSize: 9 + Theme.pointSizeOffset
                                color: Theme.accent
                                Layout.alignment: Qt.AlignVCenter
                            }
                            QQC.Label {
                                Layout.fillWidth: true
                                text: del.title
                                color: del.isSelected || rowMouse.containsMouse
                                     ? Theme.text : Theme.listText
                                font.family: FontIcons.display
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                        }

                        // Snippet (snippet above date).
                        QQC.Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 2
                            visible: del.snippet !== ""
                            text: del.snippet
                            color: Theme.listSnippet
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        // Date - on its own line, left-aligned (not top-right).
                        QQC.Label {
                            Layout.fillWidth: true
                            Layout.topMargin: 5
                            text: del.modified ? Qt.formatDateTime(del.modified, "MMM d, h:mm AP") : ""
                            color: Theme.countText
                            font.family: FontIcons.display
                            font.pixelSize: 11
                        }
                    }

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor

                        // Selectable session row named for its title.
                        Accessible.role: Accessible.ListItem
                        Accessible.name: del.title
                        Accessible.selected: del.isSelected
                        Accessible.onPressAction: {
                            convModel.activate(del.index);
                            root.sessionActivated(convModel.idAt(del.index));
                        }

                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                rowMenu.openFor(convModel.idAt(del.index), del.pinned);
                                return;
                            }
                            convModel.activate(del.index);
                            root.sessionActivated(convModel.idAt(del.index));
                        }
                        // Double-click promotes the preview into a pinned tab. The
                        // preceding single `clicked` already previewed it.
                        onDoubleClicked: root.sessionOpened(convModel.idAt(del.index))
                    }
                }
            }
        }
    }
}
