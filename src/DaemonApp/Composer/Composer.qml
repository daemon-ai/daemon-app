import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

Rectangle {
    id: root

    color: Theme.surface
    implicitHeight: layout.implicitHeight + 2 * Theme.spacing

    signal submitted(string text)

    function send() {
        const trimmed = input.text.trim();
        if (trimmed.length > 0) {
            root.submitted(trimmed);
            input.clear();
        }
    }

    // Top separator line.
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing
        spacing: Theme.spacingSmall

        // Rounded, bordered input field that grows up to a max height.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(Math.max(input.implicitHeight + 2 * Theme.spacingSmall, 38), 140)
            radius: Theme.radius
            color: Theme.searchBackground
            border.width: input.activeFocus ? 2 : 1
            border.color: input.activeFocus ? Theme.searchFocusBorder : Theme.searchBorder

            QQC.ScrollView {
                anchors.fill: parent
                anchors.margins: Theme.spacingSmall

                QQC.TextArea {
                    id: input
                    placeholderText: qsTr("Send a message...  (Enter to send, Shift+Enter for newline)")
                    placeholderTextColor: Theme.textMuted
                    wrapMode: QQC.TextArea.Wrap
                    color: Theme.text
                    selectionColor: Theme.searchSelection
                    selectedTextColor: Theme.text
                    font.family: FontIcons.display
                    font.pixelSize: 14
                    background: null

                    Keys.onReturnPressed: function(event) {
                        if (event.modifiers & Qt.ShiftModifier) {
                            event.accepted = false;
                        } else {
                            event.accepted = true;
                            root.send();
                        }
                    }
                }
            }
        }

        Kit.IconButton {
            Layout.alignment: Qt.AlignBottom
            implicitWidth: 38
            implicitHeight: 38
            icon: FontIcons.fa_paper_plane
            iconColor: enabled ? Theme.accent : Theme.textMuted
            iconPointSize: 17
            tooltipText: qsTr("Send")
            enabled: input.text.trim().length > 0
            onClicked: root.send()
        }
    }
}
