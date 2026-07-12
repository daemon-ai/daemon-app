// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] Work package AC (sidebar composition): the shared contract that
// activating an IntegrationsTreeModel conversation row opens a native Chat tab for
// (transport, conversation). Both surfaces mount the SAME IntegrationsTreeModel and route its
// conversationActivated(transport, conv) into the chat-tab dispatch (GUI Session.openConversation,
// TUI rwdetail::openConversationTab -> TabModel::openConversation). Driving the real model against
// the real helper + TabModel guards that end-to-end seam.

#include "integrations_tree_model.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "root_widget_detail.h"
#include "tab_model.h"

#include <QSignalSpy>
#include <QtTest>

namespace {

// A minimal seeded MIRROR: one Matrix account with a channel + a DM (enough to yield
// conversation leaves the tree flattens under Channels / Direct Messages) — the same apply
// pipeline production feeds (§9; the legacy registry/persons fakes died with the read port).
void seedTree(mirror::MirrorService& svc) {
    mirror::SeedSet seed;
    mirror::Adapter matrix;
    matrix.family = QStringLiteral("matrix");
    matrix.display_name = QStringLiteral("Matrix");
    matrix.cap_rooms = true;
    matrix.cap_direct_messages = true;
    seed.adapters = {matrix};
    mirror::TransportAccount acct;
    acct.transport = QStringLiteral("matrix/@you");
    acct.family = QStringLiteral("matrix");
    acct.display_name = QStringLiteral("Matrix (@you)");
    acct.enabled = true;
    acct.reason = QStringLiteral("ready");
    seed.transportAccounts = {acct};
    mirror::Conversation general;
    general.transport = acct.transport;
    general.id = QStringLiteral("!general");
    general.title = QStringLiteral("general");
    general.kind = QStringLiteral("channel");
    mirror::Conversation dm;
    dm.transport = acct.transport;
    dm.id = QStringLiteral("!dm");
    dm.title = QStringLiteral("Alice");
    dm.kind = QStringLiteral("dm");
    seed.conversations = {general, dm};
    mirror::Seeder seeder(svc.store());
    seeder.seed(seed);
}

int findConversationRow(const IntegrationsTreeModel& model) {
    for (int i = 0; i < model.rowCount(); ++i) {
        if (model.data(model.index(i, 0), IntegrationsTreeModel::RowKindRole).toString() ==
            QStringLiteral("conversation")) {
            return i;
        }
    }
    return -1;
}

} // namespace

class TestIntegrationsActivation : public QObject {
    Q_OBJECT

private slots:
    // Activating a conversation row opens a Chat tab keyed by (transport, conversation) — the
    // deliberate replacement for the old Channels-page fallback (A4 dispatch), reached from the
    // integrations tree (work package AC).
    void conversationActivationOpensChatTab() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTree(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        TabModel tabs;
        // The production wiring both shells install: conversationActivated -> chat-tab dispatch.
        QObject::connect(&model, &IntegrationsTreeModel::conversationActivated, &tabs,
                         [&tabs](const QString& transport, const QString& conversation) {
                             rwdetail::openConversationTab(&tabs, transport, conversation,
                                                           QString());
                         });

        const int row = findConversationRow(model);
        QVERIFY(row >= 0);
        const QString transport =
            model.data(model.index(row, 0), IntegrationsTreeModel::TransportRole).toString();
        const QString conversation =
            model.data(model.index(row, 0), IntegrationsTreeModel::ConversationRole).toString();

        model.activate(row);

        QCOMPARE(tabs.count(), 1);
        QCOMPARE(tabs.kindAt(0), static_cast<int>(TabModel::Chat));
        QCOMPARE(tabs.transportAt(0), transport);
        QCOMPARE(tabs.conversationAt(0), conversation);
        // The rewire's whole point: NO Channels admin page was opened.
        for (int i = 0; i < tabs.count(); ++i) {
            QVERIFY(tabs.kindAt(i) != static_cast<int>(TabModel::Channels));
        }
    }

    // Re-activating the same conversation focuses its existing tab (find-or-create), never a
    // duplicate.
    void reactivatingConversationFocusesExistingTab() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTree(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        TabModel tabs;
        QObject::connect(&model, &IntegrationsTreeModel::conversationActivated, &tabs,
                         [&tabs](const QString& transport, const QString& conversation) {
                             rwdetail::openConversationTab(&tabs, transport, conversation,
                                                           QString());
                         });

        const int row = findConversationRow(model);
        QVERIFY(row >= 0);
        model.activate(row);
        model.activate(row);
        QCOMPARE(tabs.count(), 1); // reused, not duplicated
    }
};

QTEST_MAIN(TestIntegrationsActivation)
#include "tst_integrations_activation.moc"
