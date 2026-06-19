import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Transcript
import DaemonApp.Composer

Rectangle {
    id: root

    color: Theme.background

    function open(conversationId) {
        controller.open(conversationId);
    }

    function createNew() {
        controller.createConversation(-1);
    }

    ConversationController {
        id: controller
        store: ChatStore
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

            Kit.IconButton {
                icon: FontIcons.fa_plus
                iconColor: Theme.iconMuted
                implicitWidth: 34
                implicitHeight: 28
                iconPointSize: 16
                tooltipText: qsTr("Create a note")
                onClicked: root.createNew()
            }

            Item { Layout.fillWidth: true } // spacer
            Item { implicitWidth: 15 }

            Kit.TextButton {
                text: qsTr("Upgrade")
                implicitHeight: 28
                onClicked: {} // placeholder (subscription deferred)
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

            // Task-completion pie (static placeholder).
            Item {
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 2
                Layout.rightMargin: 2
                implicitWidth: 18
                implicitHeight: 28

                Canvas {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    property color trackColor: Theme.border
                    property color fillColor: Theme.accent
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
                        // ~40% filled wedge.
                        ctx.beginPath();
                        ctx.moveTo(cx, cy);
                        ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * 0.4);
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

                QQC.Popup {
                    id: settingsMenu
                    x: parent.width - width
                    y: parent.height + 4
                    padding: Theme.spacingSmall
                    modal: false
                    focus: true

                    background: Rectangle {
                        color: Theme.background
                        border.color: Theme.border
                        radius: Theme.radius
                    }

                    contentItem: ColumnLayout {
                        spacing: Theme.spacingSmall

                        QQC.Label {
                            text: qsTr("Theme")
                            color: Theme.textMuted
                            font.family: FontIcons.display
                            font.pixelSize: 11
                            font.capitalization: Font.AllUppercase
                            Layout.leftMargin: Theme.spacingSmall
                        }

                        RowLayout {
                            spacing: 0
                            Kit.ThemeSwatch {
                                themeName: qsTr("Light"); chipColor: Theme.chipLight
                                selected: Theme.theme === "Light"
                                onPicked: Theme.setTheme("Light")
                            }
                            Kit.ThemeSwatch {
                                themeName: qsTr("Dark"); chipColor: Theme.chipDark
                                selected: Theme.theme === "Dark"
                                onPicked: Theme.setTheme("Dark")
                            }
                            Kit.ThemeSwatch {
                                themeName: qsTr("Sepia"); chipColor: Theme.chipSepia
                                selected: Theme.theme === "Sepia"
                                onPicked: Theme.setTheme("Sepia")
                            }
                        }
                    }
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
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    content: controller.content
                }

                Composer {
                    Layout.fillWidth: true
                    onSubmitted: function(text) { controller.appendUserText(text); }
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
        }
    }
}
