import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Tabs

// The conversation pane host. A pane-level tab strip (TabBar) sits where the old
// markdown-formatting header was; below it a StackLayout shows the active tab's
// page. Transcript tabs render a TranscriptPage (each with its own controller +
// orchestrator, so background tabs keep streaming/scroll state); page tabs render
// a generic page (Settings now). The shared TabModel is the single source of
// truth for the open tabs and the active one.
Rectangle {
    id: root

    color: Theme.background

    // Compact (phone) only: pop back to the conversation list.
    signal backRequested()

    // A throwaway controller used solely to create new conversations in the
    // shared store; each TranscriptPage owns its own controller for display.
    ConversationController {
        id: creator
        store: ConversationStore
    }

    TabModel {
        id: tabModel
    }

    // --- Public API used by the shell (Main.qml) ----------------------------
    // Open (or re-activate) a transcript tab for an existing conversation.
    function open(conversationId) {
        openConversation(conversationId);
    }
    function openConversation(conversationId) {
        tabModel.openTranscript(conversationId, qsTr("Conversation"));
    }
    // Create a brand-new conversation and open it in a tab.
    function createNew() {
        const id = creator.createConversation("");
        tabModel.openTranscript(id, qsTr("New conversation"));
    }
    // Open (or re-activate) the singleton Settings page tab.
    function openSettings() {
        tabModel.openPage(TabModel.Settings, qsTr("Settings"));
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: tab strip -------------------------------------------
        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            spacing: 4
            // Distraction-free hides the chrome (Esc exits).
            visible: !UiSettings.distractionFree

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

        // --- Body: active tab's page -------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                currentIndex: tabModel.currentIndex
                visible: tabModel.count > 0

                Repeater {
                    model: tabModel

                    delegate: Loader {
                        id: pageLoader

                        required property int index
                        required property int kind
                        required property int conversationId
                        required property int tabId

                        sourceComponent: kind === TabModel.Settings ? settingsComp
                                                                     : transcriptComp

                        onLoaded: {
                            if (kind === TabModel.Transcript) {
                                // Connect before assigning conversationId so the
                                // first title resolution (triggered by open) lands.
                                item.titleResolved.connect(function(t) {
                                    tabModel.setTitle(tabModel.indexOfTabId(pageLoader.tabId), t);
                                });
                                item.openSettingsRequested.connect(root.openSettings);
                                item.conversationId = pageLoader.conversationId;
                            }
                        }
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
    }

    // Page components instantiated per tab by the Repeater above.
    Component {
        id: transcriptComp
        TranscriptPage {}
    }
    Component {
        id: settingsComp
        SettingsPage {}
    }
}
