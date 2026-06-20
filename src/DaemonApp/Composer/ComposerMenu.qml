import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// The composer "+" menu - the QML port of Hermes' ContextMenu
// (apps/desktop/src/app/chat/composer/context-menu.tsx): attach files / folder /
// images / URL, a prompt-snippets section, and a "type @ to mention" tip. There
// is no real upload pipeline yet, so attach actions surface as signals the host
// (or the composer's own mock pipeline) fulfills.
Item {
    id: root

    property bool composerEnabled: true

    // Attach intents (host wires real pickers later; the composer mocks chips).
    signal requestFiles()
    signal requestFolder()
    signal requestImages()
    signal requestUrl()
    // Insert prompt-snippet text into the draft.
    signal insertText(string text)

    property var snippets: [
        { label: qsTr("Code review"), text: qsTr("Review the following code for correctness, clarity, and edge cases:\n\n") },
        { label: qsTr("Implementation plan"), text: qsTr("Draft a step-by-step implementation plan for:\n\n") },
        { label: qsTr("Explain this"), text: qsTr("Explain what the following does and why:\n\n") }
    ]

    implicitWidth: 30
    implicitHeight: 30

    Kit.IconButton {
        id: trigger
        anchors.fill: parent
        icon: FontIcons.fa_plus
        iconColor: Theme.iconMuted
        iconPointSize: 15
        tooltipText: qsTr("Attach and more")
        enabled: root.composerEnabled
        onClicked: menu.open()
    }

    QQC.Popup {
        id: menu
        // Anchored to the trigger, opening upward (side="top" parity).
        y: -implicitHeight - 8
        x: 0
        width: 240
        padding: 6
        modal: false
        focus: true

        background: Rectangle {
            color: Theme.background
            border.color: Theme.border
            border.width: 1
            radius: 10
        }

        contentItem: ColumnLayout {
            spacing: 0

            SectionLabel { text: qsTr("Attach") }

            MenuRow {
                icon: FontIcons.fa_paperclip
                label: qsTr("Files")
                onTriggered: { root.requestFiles(); menu.close(); }
            }
            MenuRow {
                icon: FontIcons.fa_folder
                label: qsTr("Folder")
                onTriggered: { root.requestFolder(); menu.close(); }
            }
            MenuRow {
                icon: FontIcons.fa_image
                label: qsTr("Images")
                onTriggered: { root.requestImages(); menu.close(); }
            }
            MenuRow {
                icon: FontIcons.fa_link
                label: qsTr("URL")
                onTriggered: { root.requestUrl(); menu.close(); }
            }

            Divider {}

            SectionLabel { text: qsTr("Prompt snippets") }

            Repeater {
                model: root.snippets
                delegate: MenuRow {
                    required property var modelData
                    icon: FontIcons.mt_article
                    iconFamily: FontIcons.mtSymbols
                    label: modelData.label
                    onTriggered: { root.insertText(modelData.text); menu.close(); }
                }
            }

            Divider {}

            // Tip: type @ to mention (mirrors the context menu footer).
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 8
                Layout.topMargin: 6
                spacing: 6

                Rectangle {
                    implicitWidth: 18
                    implicitHeight: 18
                    radius: 4
                    color: Theme.codeBackground
                    Text {
                        anchors.centerIn: parent
                        text: FontIcons.fa_at
                        font.family: FontIcons.faSolid
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                }
                QQC.Label {
                    Layout.fillWidth: true
                    text: qsTr("Type @ to mention")
                    font.family: FontIcons.display
                    font.pixelSize: 11
                    color: Theme.textMuted
                    wrapMode: Text.WordWrap
                }
            }
        }
    }

    // ----- Local building blocks -------------------------------------------
    component SectionLabel: QQC.Label {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.topMargin: 6
        Layout.bottomMargin: 2
        font.family: FontIcons.display
        font.pixelSize: Theme.labelSize
        font.weight: Font.DemiBold
        font.letterSpacing: Theme.labelTracking
        font.capitalization: Font.AllUppercase
        color: Theme.textMuted
    }

    component Divider: Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 6
        Layout.rightMargin: 6
        Layout.topMargin: 6
        Layout.bottomMargin: 2
        implicitHeight: 1
        color: Theme.border
    }

    component MenuRow: Item {
        id: rowRoot
        property string icon: ""
        property string iconFamily: FontIcons.faSolid
        property string label: ""
        signal triggered()

        Layout.fillWidth: true
        implicitHeight: 34

        Rectangle {
            anchors.fill: parent
            radius: 5
            color: rowArea.containsMouse ? Theme.hover : "transparent"
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 10

            Text {
                Layout.preferredWidth: 16
                horizontalAlignment: Text.AlignHCenter
                text: rowRoot.icon
                font.family: rowRoot.iconFamily
                font.pixelSize: 14
                color: Theme.iconMuted
            }
            QQC.Label {
                Layout.fillWidth: true
                text: rowRoot.label
                font.family: FontIcons.display
                font.pixelSize: 13
                color: Theme.text
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }

        MouseArea {
            id: rowArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: rowRoot.triggered()
        }
    }
}
