// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import DaemonApp.Sidebar

// [integrations wire v38] Work package AC (sidebar composition): the REAL Sidebar.qml, driven by
// the mock context-property seams. Guards the placement decision A2 deferred:
//   - the new IntegrationsTree is MOUNTED in the sidebar and populated from the Transports seam;
//   - activating one of its conversation rows propagates to the Sidebar's conversationActivated
//     signal (the seam the shell forwards to the chat-tab dispatch);
//   - the LEGACY transportsTree() "Integrations" section is NO LONGER composed into the fleet
//     SidebarModel, even though a demo DaemonNet is present in context (so the two would otherwise
//     render a duplicate "Integrations" section). The transportsTree() projection itself is
//     retained on IDaemonNet for its other consumer (RoutingManagerController); only the sidebar
//     rendering is retired — the new IntegrationsTree owns the surface.
TestCase {
    id: tc
    name: "SidebarComposition"
    width: 300
    height: 720
    visible: true
    when: windowShown

    Sidebar {
        id: sidebar
        anchors.fill: parent
    }

    SignalSpy {
        id: convSpy
        target: sidebar
        signalName: "conversationActivated"
    }

    function findConversationRow(model) {
        for (var i = 0; i < model.rowCount(); ++i) {
            if (model.data(model.index(i, 0), IntegrationsTreeModel.RowKindRole) === "conversation")
                return i;
        }
        return -1;
    }

    // The IntegrationsTree is mounted in the sidebar and populated from the mock Transports seam.
    function test_integrations_tree_mounted_and_populated() {
        var m = findChild(sidebar, "integrationsTreeModel");
        verify(m !== null, "the sidebar composes an IntegrationsTree");
        verify(m.rowCount() > 0, "the mounted tree is populated from the Transports seam");
    }

    // Activating a conversation row in the mounted tree propagates to the Sidebar's
    // conversationActivated signal (routed by the shell to the chat-tab dispatch).
    function test_conversation_activation_propagates_to_sidebar() {
        var m = findChild(sidebar, "integrationsTreeModel");
        verify(m !== null);
        var row = findConversationRow(m);
        verify(row >= 0, "a conversation row exists in the mounted tree");
        var t = m.data(m.index(row, 0), IntegrationsTreeModel.TransportRole);
        var c = m.data(m.index(row, 0), IntegrationsTreeModel.ConversationRole);
        convSpy.clear();
        m.activate(row);
        compare(convSpy.count, 1, "the sidebar re-emits conversationActivated");
        compare(convSpy.signalArguments[0][0], t);
        compare(convSpy.signalArguments[0][1], c);
    }

    // The legacy transportsTree() section is NOT composed into the fleet model: even with a demo
    // DaemonNet in context, the fleet SidebarModel has no expanded transport section.
    function test_legacy_transports_section_not_composed() {
        var s = findChild(sidebar, "fleetSidebarModel");
        verify(s !== null, "the fleet SidebarModel is reachable");
        verify(!s.anyTransportExpanded,
               "the legacy transportsTree() Integrations section is no longer mounted");
    }
}
