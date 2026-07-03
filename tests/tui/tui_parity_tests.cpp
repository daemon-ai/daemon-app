// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// ===========================================================================
// GUI <-> TUI parity guardrail.
//
// POLICY: the node decides, the apps render - and there are TWO renderers.
// Every TabModel::Kind, every CommandRegistry command id and every first-run
// phase ships with BOTH frontends (QML GUI + TUI), or is listed in an
// accepted-divergence table below WITH a one-line reason (and the wave that
// removes it, when temporary).
//
// When this test fails you either:
//   (a) added a TabModel kind / command id without a TUI route - implement the
//       TUI side and add it to tuiroutes::routedKinds() / handledCommands()
//       (src/tui/tui_parity_routes.cpp, next to the dispatch sites), or
//   (b) consciously accepted a divergence - add an exemption entry here, or
//   (c) removed/renamed a kind or command - delete the stale declaration or
//       exemption entry (stale entries fail too).
// ===========================================================================

#include "command_registry.h"
#include "tab_model.h"
#include "tui_parity_routes.h"

#include <QMetaEnum>
#include <QtTest>

namespace {

// Accepted TabModel::Kind divergences: kinds the TUI does not render yet.
QHash<int, QString> exemptKinds() {
    return {
        {TabModel::UsersAccess,
         QStringLiteral("Users & Access admin page is GUI-only; the Wave 1 "
                        "Users&Access workstream adds the TUI page and removes this")},
    };
}

// Accepted command-id divergences: registry ids without a (real) TUI route.
QHash<QString, QString> exemptCommands() {
    return {
        {QStringLiteral("access"),
         QStringLiteral("opens the Users & Access admin page (TUI page lands in "
                        "Wave 1 together with the UsersAccess tab kind)")},
        {QStringLiteral("usage"),
         QStringLiteral("permanent: usage/context is live in the status-bar "
                        "footer in BOTH shells; the command is a deliberate no-op")},
        {QStringLiteral("compress"),
         QStringLiteral("compaction is a simulated placeholder in BOTH frontends "
                        "(no real node capability yet); parity is trivially even")},
    };
}

} // namespace

class TuiParityTests : public QObject {
    Q_OBJECT

private slots:
    // Every TabModel::Kind is rendered by the TUI or explicitly exempted -
    // and never both (a routed kind with a leftover exemption is stale).
    void everyTabKindRoutedOrExempt() {
        const QMetaEnum meta = QMetaEnum::fromType<TabModel::Kind>();
        QVERIFY(meta.isValid());
        QVERIFY(meta.keyCount() > 0);

        QSet<int> known;
        for (int i = 0; i < meta.keyCount(); ++i) {
            known.insert(meta.value(i));
        }
        // Stale-declaration guard: both tables may only name real kinds.
        const QSet<int> routed = tuiroutes::routedKinds();
        for (int kind : routed) {
            QVERIFY2(known.contains(kind),
                     qPrintable(QStringLiteral("tuiroutes::routedKinds() names %1, which is not "
                                               "a TabModel::Kind (stale entry?)")
                                    .arg(kind)));
        }
        const QHash<int, QString> exempt = exemptKinds();
        for (auto it = exempt.cbegin(); it != exempt.cend(); ++it) {
            QVERIFY2(known.contains(it.key()),
                     qPrintable(QStringLiteral("exemptKinds() names %1, which is not a "
                                               "TabModel::Kind (stale exemption?)")
                                    .arg(it.key())));
        }

        for (int i = 0; i < meta.keyCount(); ++i) {
            const int kind = meta.value(i);
            const QString name = QString::fromLatin1(meta.key(i));
            const bool isRouted = routed.contains(kind);
            const bool isExempt = exempt.contains(kind);
            QVERIFY2(isRouted || isExempt,
                     qPrintable(QStringLiteral(
                                    "TabModel::%1 has no TUI route and no accepted-divergence "
                                    "entry. Ship the TUI rendering (then add it to "
                                    "tuiroutes::routedKinds in src/tui/tui_parity_routes.cpp) or "
                                    "add a reasoned exemption in this test.")
                                    .arg(name)));
            QVERIFY2(!(isRouted && isExempt),
                     qPrintable(QStringLiteral("TabModel::%1 is routed AND exempted - delete the "
                                               "stale exemption entry.")
                                    .arg(name)));
        }
    }

    // Every CommandRegistry id has a TUI route (slash funnel, manager-page
    // route or palette branch) or an explicit exemption - and never both.
    void everyCommandRoutedOrExempt() {
        CommandRegistry registry;
        // Grant every capability so gated entries (e.g. "access") are visible
        // to the parity sweep; the registry hides them fail-closed otherwise.
        registry.setCapabilityProvider([](const QString&) { return true; });
        registry.search(QString());

        QSet<QString> ids;
        for (const CommandRegistry::Command& c : registry.visibleCommands()) {
            ids.insert(c.id);
        }
        QVERIFY(!ids.isEmpty());
        // The permissive provider must actually unlock the gated catalog.
        QVERIFY(ids.contains(QStringLiteral("access")));

        // Stale-declaration guard: both tables may only name real command ids.
        const QSet<QString> handled = tuiroutes::handledCommands();
        for (const QString& id : handled) {
            QVERIFY2(ids.contains(id),
                     qPrintable(QStringLiteral("tuiroutes::handledCommands() names \"%1\", which "
                                               "is not a CommandRegistry id (stale entry?)")
                                    .arg(id)));
        }
        const QHash<QString, QString> exempt = exemptCommands();
        for (auto it = exempt.cbegin(); it != exempt.cend(); ++it) {
            QVERIFY2(ids.contains(it.key()),
                     qPrintable(QStringLiteral("exemptCommands() names \"%1\", which is not a "
                                               "CommandRegistry id (stale exemption?)")
                                    .arg(it.key())));
        }

        for (const QString& id : ids) {
            const bool isHandled = handled.contains(id);
            const bool isExempt = exempt.contains(id);
            QVERIFY2(isHandled || isExempt,
                     qPrintable(QStringLiteral(
                                    "Command \"%1\" has no TUI route and no accepted-divergence "
                                    "entry. Route it in the TUI (then add it to "
                                    "tuiroutes::handledCommands in "
                                    "src/tui/tui_parity_routes.cpp) or add a reasoned exemption "
                                    "in this test.")
                                    .arg(id)));
            QVERIFY2(!(isHandled && isExempt),
                     qPrintable(QStringLiteral("Command \"%1\" is routed AND exempted - delete "
                                               "the stale exemption entry.")
                                    .arg(id)));
        }
    }
};

QTEST_GUILESS_MAIN(TuiParityTests)
#include "tui_parity_tests.moc"
