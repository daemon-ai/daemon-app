import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Dialogs
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Tabs
import DaemonApp.Pages

// The conversation pane host, built as two stacked containers with EXPLICIT
// anchored geometry (no outer ColumnLayout): a fixed-height tab bar at the top
// and the transcript body filling the rest. The previous nested-Layout shell let
// the tab strip's height balloon and swallow the transcript; anchoring the bar to
// a hard height makes that impossible. Each transcript tab is its own
// TranscriptPage (own controller + orchestrator, so background/pinned tabs keep
// streaming + scroll state) kept alive in a Repeater and shown/hidden by the
// shared TabModel's currentIndex. The settings "..." popup drops from the bar.
Rectangle {
    id: root

    color: Theme.background

    // The fixed height of the top tab bar. A constant so the bar can never grow
    // into the transcript region.
    readonly property int barHeight: 36

    // Compact (phone) only: pop back to the conversation list.
    signal backRequested()

    // A page forwarded a window-level command (help / title / save); the shell
    // (Main.qml) routes it.
    signal paneCommandForwarded(string command)

    // A throwaway controller used solely to create new conversations in the
    // shared store; each TranscriptPage owns its own controller for display.
    ConversationController {
        id: creator
        store: ConversationStore
    }

    TabModel {
        id: tabModel
    }

    // The active tab's TranscriptPage (null when no transcript tab is active), so
    // the settings popup's "Move to Trash" can act on the open conversation.
    readonly property Item activePage: {
        const ld = tabRepeater.itemAt(tabModel.currentIndex);
        return (ld && ld.item) ? ld.item : null;
    }

    // --- Public API used by the shell (Main.qml) ----------------------------
    // The canonical conversation title (the same string the list shows), with a
    // generic fallback so a chip is never blank.
    function _titleFor(conversationId) {
        const t = ConversationStore.title(conversationId);
        return (t && t.length > 0) ? t : qsTr("Conversation");
    }
    // Single-click / type-ahead open: load the conversation into the transient
    // preview tab (reused on the next preview), VSCode-style.
    function open(conversationId) {
        openConversation(conversationId);
    }
    function openConversation(conversationId) {
        tabModel.previewTranscript(conversationId, _titleFor(conversationId));
    }
    // Deliberate open (list double-click): a permanent, pinned tab. Passing the
    // real title (same as preview) keeps the pin from clobbering the chip label.
    function openConversationPinned(conversationId) {
        tabModel.openTranscriptPinned(conversationId, _titleFor(conversationId));
    }
    // Create a brand-new conversation and open it in a pinned tab.
    function createNew() {
        const id = creator.createConversation("");
        tabModel.openTranscriptPinned(id, _titleFor(id));
    }
    // Open an app-level manager/settings page as a singleton tab (the GUI's
    // replacement for the old Nav modal overlay). Mirrors the TUI's
    // RootWidget::openManagerPage routing. `section` deep-links into pages that
    // expose one (Settings / Models).
    function openManagerPage(pageId, section) {
        const map = {
            "settings":  [TabModel.Settings,  qsTr("Settings")],
            "models":    [TabModel.Models,    qsTr("Models")],
            "accounts":  [TabModel.Accounts,  qsTr("Accounts")],
            "profiles":  [TabModel.Profiles,  qsTr("Profiles")],
            "fleet":     [TabModel.Fleet,     qsTr("Fleet")],
            "sessions":  [TabModel.Sessions,  qsTr("Sessions")],
            "dashboard": [TabModel.Dashboard, qsTr("Dashboard")],
            "approvals": [TabModel.Approvals, qsTr("Approvals")],
            "routing":   [TabModel.Routing,   qsTr("Routing")],
            "cron":      [TabModel.Cron,      qsTr("Scheduled jobs")],
        };
        const entry = map[pageId];
        if (!entry)
            return;
        tabModel.openPage(entry[0], entry[1]);
        if (section && section.length > 0) {
            const ld = tabRepeater.itemAt(tabModel.currentIndex);
            if (ld && ld.item && "section" in ld.item)
                ld.item.section = section;
        }
    }
    // Route a command (palette / slash) to the active tab's orchestrator, so the
    // command palette can drive the foreground conversation. No-op without a tab.
    function invokeActiveCommand(command) {
        if (root.activePage && root.activePage.invokeCommand)
            root.activePage.invokeCommand(command);
    }

    // Rename / export the active conversation (the /title + /save targets, and the
    // command palette's session actions), acting through the shared store + exporter.
    function renameActive() {
        if (root.activePage && root.activePage.conversationController)
            renameDialog.openFor(root.activePage.conversationController.currentId);
    }
    function exportActive() {
        if (root.activePage && root.activePage.conversationController)
            exportDialog.openFor(root.activePage.conversationController.currentId);
    }
    // Open the settings popup over the pane, bound to the active conversation.
    function openSettings() {
        settingsMenu.controller = root.activePage && root.activePage.conversationController
                                ? root.activePage.conversationController : null;
        settingsMenu.open();
    }

    // --- Bar 1: the tab strip header (fixed height, top) --------------------
    Rectangle {
        id: tabBarBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.barHeight
        // Distraction-free hides the chrome (Esc exits).
        visible: !UiSettings.distractionFree
        color: Theme.background

        // A hairline under the bar separates it from the transcript.
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 4

            // Compact (phone) only: pop back to the conversation list.
            Kit.IconButton {
                icon: FontIcons.fa_chevron_left
                iconColor: Theme.iconMuted
                implicitWidth: 30
                implicitHeight: 28
                iconPointSize: 16
                visible: LayoutState.isCompact
                tooltipText: qsTr("Back")
                onClicked: root.backRequested()
            }

            TabBar {
                Layout.fillWidth: true
                Layout.fillHeight: true
                tabModel: tabModel
                onNewTabRequested: root.createNew()
                onSettingsRequested: root.openSettings()
            }
        }
    }

    // --- Bar 2: the transcript body (fills everything below the tab bar) ----
    Item {
        id: body
        anchors.top: tabBarBar.visible ? tabBarBar.bottom : parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        // One page per tab, all kept alive (background streaming/scroll), only
        // the active one shown. Keyed by the model so a reassigned preview tab
        // reuses its page (and reloads via TranscriptPage.onConversationIdChanged).
        Repeater {
            id: tabRepeater
            model: tabModel

            delegate: Loader {
                id: pageLoader

                required property int index
                required property int kind
                required property int conversationId
                required property int tabId

                anchors.fill: parent
                visible: index === tabModel.currentIndex
                active: true
                // Transcript tabs load TranscriptPage; app-level page kinds mount
                // their manager/settings component directly as tab content (the GUI
                // equivalent of the TUI's per-kind page projection).
                sourceComponent: {
                    switch (pageLoader.kind) {
                    case TabModel.Transcript: return transcriptComp;
                    case TabModel.Settings:   return settingsComp;
                    case TabModel.Models:     return modelsComp;
                    case TabModel.Accounts:   return accountsComp;
                    case TabModel.Profiles:   return profilesComp;
                    case TabModel.Fleet:      return fleetComp;
                    case TabModel.Sessions:   return sessionsComp;
                    case TabModel.Dashboard:  return dashboardComp;
                    case TabModel.Approvals:  return approvalsComp;
                    case TabModel.Routing:    return routingComp;
                    case TabModel.Cron:       return cronComp;
                    default:                  return transcriptComp;
                    }
                }

                onLoaded: {
                    // Manager/settings pages are self-contained (they bind global
                    // context-property seams); only transcript tabs need the
                    // conversation wiring below.
                    if (pageLoader.kind !== TabModel.Transcript)
                        return;
                    // Bind reactively so a preview tab reassigned to another
                    // conversation reloads in place.
                    item.conversationId = Qt.binding(() => pageLoader.conversationId);
                    // Only the foreground tab feeds the shared footer status model.
                    item.isActive = Qt.binding(() => pageLoader.index === tabModel.currentIndex);
                    item.titleResolved.connect(function(t) {
                        tabModel.setTitle(tabModel.indexOfTabId(pageLoader.tabId), t);
                    });
                    item.openSettingsRequested.connect(root.openSettings);
                    item.commandForwarded.connect(root.paneCommandForwarded);
                    // A submit/edit "commits" to the conversation -> pin the tab.
                    item.committed.connect(function() {
                        tabModel.pinTabById(pageLoader.tabId);
                    });
                }
            }
        }

        // Empty state: no tabs open yet.
        Column {
            anchors.centerIn: parent
            visible: tabModel.count === 0
            spacing: Theme.spacing

            Kit.Glyph {
                anchors.horizontalCenter: parent.horizontalCenter
                glyph: FontIcons.fa_comments
                font.pointSize: 36 + Theme.pointSizeOffset
                color: Theme.border
            }

            QQC.Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Select a conversation")
                color: Theme.textMuted
                font.family: FontIcons.display
                font.pixelSize: 16
            }
        }
    }

    // The editor settings popup (Style / Text / Theme / Options / Trash / Reset),
    // dropped from the top-right under the "..." button.
    SettingsMenu {
        id: settingsMenu
        objectName: "settingsMenu"
        x: root.width - width - 8
        y: tabBarBar.visible ? tabBarBar.height + 4 : 8
        maxHeight: Math.round(root.height * 0.85)
        // "Search transcript" routes through the active tab's command path, which
        // opens that transcript's find bar (same as Ctrl+F / /find).
        onSearchRequested: root.invokeActiveCommand("find")
    }

    // Transcript page instantiated per tab by the Repeater above.
    Component {
        id: transcriptComp
        TranscriptPage {}
    }

    // App-level manager/settings pages, mounted as tab content by the Repeater's
    // per-kind switch. Each is self-contained (binds global context-property seams).
    Component { id: settingsComp;  SettingsPage {} }
    Component { id: modelsComp;    ModelsPage {} }
    Component { id: accountsComp;  AccountsPage {} }
    Component { id: profilesComp;  ProfilesPage {} }
    Component { id: fleetComp;     FleetPage {} }
    Component { id: sessionsComp;  SessionsPage {} }
    Component { id: dashboardComp; DashboardPage {} }
    Component { id: approvalsComp; ApprovalsPage {} }
    Component { id: routingComp;   RoutingPage {} }
    Component { id: cronComp;      CronPage {} }

    // --- Session-action dialogs (rename + export) ---------------------------
    // Rename the conversation via the store. openFor(id) seeds the field with the
    // current title and remembers the target id.
    Kit.Dialog {
        id: renameDialog
        property int targetId: -1
        title: qsTr("Rename conversation")
        width: 380
        acceptText: qsTr("Rename")

        function openFor(conversationId) {
            renameDialog.targetId = conversationId;
            renameField.text = ConversationStore.title(conversationId);
            open();
            renameField.forceActiveFocus();
            renameField.selectAll();
        }

        onAccepted: {
            if (renameDialog.targetId >= 0 && renameField.text.trim().length > 0)
                ConversationStore.renameConversation(renameDialog.targetId, renameField.text.trim());
        }

        contentItem: Kit.TextField {
            id: renameField
            underline: true
            placeholderText: qsTr("Conversation title")
            onAccepted: renameDialog.accept()
        }
    }

    // Export the conversation transcript to a JSON file via the shared Exporter.
    FileDialog {
        id: exportDialog
        property int targetId: -1
        title: qsTr("Export transcript")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON files (*.json)"), qsTr("All files (*)")]
        defaultSuffix: "json"

        function openFor(conversationId) {
            exportDialog.targetId = conversationId;
            const t = ConversationStore.title(conversationId);
            exportDialog.currentFile = "file:" + (t && t.length > 0 ? t : "conversation") + ".json";
            open();
        }

        onAccepted: {
            if (exportDialog.targetId >= 0)
                Exporter.writeFile(selectedFile, Exporter.toJson(ConversationStore, exportDialog.targetId));
        }
    }
}
