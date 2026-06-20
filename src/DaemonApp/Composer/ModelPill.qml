import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// The composer model selector - the QML port of Hermes' ModelPill
// (apps/desktop/src/app/chat/composer/model-pill.tsx). A quiet pill showing the
// current model + a caret; clicking opens an upward menu of a canned model list.
// There is no model backend yet, so the selection is held in-memory and is
// purely cosmetic.
Item {
    id: root

    // Canned model list (no gateway model backend exists yet).
    property var models: [
        "claude-opus-4.8",
        "claude-sonnet-4.6",
        "gpt-5.5",
        "gpt-5.3-codex",
        "gemini-3-pro"
    ]
    property int currentIndex: 0
    readonly property string currentModel: models[currentIndex] !== undefined ? models[currentIndex] : ""

    implicitWidth: pill.implicitWidth
    implicitHeight: 28

    Rectangle {
        id: pill
        anchors.fill: parent
        radius: Theme.radius
        implicitWidth: row.implicitWidth + 16
        color: !root.enabled ? "transparent"
             : pillArea.pressed ? Theme.pressed
             : pillArea.containsMouse ? Theme.hover
             : "transparent"

        Behavior on color { ColorAnimation { duration: Theme.motionFast } }

        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: 5

            QQC.Label {
                Layout.maximumWidth: 150
                text: root.currentModel
                font.family: FontIcons.display
                font.pixelSize: 12
                color: root.enabled ? Theme.textMuted : Theme.textMuted
                opacity: root.enabled ? 1.0 : 0.5
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                text: FontIcons.fa_chevron_down
                font.family: FontIcons.faSolid
                font.pixelSize: 8
                color: Theme.textMuted
                opacity: 0.6
                verticalAlignment: Text.AlignVCenter
            }
        }

        MouseArea {
            id: pillArea
            anchors.fill: parent
            hoverEnabled: true
            enabled: root.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: menu.open()
        }
    }

    QQC.Popup {
        id: menu
        // Open upward (menu sits above the pill, like Hermes' side="top").
        y: -implicitHeight - 6
        x: parent.width - width
        width: 220
        padding: 1
        modal: false
        focus: true

        background: Rectangle {
            color: Theme.background
            border.color: Theme.border
            border.width: 1
            radius: Theme.radius
        }

        contentItem: ColumnLayout {
            spacing: 0

            Repeater {
                model: root.models
                delegate: Item {
                    id: itemRoot
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    implicitHeight: 32

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 1
                        radius: 5
                        color: itemArea.containsMouse ? Theme.hover : "transparent"
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        spacing: 8

                        QQC.Label {
                            Layout.fillWidth: true
                            text: itemRoot.modelData
                            font.family: FontIcons.display
                            font.pixelSize: 12
                            color: itemRoot.index === root.currentIndex ? Theme.accent : Theme.text
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            visible: itemRoot.index === root.currentIndex
                            text: FontIcons.check
                            font.family: FontIcons.faSolid
                            font.pixelSize: 11
                            color: Theme.accent
                        }
                    }

                    MouseArea {
                        id: itemArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentIndex = itemRoot.index;
                            menu.close();
                        }
                    }
                }
            }
        }
    }
}
