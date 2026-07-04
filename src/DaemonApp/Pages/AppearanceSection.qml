// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit
import DaemonApp.Settings

// App-wide appearance settings. Binds the same UiSettings singleton the
// per-transcript "..." popup uses, so the page and the popup stay consistent;
// the transcript popup is left untouched. Theme/fonts/layout are all client-local
// app prefs and work offline.
ColumnLayout {
    id: root
    spacing: 18

    SectionLabel { text: qsTr("Theme") }

    Grid {
        Layout.fillWidth: true
        columns: 4
        spacing: 8

        Kit.ThemeSwatch {
            themeName: qsTr("Light"); chipColor: Theme.chipLight
            selected: Theme.theme === "Light"; onPicked: Theme.setTheme("Light")
        }
        Kit.ThemeSwatch {
            themeName: qsTr("Dark"); chipColor: Theme.chipDark
            selected: Theme.theme === "Dark"; onPicked: Theme.setTheme("Dark")
        }
        Kit.ThemeSwatch {
            themeName: qsTr("Sepia"); chipColor: Theme.chipSepia
            selected: Theme.theme === "Sepia"; onPicked: Theme.setTheme("Sepia")
        }
        Kit.ThemeSwatch {
            themeName: qsTr("Midnight"); chipColor: Theme.chipMidnight
            selected: Theme.theme === "Midnight"; onPicked: Theme.setTheme("Midnight")
        }
    }

    SectionLabel { text: qsTr("Language") }

    SettingRow {
        label: qsTr("Language")
        description: qsTr("Language used across the interface. Applied immediately.")
        Kit.Dropdown {
            id: languageCombo
            Layout.preferredWidth: 200
            // {code,label} list from UiSettings; show labels, persist codes.
            readonly property var langs: UiSettings.availableLanguages
            model: langs.map(function(entry) { return entry.label })
            Component.onCompleted: currentIndex = Math.max(0, langs.findIndex(
                function(entry) { return entry.code === UiSettings.language }))
            onActivated: UiSettings.language = langs[currentIndex].code
        }
    }

    SectionLabel { text: qsTr("Editor text") }

    SettingRow {
        label: qsTr("Font")
        description: qsTr("Typeface used for the transcript / editor text.")
        RowLayout {
            spacing: 6
            // Category (Sans / Serif / Mono) - the same buckets the transcript
            // "..." popup offers, driving the shared UiSettings store.
            Kit.Dropdown {
                id: categoryCombo
                Layout.preferredWidth: 90
                // Stored value stays the canonical Sans/Serif/Mono identifier; the
                // dropdown shows a translatable label for each.
                readonly property var categoryValues: ["Sans", "Serif", "Mono"]
                model: [qsTr("Sans"), qsTr("Serif"), qsTr("Mono")]
                Component.onCompleted: currentIndex =
                    Math.max(0, categoryValues.indexOf(UiSettings.fontCategory))
                onActivated: UiSettings.fontCategory = categoryValues[currentIndex]
            }
            // Specific face within the active category.
            Kit.Dropdown {
                id: fontCombo
                Layout.preferredWidth: 150
                readonly property var fonts: UiSettings.fontCategory === "Serif"
                    ? UiSettings.serifFonts
                    : UiSettings.fontCategory === "Mono" ? UiSettings.monoFonts
                                                         : UiSettings.sansFonts
                readonly property int fontIdx: UiSettings.fontCategory === "Serif"
                    ? UiSettings.serifIndex
                    : UiSettings.fontCategory === "Mono" ? UiSettings.monoIndex
                                                         : UiSettings.sansIndex
                model: fonts
                currentIndex: fontIdx
                onActivated: UiSettings.selectFont(UiSettings.fontCategory, currentIndex)
            }
        }
    }

    SettingRow {
        label: qsTr("Font size")
        description: qsTr("Size of the transcript / editor text.")
        RowLayout {
            spacing: 6
            Kit.IconButton {
                icon: FontIcons.fa_minus; iconColor: Theme.text; iconPointSize: 12
                implicitWidth: 30; implicitHeight: 28
                enabled: UiSettings.editorFontSize > UiSettings.minFontSize
                onClicked: UiSettings.decreaseFontSize()
            }
            Text {
                text: UiSettings.editorFontSize
                font.family: FontIcons.mono; font.pixelSize: 14; color: Theme.text
                horizontalAlignment: Text.AlignHCenter
                Layout.preferredWidth: 26
            }
            Kit.IconButton {
                icon: FontIcons.fa_plus; iconColor: Theme.text; iconPointSize: 12
                implicitWidth: 30; implicitHeight: 28
                enabled: UiSettings.editorFontSize < UiSettings.maxFontSize
                onClicked: UiSettings.increaseFontSize()
            }
        }
    }

    SectionLabel { text: qsTr("Layout") }

    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Center text"); checked: UiSettings.centerText
        onToggled: function(on) { UiSettings.centerText = on; }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Distraction-free (hide chrome)"); checked: UiSettings.distractionFree
        onToggled: function(on) { UiSettings.distractionFree = on; }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Raw markdown editor"); checked: UiSettings.showRawMarkdown
        onToggled: function(on) { UiSettings.showRawMarkdown = on; }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Show Sessions"); checked: UiSettings.showSessionsList
        onToggled: function(on) { UiSettings.showSessionsList = on; }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("Show Fleet Tree"); checked: UiSettings.showFleetTree
        onToggled: function(on) { UiSettings.showFleetTree = on; }
    }
    OptionRow {
        Layout.fillWidth: true
        label: qsTr("User message rail"); checked: UiSettings.showUserRail
        onToggled: function(on) { UiSettings.showUserRail = on; }
    }

    Item { Layout.fillHeight: true }
}
