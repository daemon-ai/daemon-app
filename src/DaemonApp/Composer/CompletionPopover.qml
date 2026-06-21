import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// The slash / @ completion popover - the QML port of Hermes'
// ComposerTriggerPopover. It is a presentation surface: the shared
// ComposerSessionController owns the trigger detection + keyboard navigation
// (keys arrive through the text input) and drives `items` / `activeIndex`; this
// popover renders the list and reports mouse interactions by row index.
QQC.Popup {
    id: root

    // A CompletionModel (QAbstractListModel with label/hint/group/value/action
    // roles) owned by the controller.
    property var items: null
    property int activeIndex: 0
    // "slash" | "mention" - drives the leading badge.
    property string kind: "slash"

    // Row index interactions (the controller maps them onto its item list).
    signal picked(int index)
    signal hovered(int index)

    width: 320
    padding: 1
    modal: false
    // Never steal focus from the text input; navigation is driven by the composer.
    closePolicy: QQC.Popup.NoAutoClose

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: 10
    }

    contentItem: ColumnLayout {
        spacing: 0

        Repeater {
            model: root.items
            delegate: Item {
                id: itemRoot
                required property int index
                required property string label
                required property string hint
                required property string group
                Layout.fillWidth: true
                implicitHeight: 36

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    radius: 6
                    color: itemRoot.index === root.activeIndex ? Theme.hover : "transparent"
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 10
                    spacing: 8

                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: 20
                        implicitHeight: 20
                        radius: 4
                        color: Theme.codeBackground
                        Text {
                            anchors.centerIn: parent
                            text: root.kind === "mention" ? FontIcons.fa_at : "/"
                            font.family: root.kind === "mention" ? FontIcons.faSolid : FontIcons.display
                            font.pixelSize: root.kind === "mention" ? 10 : 13
                            font.weight: Font.DemiBold
                            color: Theme.accent
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        QQC.Label {
                            Layout.fillWidth: true
                            text: itemRoot.label
                            font.family: FontIcons.display
                            font.pixelSize: 13
                            color: Theme.text
                            elide: Text.ElideRight
                        }
                        QQC.Label {
                            Layout.fillWidth: true
                            visible: itemRoot.hint !== ""
                            text: itemRoot.hint
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            color: Theme.textMuted
                            elide: Text.ElideRight
                        }
                    }

                    QQC.Label {
                        visible: itemRoot.group !== ""
                        text: itemRoot.group
                        font.family: FontIcons.display
                        font.pixelSize: 10
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: Theme.labelTracking
                        color: Theme.textMuted
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: root.hovered(itemRoot.index)
                    onClicked: root.picked(itemRoot.index)
                }
            }
        }
    }
}
