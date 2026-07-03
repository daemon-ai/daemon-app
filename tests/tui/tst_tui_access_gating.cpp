// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// TUI capability gating for the Users & Access admin surface (auth6, Wave 1).
//
// Proves the two TUI gates mirror the GUI:
//  - the command palette: the "access" entry is hidden fail-closed until the
//    wired principal grants access_admin, and re-hides on logout (the same
//    CommandRegistry::setCapabilityProvider wiring RootWidget installs);
//  - the page route: TuiPageHub::openManagerPage("access") consumes the command
//    but never mounts the UsersAccess page without access_admin (Main.qml /
//    Session.qml parity), and mounts the singleton page tab with it.
// The node enforces every capability server-side regardless; this is UI-only.

#include "command_registry.h"
#include "daemon/node_api_codec.h"
#include "daemon/principal_model.h"
#include "tab_model.h"
#include "tui_page_hub.h"

#include <algorithm>
#include <QtTest>

using daemonapp::daemon::DecodedPrincipalView;
using daemonapp::daemon::PrincipalModel;

namespace {

DecodedPrincipalView adminView() {
    DecodedPrincipalView v;
    v.userId = QStringLiteral("uid-admin");
    v.username = QStringLiteral("root");
    v.roles = {QStringLiteral("admin")};
    v.capabilities = {QStringLiteral("session_read"), QStringLiteral("session_write"),
                      QStringLiteral("access_admin")};
    return v;
}

DecodedPrincipalView viewerView() {
    DecodedPrincipalView v;
    v.userId = QStringLiteral("uid-viewer");
    v.username = QStringLiteral("guest");
    v.roles = {QStringLiteral("viewer")};
    v.capabilities = {QStringLiteral("session_read")};
    return v;
}

bool commandVisible(const CommandRegistry& registry, const QString& id) {
    const QVector<CommandRegistry::Command> visible = registry.visibleCommands();
    return std::ranges::any_of(visible,
                               [&id](const CommandRegistry::Command& c) { return c.id == id; });
}

} // namespace

class TuiAccessGatingTests : public QObject {
    Q_OBJECT

private slots:
    // The palette wiring RootWidget installs: provider bound to the principal,
    // re-applied on its changed() signal. "access" tracks login/logout.
    void paletteEntryTracksPrincipal() {
        CommandRegistry registry;
        // Fail-closed before any provider is set.
        QVERIFY(!commandVisible(registry, QStringLiteral("access")));

        PrincipalModel principal;
        auto applyCaps = [&registry, &principal] {
            registry.setCapabilityProvider(
                [&principal](const QString& cap) { return principal.hasCapability(cap); });
        };
        applyCaps();
        connect(&principal, &PrincipalModel::changed, &registry, applyCaps);

        // Unauthenticated principal: still hidden.
        QVERIFY(!commandVisible(registry, QStringLiteral("access")));

        // An admin (access_admin) logs in: the entry appears via the changed() re-apply.
        principal.setPrincipal(adminView());
        QVERIFY(commandVisible(registry, QStringLiteral("access")));

        // Logout re-gates closed.
        principal.clear();
        QVERIFY(!commandVisible(registry, QStringLiteral("access")));

        // A viewer without access_admin never sees the entry.
        principal.setPrincipal(viewerView());
        QVERIFY(!commandVisible(registry, QStringLiteral("access")));
        // Ungated entries stay visible throughout.
        QVERIFY(commandVisible(registry, QStringLiteral("settings")));
    }

    // The page route: "access" is consumed but mounts nothing without the
    // capability, and opens the singleton UsersAccess page tab with it.
    void openManagerPageGatesOnCapability() {
        TabModel tabs;
        PrincipalModel principal;
        TuiPageHub::Dependencies deps;
        deps.tabModel = &tabs;
        deps.principal = &principal;
        const TuiPageHub hub(deps);

        // Unauthenticated: consumed (a GUI Main.qml "case access" no-op), no tab.
        QVERIFY(hub.openManagerPage(QStringLiteral("access")));
        QCOMPARE(tabs.count(), 0);

        // A viewer without access_admin: still consumed, still no tab.
        principal.setPrincipal(viewerView());
        QVERIFY(hub.openManagerPage(QStringLiteral("access")));
        QCOMPARE(tabs.count(), 0);

        // An admin: the UsersAccess page tab mounts (find-or-create singleton).
        principal.setPrincipal(adminView());
        QVERIFY(hub.openManagerPage(QStringLiteral("access")));
        QCOMPARE(tabs.count(), 1);
        QCOMPARE(tabs.kindAt(0), static_cast<int>(TabModel::UsersAccess));
        QVERIFY(hub.openManagerPage(QStringLiteral("access")));
        QCOMPARE(tabs.count(), 1); // re-activated, not duplicated

        // Unknown ids are not manager pages (fall through to other handlers).
        QVERIFY(!hub.openManagerPage(QStringLiteral("not-a-page")));
    }

    // The page projection mirrors the GUI's UsersAccessPage.qml: live WhoAmI
    // panel + the pending user-administration scaffold.
    void usersAccessMarkdownMirrorsWhoAmI() {
        TabModel tabs;
        PrincipalModel principal;
        TuiPageHub::Dependencies deps;
        deps.tabModel = &tabs;
        deps.principal = &principal;
        const TuiPageHub hub(deps);

        // Unauthenticated: the WhoAmI panel says so, with no roles/capabilities.
        QString md = hub.pageMarkdownForKind(TabModel::UsersAccess);
        QVERIFY(md.contains(QStringLiteral("Users & Access")));
        QVERIFY(md.contains(QStringLiteral("Not authenticated")));
        QVERIFY(md.contains(QStringLiteral("(none)")));
        QVERIFY(md.contains(QStringLiteral("User administration is not available yet.")));

        // Authenticated: username, user id, roles and capabilities are live.
        principal.setPrincipal(adminView());
        md = hub.pageMarkdownForKind(TabModel::UsersAccess);
        QVERIFY(md.contains(QStringLiteral("root")));
        QVERIFY(md.contains(QStringLiteral("uid-admin")));
        QVERIFY(md.contains(QStringLiteral("admin")));
        QVERIFY(md.contains(QStringLiteral("access_admin")));
        QVERIFY(!md.contains(QStringLiteral("Not authenticated")));
        // The user-CRUD section stays pending (node access-admin API, Auth 5).
        QVERIFY(md.contains(QStringLiteral("User administration is not available yet.")));
    }
};

QTEST_GUILESS_MAIN(TuiAccessGatingTests)
#include "tst_tui_access_gating.moc"
