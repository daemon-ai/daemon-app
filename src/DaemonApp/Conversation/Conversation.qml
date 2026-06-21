import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings
import DaemonApp.Transcript
import DaemonApp.Composer

Rectangle {
    id: root

    color: Theme.background

    // No task model yet, so the header pie stays hidden (it only shows when a
    // conversation has checklist tasks, per CircularProgressBarPie).
    property int totalTasks: 0
    property int doneTasks: 0

    // Emitted by the compact (phone) back button so the shell's StackView can pop
    // back to the conversation list.
    signal backRequested()

    function open(conversationId) {
        controller.open(conversationId);
    }

    function createNew() {
        controller.createConversation("");
    }

    ConversationController {
        id: controller
        store: ConversationStore
    }

    // The shared submit pipeline: owns the turn (injected into the transcript) and
    // the status-stack todos (rendered by the composer). The QML only routes the
    // composer's intents into it and handles the front-end-only slash commands it
    // surfaces via commandRequested.
    ConversationOrchestrator {
        id: orchestrator
        conversation: controller
        onCommandRequested: function(command) {
            if (command === "theme")
                settingsMenu.open();
            else if (command === "distraction")
                UiSettings.distractionFree = true;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Header: editor top bar -----------------------
        // Only `+` (new) and `...` (settings) are functional; the formatting
        // controls are faithful placeholders pending the block editor.
        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            implicitHeight: 28 + 24
            spacing: 0
            // Distraction-free hides the formatting/settings chrome (Esc exits).
            visible: !UiSettings.distractionFree

            // Compact (phone) only: pop back to the conversation list. The shell
            // pushes the conversation onto a StackView, so this is the up affordance.
            Kit.IconButton {
                icon: FontIcons.fa_chevron_left
                iconColor: Theme.iconMuted
                implicitWidth: 34
                implicitHeight: 28
                iconPointSize: 16
                visible: LayoutState.isCompact
                tooltipText: qsTr("Back")
                onClicked: root.backRequested()
            }

            Kit.IconButton {
                icon: FontIcons.fa_plus
                iconColor: Theme.iconMuted
                implicitWidth: 34
                implicitHeight: 28
                iconPointSize: 16
                tooltipText: qsTr("Create a note")
                onClicked: root.createNew()
            }

            Item { implicitWidth: 15 }

            // Upgrade: bordered pill, radius 7, 1px toolButtonColor, 13px text.
            QQC.Button {
                id: upgradeButton
                implicitHeight: 26
                leftPadding: 12
                rightPadding: 12
                topPadding: 4
                bottomPadding: 4
                text: qsTr("Upgrade")
                onClicked: {} // placeholder (subscription deferred)

                contentItem: QQC.Label {
                    text: upgradeButton.text
                    color: Theme.iconMuted
                    font.family: FontIcons.display
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: Theme.radius
                    color: upgradeButton.down ? Theme.pressed
                         : upgradeButton.hovered ? Theme.hover : "transparent"
                    border.width: 1
                    border.color: Theme.iconMuted
                }
            }

            Item { implicitWidth: 15 }

            // Headings: H + chevron.
            ToolGroup {
                implicitWidth: 40
                tooltipText: qsTr("Add a title or a heading")
                content: [
                    Kit.Glyph {
                        glyph: FontIcons.fa_heading
                        font.pointSize: 12 + Theme.pointSizeOffset
                        color: Theme.iconMuted
                    },
                    Kit.Glyph {
                        glyph: FontIcons.fa_chevron_down
                        font.pointSize: 8 + Theme.pointSizeOffset
                        color: Theme.iconMuted
                    }
                ]
            }

            // Checklist.
            ToolGroup {
                implicitWidth: 30
                tooltipText: qsTr("Add a checklist")
                content: Kit.Glyph {
                    glyph: FontIcons.fa_square_check
                    font.pointSize: 13 + Theme.pointSizeOffset
                    color: Theme.iconMuted
                }
            }

            // Lists: bullets + chevron.
            ToolGroup {
                implicitWidth: 40
                tooltipText: qsTr("Lists")
                content: [
                    Kit.Glyph {
                        glyph: FontIcons.fa_list_ul
                        font.pointSize: 12 + Theme.pointSizeOffset
                        color: Theme.iconMuted
                    },
                    Kit.Glyph {
                        glyph: FontIcons.fa_chevron_down
                        font.pointSize: 8 + Theme.pointSizeOffset
                        color: Theme.iconMuted
                    }
                ]
            }

            // Divider.
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 4
                Layout.rightMargin: 4
                implicitWidth: 1
                implicitHeight: 14
                color: Theme.border
            }

            // Text styles: Aa + chevron.
            ToolGroup {
                implicitWidth: 44
                tooltipText: qsTr("Text styles")
                content: [
                    QQC.Label {
                        text: qsTr("Aa")
                        font.family: FontIcons.display
                        font.pointSize: 12 + Theme.pointSizeOffset
                        color: Theme.iconMuted
                    },
                    Kit.Glyph {
                        glyph: FontIcons.fa_chevron_down
                        font.pointSize: 8 + Theme.pointSizeOffset
                        color: Theme.iconMuted
                    }
                ]
            }

            // Link.
            ToolGroup {
                implicitWidth: 30
                tooltipText: qsTr("Add a link")
                content: Kit.Glyph {
                    glyph: FontIcons.fa_link
                    font.pointSize: 12 + Theme.pointSizeOffset
                    color: Theme.iconMuted
                }
            }

            // Kanban.
            ToolGroup {
                implicitWidth: 30
                tooltipText: qsTr("Kanban board")
                content: Kit.Glyph {
                    glyph: FontIcons.mt_view_kanban
                    family: FontIcons.mtSymbols
                    font.pointSize: 15 + Theme.pointSizeOffset
                    color: Theme.iconMuted
                }
            }

            // Image.
            ToolGroup {
                implicitWidth: 30
                tooltipText: qsTr("Add an image")
                content: Kit.Glyph {
                    glyph: FontIcons.fa_image
                    font.pointSize: 12 + Theme.pointSizeOffset
                    color: Theme.iconMuted
                }
            }

            // Push the pie + settings to the right edge (topBar spacer + 15px),
            // leaving the formatting cluster left-aligned after Upgrade.
            Item { Layout.fillWidth: true }
            Item { implicitWidth: 15 }

            // Task-completion pie (CircularProgressBarPie): gray, lineWidth 2,
            // size 20, and ONLY visible when the conversation has tasks. With no
            // task model it stays hidden.
            Item {
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 2
                Layout.rightMargin: 2
                implicitWidth: 20
                implicitHeight: 28
                visible: root.totalTasks > 0

                Canvas {
                    id: pie
                    anchors.centerIn: parent
                    width: 20
                    height: 20
                    property color trackColor: Theme.border
                    property color fillColor: Theme.iconMuted
                    property real progress: root.totalTasks > 0 ? root.doneTasks / root.totalTasks : 0
                    onProgressChanged: requestPaint()
                    onPaint: {
                        const ctx = getContext("2d");
                        const cx = width / 2;
                        const cy = height / 2;
                        const r = width / 2 - 1;
                        ctx.reset();
                        ctx.beginPath();
                        ctx.arc(cx, cy, r, 0, Math.PI * 2);
                        ctx.lineWidth = 2;
                        ctx.strokeStyle = trackColor;
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.moveTo(cx, cy);
                        ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * progress);
                        ctx.closePath();
                        ctx.fillStyle = fillColor;
                        ctx.fill();
                    }
                }
            }

            // Settings / theme menu.
            Kit.IconButton {
                icon: FontIcons.fa_ellipsis_h
                iconColor: Theme.iconMuted
                implicitWidth: 34
                implicitHeight: 28
                tooltipText: qsTr("Settings")
                onClicked: settingsMenu.open()

                SettingsMenu {
                    id: settingsMenu
                    objectName: "settingsMenu"
                    x: parent.width - width
                    y: parent.height + 4
                    controller: controller
                    maxHeight: Math.round(root.height * 0.85)
                }
            }
        }

        // --- Body -----------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                visible: controller.hasConversation

                Transcript {
                    id: transcript
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    // The orchestrator owns the turn; the transcript is a consumer.
                    turn: orchestrator.turn
                    onEdited: function(markdown) { controller.updateContent(markdown); }

                    // Load on first realize; conversation switches and external
                    // content changes (e.g. appended messages) reload below.
                    Component.onCompleted: load(controller.content)

                    Connections {
                        target: controller
                        // A new conversation became current.
                        function onConversationChanged() { transcript.load(controller.content); }
                        // External content change (append); the user's own edits go
                        // through updateContent() and do not emit contentChanged.
                        function onContentChanged() { transcript.load(controller.content); }
                    }
                }

                Composer {
                    id: composer
                    Layout.fillWidth: true
                    // Match the transcript's centered column when center text is on
                    // (contentMaxWidth defaults to 720, the transcript column width).
                    centerContent: UiSettings.centerText
                    // Stays visible in distraction-free: hiding the composer would
                    // remove the only way to talk to the agent. Only the header
                    // chrome and surrounding app panes hide in distraction-free.
                    busy: transcript.busy
                    conversationId: controller.currentId
                    // The status-stack todos are owned/lifecycled by the orchestrator.
                    todosModel: orchestrator.todos

                    // All submit orchestration (persist + start turn + todos) lives
                    // in the shared orchestrator; the QML only routes intents.
                    onSubmitted: function(text, attachmentRefs) {
                        orchestrator.submit(text, attachmentRefs);
                    }
                    onSteer: function(text) { orchestrator.steer(text); }
                    onCancelRequested: orchestrator.cancel()
                    onCommandInvoked: function(command) { orchestrator.invokeCommand(command); }
                }
            }

            Column {
                anchors.centerIn: parent
                visible: !controller.hasConversation
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

            // Distraction-free hides the header (and its settings menu), so keep
            // an always-visible escape hatch overlaid on the editor.
            Kit.IconButton {
                visible: UiSettings.distractionFree
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                z: 10
                icon: FontIcons.fa_xmark
                iconColor: Theme.iconMuted
                backgroundRadius: width / 2
                tooltipText: qsTr("Exit distraction-free (Esc)")
                onClicked: UiSettings.distractionFree = false
            }
        }
    }
}
