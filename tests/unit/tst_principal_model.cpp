// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/node_api_codec.h"
#include "daemon/principal_model.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using daemonapp::daemon::DecodedPrincipalView;
using daemonapp::daemon::PrincipalModel;

// The WhoAmI / principal model that drives advisory capability gating. The gate is fail-closed:
// no capability is granted unless an authenticated principal explicitly carries it. (The node still
// enforces server-side; this only hides/shows UI.)
class TestPrincipalModel : public QObject {
    Q_OBJECT

private slots:
    // Default (unauthenticated): no capabilities, no roles, fail-closed.
    void unauthenticatedGrantsNothing() {
        PrincipalModel m;
        QVERIFY(!m.authenticated());
        QVERIFY(!m.hasCapability(QStringLiteral("access_admin")));
        QVERIFY(!m.hasCapability(QStringLiteral("session_read")));
        QVERIFY(!m.hasRole(QStringLiteral("admin")));
    }

    // An admin principal grants its capabilities (incl. access_admin -> the admin page).
    void adminGrantsAccessAdmin() {
        PrincipalModel m;
        QSignalSpy changed(&m, &PrincipalModel::changed);
        DecodedPrincipalView v;
        v.userId = QStringLiteral("uid-admin");
        v.username = QStringLiteral("root");
        v.roles = {QStringLiteral("admin")};
        v.capabilities = {QStringLiteral("session_read"), QStringLiteral("session_write"),
                          QStringLiteral("access_admin")};
        m.setPrincipal(v);

        QCOMPARE(changed.count(), 1);
        QVERIFY(m.authenticated());
        QCOMPARE(m.username(), QStringLiteral("root"));
        QVERIFY(m.hasRole(QStringLiteral("admin")));
        QVERIFY(m.hasCapability(QStringLiteral("access_admin")));
        QVERIFY(m.hasCapability(QStringLiteral("session_write")));
    }

    // A Viewer (read-only) must NOT be granted the admin page or any write capability.
    void viewerSeesNoAdminPage() {
        PrincipalModel m;
        DecodedPrincipalView v;
        v.userId = QStringLiteral("uid-viewer");
        v.username = QStringLiteral("guest");
        v.roles = {QStringLiteral("viewer")};
        v.capabilities = {QStringLiteral("session_read"), QStringLiteral("fleet_read"),
                          QStringLiteral("profile_read")};
        m.setPrincipal(v);

        QVERIFY(m.authenticated());
        QVERIFY(m.hasCapability(QStringLiteral("session_read")));
        QVERIFY(!m.hasCapability(QStringLiteral("access_admin"))); // no admin page
        QVERIFY(!m.hasCapability(QStringLiteral("session_write")));
        QVERIFY(!m.hasRole(QStringLiteral("admin")));
    }

    // Logout/disconnect clears the principal so gated surfaces re-gate closed.
    void clearReGatesClosed() {
        PrincipalModel m;
        DecodedPrincipalView v;
        v.userId = QStringLiteral("uid-admin");
        v.username = QStringLiteral("root");
        v.capabilities = {QStringLiteral("access_admin")};
        m.setPrincipal(v);
        QVERIFY(m.hasCapability(QStringLiteral("access_admin")));

        QSignalSpy changed(&m, &PrincipalModel::changed);
        m.clear();
        QCOMPARE(changed.count(), 1);
        QVERIFY(!m.authenticated());
        QVERIFY(!m.hasCapability(QStringLiteral("access_admin")));
        QVERIFY(m.username().isEmpty());
    }
};

QTEST_MAIN(TestPrincipalModel)
#include "tst_principal_model.moc"
