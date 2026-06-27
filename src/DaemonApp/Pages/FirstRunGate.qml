import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Full-screen onboarding gate, mounted over the shell while FirstRun.active. It
// renders one of the FirstRun phases: the connection picker, a connecting
// splash, or the inference gate. Setup completes via FirstRun.completeInference,
// after which the shell is shown.
Rectangle {
    id: root
    anchors.fill: parent
    color: Theme.background
    focus: true

    readonly property string phase: FirstRun ? FirstRun.phase : "connect"

    // Centered onboarding card.
    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 560)
        height: Math.min(parent.height - 80, content.implicitHeight + 64)
        radius: 14
        color: Theme.surface
        border.color: Theme.border
        border.width: 1

        ColumnLayout {
            id: content
            anchors.fill: parent
            anchors.margins: 32
            spacing: 18

            // --- Brand / title ---
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Text {
                    text: qsTr("Welcome to daemon-app")
                    font.family: FontIcons.display; font.pixelSize: 22
                    font.weight: Font.DemiBold; color: Theme.text
                }
                Text {
                    text: root.phase === "inference"
                          ? qsTr("Almost there - confirm an inference model.")
                          : qsTr("Connect to a daemon node to get started.")
                    font.family: FontIcons.display; font.pixelSize: 13; color: Theme.textMuted
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
            }

            // --- Phase: connect (the shared picker) ---
            ConnectionPicker {
                visible: root.phase === "connect"
                Layout.fillWidth: true
                showConnect: true
            }
            Text {
                visible: root.phase === "connect" && FirstRun && FirstRun.error.length > 0
                text: FirstRun ? FirstRun.error : ""
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.danger
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }

            // --- Phase: connecting (splash) ---
            ColumnLayout {
                visible: root.phase === "connecting"
                Layout.fillWidth: true
                spacing: 12
                Item { Layout.preferredHeight: 8 }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12
                    QQC.BusyIndicator { running: root.phase === "connecting"; implicitWidth: 28; implicitHeight: 28 }
                    Text {
                        text: qsTr("Connecting to %1...").arg(Connection ? Connection.target : "")
                        font.family: FontIcons.display; font.pixelSize: 14; color: Theme.text
                    }
                }
                Item { Layout.preferredHeight: 8 }
            }

            // --- Phase: inference gate (provider key + model pick) ---
            ColumnLayout {
                visible: root.phase === "inference"
                Layout.fillWidth: true
                spacing: 10

                SectionLabel { text: qsTr("Provider") }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Kit.Dropdown {
                        id: providerBox
                        Layout.preferredWidth: 150
                        model: (Accounts ? Accounts.availableProviders() : []).map(function(p) {
                            return p.name;
                        })
                    }
                    Kit.TextField {
                        id: keyField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Paste API key")
                        echoMode: TextInput.Password
                    }
                }
                Kit.TextButton {
                    text: qsTr("Connect provider")
                    enabled: keyField.text.length > 0
                    onClicked: {
                        var provs = Accounts ? Accounts.availableProviders() : [];
                        var pid = (providerBox.currentIndex >= 0
                                   && providerBox.currentIndex < provs.length)
                                  ? provs[providerBox.currentIndex].id : "anthropic";
                        Accounts.addApiKey(pid, "", keyField.text, "");
                        keyField.text = "";
                    }
                }

                SectionLabel { text: qsTr("Model") }
                Text {
                    text: (ModelCatalog && ModelCatalog.currentModelId.length > 0)
                          ? qsTr("Selected: %1").arg(ModelCatalog.currentModelId)
                          : qsTr("Discovering models…")
                    font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, 132)
                    clip: true
                    model: ModelCatalog ? ModelCatalog.installed : null
                    delegate: Rectangle {
                        required property var entry
                        width: ListView.view ? ListView.view.width : 0
                        height: 30
                        radius: 6
                        color: entry.active ? Theme.hover : "transparent"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            text: entry.name + "  \u00b7  " + entry.provider
                            font.family: FontIcons.display; font.pixelSize: 12; color: Theme.text
                        }
                        TapHandler { onTapped: ModelCatalog.activate(entry.id) }
                    }
                }
            }

            // --- Footer actions ---
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Kit.TextButton {
                    text: qsTr("Skip for now")
                    onClicked: FirstRun.skip()
                }
                Item { Layout.fillWidth: true }
                Kit.TextButton {
                    visible: root.phase === "inference"
                    text: qsTr("Finish setup")
                    accentFilled: true
                    // CON-7: only enabled once a usable model is reachable.
                    enabled: !FirstRun || FirstRun.inferenceReady
                    onClicked: FirstRun.completeInference()
                }
            }
        }
    }
}
