import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme

// The composer status stack - the region docked directly above the composer
// surface that carries todos and the prompt queue (Hermes' ComposerStatusStack
// + QueuePanel). Collapses to nothing when every section is empty so it never
// inflates the composer's height at rest.
Item {
    id: root

    property var queueModel: null
    property var todoModel: null
    property var subagentModel: null
    property int editingIndex: -1
    property bool busy: false

    signal sendNow(int index)
    signal editEntry(int index)
    signal deleteEntry(int index)

    readonly property bool hasTodos: todoModel && todoModel.count > 0
    readonly property bool hasQueue: queueModel && queueModel.count > 0
    readonly property bool hasSubagents: subagentModel && subagentModel.count > 0
    readonly property bool hasContent: hasTodos || hasQueue || hasSubagents

    visible: hasContent
    implicitHeight: visible ? card.implicitHeight : 0

    Rectangle {
        id: card
        width: parent.width
        implicitHeight: column.implicitHeight + 2 * Theme.spacingSmall
        radius: Theme.radius
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.border

        ColumnLayout {
            id: column
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.spacingSmall
            spacing: Theme.spacingSmall

            SubagentList {
                Layout.fillWidth: true
                visible: root.hasSubagents
                model: root.subagentModel
            }

            Rectangle {
                Layout.fillWidth: true
                visible: root.hasSubagents && (root.hasTodos || root.hasQueue)
                implicitHeight: 1
                color: Theme.border
            }

            TodoList {
                Layout.fillWidth: true
                visible: root.hasTodos
                model: root.todoModel
            }

            Rectangle {
                Layout.fillWidth: true
                visible: root.hasTodos && root.hasQueue
                implicitHeight: 1
                color: Theme.border
            }

            QueuePanel {
                Layout.fillWidth: true
                visible: root.hasQueue
                model: root.queueModel
                busy: root.busy
                editingIndex: root.editingIndex
                onSendNow: function(index) { root.sendNow(index); }
                onEditEntry: function(index) { root.editEntry(index); }
                onDeleteEntry: function(index) { root.deleteEntry(index); }
            }
        }
    }
}
