import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme

// Todo rows docked above the composer - the QML stand-in for Hermes' composer
// status-stack todo list (the `todo` tool is suppressed inline and relocated
// here). Backed by a ListModel of { text, done } owned by the composer; with no
// agent backend the model is populated by the demo turn driver.
ColumnLayout {
    id: root

    property var model: null

    spacing: 2

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        Layout.rightMargin: 4
        Layout.bottomMargin: 2
        spacing: 6

        Text {
            text: FontIcons.fa_square_check
            font.family: FontIcons.faSolid
            font.pixelSize: 10
            color: Theme.textMuted
        }
        QQC.Label {
            Layout.fillWidth: true
            text: qsTr("Tasks")
            font.family: FontIcons.display
            font.pixelSize: Theme.labelSize
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.labelTracking
            font.capitalization: Font.AllUppercase
            color: Theme.textMuted
        }
    }

    Repeater {
        model: root.model
        delegate: RowLayout {
            id: rowRoot
            required property int index
            required property string text
            required property bool done
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            spacing: 8

            Text {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 3
                text: rowRoot.done ? FontIcons.fa_circle_check : FontIcons.fa_circle
                font.family: FontIcons.faSolid
                font.pixelSize: 12
                color: rowRoot.done ? Theme.statusOk : Theme.textMuted
            }
            QQC.Label {
                Layout.fillWidth: true
                Layout.topMargin: 3
                Layout.bottomMargin: 3
                text: rowRoot.text
                font.family: FontIcons.display
                font.pixelSize: 12
                color: rowRoot.done ? Theme.textMuted : Theme.text
                font.strikeout: rowRoot.done
                wrapMode: Text.WordWrap
            }
        }
    }
}
