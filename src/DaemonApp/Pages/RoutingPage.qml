import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import DaemonApp.Theme
import DaemonApp.Controls as Kit

// Routing manager (mock). Vertical layout: a (deferred) topology slot on top, and below it the
// control surface - a primary "Routes" card list (the pins: origin -> session + agent) with an
// add/edit dialog, plus a Delivery/handover panel for the selected route's session. The rule /
// account / default precedence is surfaced only as the read-only provenance chip on each route.
// Backed by the shared DaemonNet via RoutingManagerController. See docs/routing-manager-design.md.
Item {
    id: root

    RoutingManagerController {
        id: rm
        daemonNet: typeof DaemonNet !== "undefined" ? DaemonNet : null
    }

    // The selected route (by origin key); drives the Delivery panel + the highlight.
    property string selectedRouteId: ""

    function provenanceColor(p) {
        switch (p) {
        case "pin": return Theme.accent;
        case "rule": return Theme.warning;
        case "account": return Theme.statusOk;
        default: return Theme.textMuted;
        }
    }

    PageHeader {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        title: qsTr("Routing")
        icon: FontIcons.fa_sitemap
    }

    ColumnLayout {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        spacing: 12

        // --- Topology: force-directed DaemonNet view (above the control surface) ---
        RoutingTopology {
            id: topology
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(220, root.height * 0.4)
            // A session node tap selects that route's session in the control below.
            onSessionActivated: function(sessionId) {
                root.selectedRouteId = "";
                rm.selectedSession = sessionId;
            }
            // Drag an origin onto a session -> pin it.
            onPinRequested: function(originKey, sessionId) {
                rm.pin(originKey, sessionId, "");
            }
        }

        // Control -> topology selection sync (selecting a route highlights its node).
        Connections {
            target: rm
            function onSelectedSessionChanged() { topology.highlight(rm.selectedSession); }
        }

        // --- Routes (primary) -------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            SectionLabel { text: qsTr("Routes"); Layout.fillWidth: true }
            Kit.TextButton {
                text: qsTr("+ Add route")
                accentFilled: true
                onClicked: routeDialog.openFor(null)
            }
        }

        QQC.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ListView {
                model: rm.routes
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds

                delegate: Rectangle {
                    id: card
                    required property var entry
                    width: ListView.view.width
                    height: 64
                    radius: 8
                    color: Theme.surface
                    border.width: root.selectedRouteId === entry.id ? 2 : 1
                    border.color: root.selectedRouteId === entry.id ? Theme.accent : Theme.border

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.selectedRouteId = card.entry.id;
                            rm.selectedSession = card.entry.session;
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 10

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: card.entry.transport + "  \u00b7  " + card.entry.scope
                                font.family: FontIcons.display; font.pixelSize: 14
                                font.bold: true; color: Theme.text
                                elide: Text.ElideRight; Layout.fillWidth: true
                            }
                            RowLayout {
                                spacing: 8
                                Text {
                                    text: "\u2192 " + card.entry.session + "  \u00b7  "
                                          + (card.entry.profile !== "" ? card.entry.profile
                                                                       : qsTr("(inherit)"))
                                    font.family: FontIcons.mono; font.pixelSize: 11; color: Theme.textMuted
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                                // Provenance chip (the read-only "why it resolves" rung).
                                Rectangle {
                                    radius: 8
                                    implicitWidth: chipText.implicitWidth + 14
                                    implicitHeight: 16
                                    color: "transparent"
                                    border.width: 1
                                    border.color: root.provenanceColor(card.entry.decidedBy)
                                    Text {
                                        id: chipText
                                        anchors.centerIn: parent
                                        text: card.entry.decidedBy
                                        font.family: FontIcons.display; font.pixelSize: 9
                                        color: root.provenanceColor(card.entry.decidedBy)
                                    }
                                }
                            }
                        }

                        Kit.IconButton {
                            icon: FontIcons.fa_gear; iconPointSize: 12
                            implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Edit route")
                            onClicked: routeDialog.openFor(card.entry)
                        }
                        Kit.IconButton {
                            icon: FontIcons.fa_trash; iconColor: Theme.danger
                            iconPointSize: 12; implicitWidth: 30; implicitHeight: 26
                            tooltipText: qsTr("Unpin")
                            onClicked: rm.unbind(card.entry.id)
                        }
                    }
                }
            }
        }

        // --- Delivery / handover (selected route's session) -------------------
        SectionLabel {
            text: rm.selectedSession !== "" ? qsTr("Delivery \u00b7 ") + rm.selectedSession
                                            : qsTr("Delivery")
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 132
            radius: 8
            color: Theme.surface
            border.color: Theme.border

            Text {
                anchors.centerIn: parent
                visible: rm.selectedSession === "" || rm.delivery.count === 0
                text: rm.selectedSession === "" ? qsTr("Select a route to see where its replies go")
                                                : qsTr("No delivery targets")
                font.family: FontIcons.display; font.pixelSize: 12; color: Theme.textMuted
            }

            ListView {
                anchors.fill: parent
                anchors.margins: 10
                clip: true
                visible: rm.selectedSession !== "" && rm.delivery.count > 0
                model: rm.delivery
                spacing: 4
                boundsBehavior: Flickable.StopAtBounds
                QQC.ScrollBar.vertical: Kit.ScrollBar {}

                delegate: RowLayout {
                    required property var entry
                    width: ListView.view.width
                    spacing: 8
                    Text {
                        text: (entry.kind === "primary" ? "\u25cf  " : "\u25cb  ")
                              + entry.transport + "  \u00b7  " + entry.route
                        font.family: FontIcons.display; font.pixelSize: 12
                        font.weight: entry.kind === "primary" ? Font.DemiBold : Font.Normal
                        color: entry.kind === "primary" ? Theme.text : Theme.textMuted
                        elide: Text.ElideRight; Layout.fillWidth: true
                    }
                    Text {
                        visible: entry.kind === "primary"
                        text: qsTr("PRIMARY")
                        font.family: FontIcons.display; font.pixelSize: 9; color: Theme.accent
                    }
                    Kit.TextButton {
                        visible: entry.kind === "spectator"
                        text: qsTr("Make primary")
                        onClicked: rm.handoverTo(rm.selectedSession, entry.transport, entry.route)
                    }
                }
            }
        }
    }

    RouteDialog {
        id: routeDialog
        controller: rm
    }
}
