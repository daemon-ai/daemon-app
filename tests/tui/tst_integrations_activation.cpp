// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] Work package AC (sidebar composition): the shared contract that
// activating an IntegrationsTreeModel conversation row opens a native Chat tab for
// (transport, conversation). Both surfaces mount the SAME IntegrationsTreeModel and route its
// conversationActivated(transport, conv) into the chat-tab dispatch (GUI Session.openConversation,
// TUI rwdetail::openConversationTab -> TabModel::openConversation). Driving the real model against
// the real helper + TabModel guards that end-to-end seam.

#include "integrations_tree_model.h"
#include "root_widget_detail.h"
#include "tab_model.h"
#include "transports/ipersons_service.h"
#include "transports/itransport_registry.h"

#include <QSignalSpy>
#include <QtTest>

using transports::IPersonsService;
using transports::ITransportRegistry;

namespace {

QVariantMap conv(const QString& transport, const QString& id, const QString& title,
                 const QString& kind, const QString& parent) {
    return {{QStringLiteral("transport"), transport},
            {QStringLiteral("id"), id},
            {QStringLiteral("title"), title},
            {QStringLiteral("kind"), kind},
            {QStringLiteral("parent"), parent}};
}

// A minimal registry: one Matrix account with a channel + a DM (enough to yield conversation
// leaves the tree flattens under Channels / Direct Messages).
class FakeRegistry : public ITransportRegistry {
public:
    using ITransportRegistry::ITransportRegistry;
    [[nodiscard]] QVariantList availableAdapters() const override {
        QVariantMap caps{{QStringLiteral("rooms"), true}, {QStringLiteral("directMessages"), true}};
        return {QVariantMap{{QStringLiteral("family"), QStringLiteral("matrix")},
                            {QStringLiteral("displayName"), QStringLiteral("Matrix")},
                            {QStringLiteral("capabilities"), caps},
                            {QStringLiteral("schema"), QVariantList{}}}};
    }
    [[nodiscard]] QVariantList instances() const override {
        return {QVariantMap{{QStringLiteral("transport"), QStringLiteral("matrix/@you")},
                            {QStringLiteral("family"), QStringLiteral("matrix")},
                            {QStringLiteral("displayName"), QStringLiteral("Matrix (@you)")},
                            {QStringLiteral("enabled"), true},
                            {QStringLiteral("connectionReason"), QStringLiteral("ready")}}};
    }
    [[nodiscard]] QVariantList conversations(const QString& transport) const override {
        if (transport != QStringLiteral("matrix/@you")) {
            return {};
        }
        return {conv(transport, QStringLiteral("!general"), QStringLiteral("general"),
                     QStringLiteral("channel"), QString()),
                conv(transport, QStringLiteral("!dm"), QStringLiteral("Alice"),
                     QStringLiteral("dm"), QString())};
    }
};

class FakePersons : public IPersonsService {
public:
    using IPersonsService::IPersonsService;
    [[nodiscard]] QVariantList persons() const override { return {}; }
};

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
        FakeRegistry reg;
        FakePersons persons;
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
