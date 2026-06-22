import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings

// Full-pane Settings, shown as a generic (non-transcript) tab. It renders the
// same UiSettings-backed options as the compact SettingsMenu popup, but laid out
// for the whole pane width - proving the tab strip hosts arbitrary pages, not
// just transcripts.
Rectangle {
    id: root

    color: Theme.background

    readonly property int hMargin: 20

    QQC.ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        QQC.ScrollBar.horizontal.policy: QQC.ScrollBar.AlwaysOff

        ColumnLayout {
            width: root.width
            spacing: 0

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.topMargin: 18
                text: qsTr("Settings")
                font.family: FontIcons.display
                font.pixelSize: 22
                font.weight: Font.DemiBold
                color: Theme.text
            }

            // ----- Theme ------------------------------------------------------
            SectionHeader { text: qsTr("Theme") }

            Grid {
                Layout.leftMargin: root.hMargin
                Layout.topMargin: 4
                columns: 4
                spacing: 4

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

            // ----- Text -------------------------------------------------------
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
                onToggled: function(on) { UiSettings.distractionFree = on; }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                Layout.preferredHeight: 34

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

            // ----- Options ----------------------------------------------------
            SectionHeader { text: qsTr("Options") }

            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Show notes list")
                checked: UiSettings.showNotesList
                onToggled: function(on) { UiSettings.showNotesList = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Show folders tree")
                checked: UiSettings.showFoldersTree
                onToggled: function(on) { UiSettings.showFoldersTree = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                label: qsTr("Show plain text")
                checked: UiSettings.showPlainText
                onToggled: function(on) { UiSettings.showPlainText = on; }
            }
            OptionRow {
                Layout.fillWidth: true
                Layout.leftMargin: root.hMargin
                Layout.rightMargin: root.hMargin
                Layout.bottomMargin: 18
                label: qsTr("User message rail")
                checked: UiSettings.showUserRail
                onToggled: function(on) { UiSettings.showUserRail = on; }
            }
        }
    }

    component SectionHeader: Text {
        Layout.fillWidth: true
        Layout.leftMargin: root.hMargin
        Layout.topMargin: 16
        Layout.bottomMargin: 2
        font.family: FontIcons.display
        font.pixelSize: Theme.labelSize
        font.weight: Font.DemiBold
        font.letterSpacing: Theme.labelTracking
        font.capitalization: Font.AllUppercase
        color: Theme.textMuted
    }
}
