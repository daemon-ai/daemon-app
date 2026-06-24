import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings

// editor settings popup: Style / Text / Theme / Options, plus Move to
// Trash and Reset all settings. Reads/writes the persisted UiSettings singleton
// and drives DaemonApp.Theme for live theme switching. The host positions the
// popup and supplies the conversation controller for Move to Trash.
QQC.Popup {
    id: root

    // ConversationController; Move to Trash archives the open conversation.
    property var controller: null

    // The user asked to find within the open transcript (host opens the find bar).
    signal searchRequested()

    // Cap on the scrollable height; the host binds this to the window height.
    property int maxHeight: 560

    readonly property int hMargin: 12

    width: 240
    padding: 0
    modal: false
    focus: true

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        border.width: 1
        radius: 10
    }

    contentItem: QQC.ScrollView {
        clip: true
        // Grow with content, but never taller than 80% of the window.
        contentWidth: availableWidth
        implicitHeight: Math.min(column.implicitHeight, root.maxHeight)
        QQC.ScrollBar.horizontal.policy: QQC.ScrollBar.AlwaysOff

        ColumnLayout {
            id: column
            width: root.width
            spacing: 0

            // ----- Transcript --------------------------------------------
            SectionHeader { text: qsTr("Transcript") }

            ActionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                Layout.topMargin: 4
                icon: FontIcons.fa_magnifying_glass
                label: qsTr("Search transcript")
                enabled: root.controller && root.controller.hasConversation
                onTriggered: {
                    root.searchRequested();
                    root.close();
                }
            }

            Divider {}

            // ----- Style -------------------------------------------------
            SectionHeader { text: qsTr("Style") }

            Row {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                spacing: 0

                FontStyleButton {
                    category: "Sans"
                    label: qsTr("Sans")
                    fonts: UiSettings.sansFonts
                    currentIndex: UiSettings.sansIndex
                    previewFamily: UiSettings.familyAt("Sans", UiSettings.sansIndex)
                    selected: UiSettings.fontCategory === "Sans"
                    onSampleClicked: UiSettings.cycleFont("Sans")
                    onFontPicked: function(index) { UiSettings.selectFont("Sans", index); }
                }
                FontStyleButton {
                    category: "Serif"
                    label: qsTr("Serif")
                    fonts: UiSettings.serifFonts
                    currentIndex: UiSettings.serifIndex
                    previewFamily: UiSettings.familyAt("Serif", UiSettings.serifIndex)
                    selected: UiSettings.fontCategory === "Serif"
                    onSampleClicked: UiSettings.cycleFont("Serif")
                    onFontPicked: function(index) { UiSettings.selectFont("Serif", index); }
                }
                FontStyleButton {
                    category: "Mono"
                    label: qsTr("Mono")
                    fonts: UiSettings.monoFonts
                    currentIndex: UiSettings.monoIndex
                    previewFamily: UiSettings.familyAt("Mono", UiSettings.monoIndex)
                    selected: UiSettings.fontCategory === "Mono"
                    onSampleClicked: UiSettings.cycleFont("Mono")
                    onFontPicked: function(index) { UiSettings.selectFont("Mono", index); }
                }
            }

            Divider {}

            // ----- Text --------------------------------------------------
            SectionHeader { text: qsTr("Text") }

            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Center text")
                checked: UiSettings.centerText
                onToggled: function(on) { UiSettings.centerText = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Distraction-free")
                checked: UiSettings.distractionFree
                onToggled: function(on) {
                    UiSettings.distractionFree = on;
                    // Get the chrome out of the way the moment it's enabled.
                    if (on)
                        root.close();
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                Layout.preferredHeight: 34
                spacing: 0

                Text {
                    Layout.leftMargin: 4
                    text: qsTr("Font size")
                    font.family: FontIcons.display
                    font.pixelSize: 14
                    color: Theme.text
                }
                Item { Layout.fillWidth: true }
                Kit.IconButton {
                    icon: FontIcons.fa_minus
                    iconColor: Theme.text
                    iconPointSize: 12
                    implicitWidth: 30
                    implicitHeight: 28
                    enabled: UiSettings.editorFontSize > UiSettings.minFontSize
                    onClicked: UiSettings.decreaseFontSize()
                }
                Kit.IconButton {
                    icon: FontIcons.fa_plus
                    iconColor: Theme.text
                    iconPointSize: 12
                    implicitWidth: 30
                    implicitHeight: 28
                    enabled: UiSettings.editorFontSize < UiSettings.maxFontSize
                    onClicked: UiSettings.increaseFontSize()
                }
            }

            Divider {}

            // ----- Theme -------------------------------------------------
            SectionHeader { text: qsTr("Theme") }

            // 2x2 grid: four 71px swatches no longer fit one row in the 240px popup.
            Grid {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                columns: 2
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
                Kit.ThemeSwatch {
                    themeName: qsTr("Midnight"); chipColor: Theme.chipMidnight
                    selected: Theme.theme === "Midnight"
                    onPicked: Theme.setTheme("Midnight")
                }
            }

            Divider {}

            // ----- Options -----------------------------------------------
            SectionHeader { text: qsTr("Options") }

            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Show Sessions")
                checked: UiSettings.showSessionsList
                onToggled: function(on) { UiSettings.showSessionsList = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Show Fleet Tree")
                checked: UiSettings.showFleetTree
                onToggled: function(on) { UiSettings.showFleetTree = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Show Raw Markdown")
                checked: UiSettings.showRawMarkdown
                onToggled: function(on) { UiSettings.showRawMarkdown = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("User message rail")
                checked: UiSettings.showUserRail
                onToggled: function(on) { UiSettings.showUserRail = on; }
            }

            Divider {}

            // ----- Actions -----------------------------------------------
            ActionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                icon: FontIcons.fa_trash
                label: qsTr("Move to Trash")
                enabled: root.controller && root.controller.hasConversation
                onTriggered: {
                    if (root.controller)
                        root.controller.moveCurrentToTrash();
                    root.close();
                }
            }

            Divider {}

            ActionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                Layout.bottomMargin: 6
                icon: FontIcons.fa_undo_alt
                label: qsTr("Reset all settings")
                onTriggered: resetDialog.open()
            }
        }
    }

    Kit.Dialog {
        id: resetDialog
        // Explicit width: a wrapping Text contentItem would otherwise feed its
        // implicitWidth back into the Dialog's implicitWidth (binding loop).
        width: 300
        title: qsTr("Reset settings?")
        acceptText: qsTr("Reset")
        destructive: true

        contentItem: Text {
            width: resetDialog.availableWidth
            text: qsTr("Reset all settings to their defaults?\nThis only affects the app appearance, not your data.")
            wrapMode: Text.Wrap
            color: Theme.text
            font.family: FontIcons.display
            font.pixelSize: 13
        }

        onAccepted: {
            UiSettings.resetAll();
            Theme.setTheme(UiSettings.theme);
        }
    }

    // ----- Local building blocks -------------------------------------------
    component SectionHeader: Text {
        Layout.fillWidth: true
        Layout.leftMargin: root.hMargin
        Layout.topMargin: 10
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
        Layout.leftMargin: root.hMargin
        Layout.rightMargin: root.hMargin
        Layout.topMargin: 8
        implicitHeight: 1
        color: Theme.border
    }

    component ActionRow: Item {
        id: actionRoot
        property string icon: ""
        property string label: ""
        signal triggered()

        implicitHeight: 38

        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: root.hMargin - 4
            anchors.rightMargin: root.hMargin - 4
            radius: 5
            color: actionArea.containsMouse && actionRoot.enabled ? Theme.hover : "transparent"
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: root.hMargin
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            opacity: actionRoot.enabled ? 1.0 : 0.4

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                horizontalAlignment: Text.AlignHCenter
                text: actionRoot.icon
                font.family: FontIcons.faSolid
                font.pixelSize: 15
                color: Theme.text
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: actionRoot.label
                font.family: FontIcons.display
                font.pixelSize: 14
                color: Theme.text
            }
        }

        MouseArea {
            id: actionArea
            anchors.fill: parent
            hoverEnabled: true
            enabled: actionRoot.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: actionRoot.triggered()
        }
    }
}
