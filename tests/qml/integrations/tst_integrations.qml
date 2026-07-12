// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

import QtQuick
import QtTest
import DaemonApp.Sidebar

// [integrations wire v38] Work package AC (sidebar composition): the REAL IntegrationsTree.qml
// renderer, driven by the mock Transports / Persons seams injected as the app-global context
// properties. Guards that (a) the tree populates from the shared IntegrationsTreeModel, (b)
// activating a conversation row routes through to the component's conversationActivated signal
// (the seam the shell hands to the chat-tab dispatch), and (c) the schema-driven add-account flow
// hands off to the interactive AuthFlowSheet (the auth handoff A2 left dangling).
TestCase {
    id: tc
    name: "IntegrationsTree"
    width: 360
    height: 640
    // The tree's rows / self-sizing depend on a genuinely visible scene (TestCase defaults to
    // visible:false, which forces every child invisible regardless of its own bindings).
    visible: true
    when: windowShown

    IntegrationsTree {
        id: tree
        anchors.fill: parent
    }

    SignalSpy {
        id: convSpy
        target: tree
        signalName: "conversationActivated"
    }

    function findConversationRow(model) {
        for (var i = 0; i < model.rowCount(); ++i) {
            if (model.data(model.index(i, 0), IntegrationsTreeModel.RowKindRole) === "conversation")
                return i;
        }
        return -1;
    }

    // (a) The tree renders the mock account + its capability-governed subtree.
    function test_populated_from_mock_services() {
        var m = findChild(tree, "integrationsTreeModel");
        verify(m !== null, "the shared IntegrationsTreeModel is reachable");
        verify(m.rowCount() > 0, "tree populated from the mock Transports seam");
        verify(findConversationRow(m) >= 0, "at least one conversation row rendered");
    }

    // (b) Activating a conversation row emits the component's conversationActivated(transport, conv)
    // — the exact seam the GUI shell forwards to Session.openConversation (and the TUI to
    // openConversationTab).
    function test_conversation_activation_routes_to_signal() {
        var m = findChild(tree, "integrationsTreeModel");
        verify(m !== null);
        var row = findConversationRow(m);
        verify(row >= 0, "a conversation row exists");
        var t = m.data(m.index(row, 0), IntegrationsTreeModel.TransportRole);
        var c = m.data(m.index(row, 0), IntegrationsTreeModel.ConversationRole);
        convSpy.clear();
        m.activate(row);
        compare(convSpy.count, 1, "conversationActivated reached the component root");
        compare(convSpy.signalArguments[0][0], t, "carries the owning transport");
        compare(convSpy.signalArguments[0][1], c, "carries the conversation id");
    }

    // (c) The add-account flow hands off to the interactive auth sheet: submitAdd emits
    // authFlowRequested, which must open the mounted AuthFlowSheet (the dangling A2 handoff).
    function test_add_account_hands_off_to_auth_sheet() {
        var ctrl = findChild(tree, "accountManagementController");
        var sheet = findChild(tree, "integrationsAuthSheet");
        verify(ctrl !== null, "the account-management controller is reachable");
        verify(sheet !== null, "an AuthFlowSheet is mounted for the add-account handoff");
        verify(!sheet.visible, "the sheet starts closed");
        ctrl.beginAdd("matrix");
        ctrl.submitAdd({ "homeserver": "https://example.org" });
        tryVerify(function() { return sheet.visible; }, 2000,
                  "authFlowRequested opens the AuthFlowSheet");
    }
}
