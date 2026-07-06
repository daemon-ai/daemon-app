// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/iconnection_service.h"
#include "daemon/cached_session_store.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_dashboard.h"
#include "daemon/daemon_session_roster.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_fleet_tree.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CachedSessionRow;
using daemonapp::daemon::CachedSessionStore;
using daemonapp::daemon::DaemonCacheStore;
using fleet::DaemonDashboard;
using fleet::DaemonSessionRoster;

namespace {

// A trivial IConnectionService whose ready() the test can toggle (setState is protected).
class TestConnection : public connection::IConnectionService {
public:
    using IConnectionService::IConnectionService;
    void connectTo(const QString&, const QString&, const QString&) override {}
    void disconnect() override {}
    void testConnection(const QString&, const QString&, const QString&) override {}
    void becomeReady() { setState(QStringLiteral("ready")); }
};

CachedSessionRow session(const QString& id, const QString& title, const QString& state) {
    CachedSessionRow r;
    r.sessionId = id;
    r.title = title;
    r.state = state; // wire PascalCase: "Active" | "Ready" | "Suspended" | ...
    r.lifecycle = QStringLiteral("Live");
    r.updatedAtMs = 1;
    return r;
}

uimodels::VariantListModel* model(QObject* m) {
    return qobject_cast<uimodels::VariantListModel*>(m);
}

// The unit "Ok" ApiResponse (a bare CBOR text string), the reply SessionUpdateMeta returns.
QByteArray okResponse() {
    QByteArray b;
    daemonapp::test::cborText(b, "Ok");
    return b;
}

} // namespace

// DaemonSessionRoster + DaemonDashboard: offline-first roster from the cache, client-local
// suspend overlay + close->archive, and dashboard counters derived from
// roster/fleet/approvals/conn.
class TestDaemonRosterDashboard : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // Offline-first: a cached roster renders with NO connection, with wire-state lowercased.
    void rosterRendersCachedOffline() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("r1.db")));
        QVERIFY(cache.isOpen());
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active"))));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s2"), QStringLiteral("Idle one"), QStringLiteral("Ready"))));

        CachedSessionStore store(&cache, nullptr);
        DaemonSessionRoster roster(&store);
        auto* m = model(roster.sessions());
        QVERIFY(m != nullptr);
        QCOMPARE(roster.count(), 2);
        // Newest-updated first is the store's order; assert by id lookup instead of position.
        const int s1 = m->indexOfId(QStringLiteral("s1"));
        QVERIFY(s1 >= 0);
        QCOMPARE(m->at(s1).value(QStringLiteral("state")).toString(), QStringLiteral("active"));
        QCOMPARE(m->at(s1).value(QStringLiteral("lifecycle")).toString(), QStringLiteral("live"));
        const int s2 = m->indexOfId(QStringLiteral("s2"));
        QCOMPARE(m->at(s2).value(QStringLiteral("state")).toString(), QStringLiteral("idle"));
    }

    // suspend overlay -> "suspended"; resume reverts to the wire state.
    void suspendResumeOverlay() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("r2.db")));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active"))));
        CachedSessionStore store(&cache, nullptr);
        DaemonSessionRoster roster(&store);
        auto* m = model(roster.sessions());

        roster.suspend(QStringLiteral("s1"));
        QCOMPARE(
            m->at(m->indexOfId(QStringLiteral("s1"))).value(QStringLiteral("state")).toString(),
            QStringLiteral("suspended"));
        roster.resume(QStringLiteral("s1"));
        QCOMPARE(
            m->at(m->indexOfId(QStringLiteral("s1"))).value(QStringLiteral("state")).toString(),
            QStringLiteral("active"));
    }

    // close -> archive is NODE-AUTHORITATIVE (Phase 5): it sends a SessionUpdateMeta{archived:true}
    // intent through the store's SessionRepository rather than mutating the cache locally. The row
    // leaves the roster only once the node's authoritative refetch prunes it - never
    // optimistically.
    void closeSendsArchiveIntent() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("roster-close.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(path), "listen");

        DaemonCacheStore cache(dir.filePath(QStringLiteral("r3.db")));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active"))));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s2"), QStringLiteral("Two"), QStringLiteral("Active"))));

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::SessionRepository sessions(&client, &cache);
        CachedSessionStore store(&cache, &sessions);
        DaemonSessionRoster roster(&store);
        QCOMPARE(roster.count(), 2);

        roster.close(QStringLiteral("s1"));

        // The archive went out as a node intent (SessionUpdateMeta carrying the archived patch for
        // s1), NOT a local cache write.
        QTRY_VERIFY_WITH_TIMEOUT(!fake.callPayloads().isEmpty(), 3000);
        const QByteArray first = fake.callPayloads().first();
        QVERIFY(first.contains("SessionUpdateMeta"));
        QVERIFY(first.contains("s1"));
        QVERIFY(first.contains("archived"));
    }

    // Dashboard counters derive from the roster + approvals + connection, and react to changes.
    void dashboardDerivesCounters() {
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("d1.db")));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active"))));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s2"), QStringLiteral("Idle"), QStringLiteral("Ready"))));
        CachedSessionStore store(&cache, nullptr);
        DaemonSessionRoster roster(&store);
        fleet::MockFleetTree fleetTree(nullptr); // empty fleet
        fleet::MockApprovalsInbox approvals;
        TestConnection conn;
        DaemonDashboard dash(&roster, &fleetTree, &approvals, &conn);

        QCOMPARE(dash.activeSessions(), 1); // only s1 is "active"
        QCOMPARE(dash.pendingApprovals(), approvals.count());
        QVERIFY(!dash.healthy());                          // not connected yet
        QCOMPARE(dash.tokensToday(), QStringLiteral("-")); // degraded placeholder

        QSignalSpy changed(&dash, &fleet::IDashboard::changed);
        conn.becomeReady();
        QVERIFY(dash.healthy());
        QVERIFY(changed.count() >= 1);

        // Suspending the only active session drops the active count (overlay -> not "active").
        roster.suspend(QStringLiteral("s1"));
        QCOMPARE(dash.activeSessions(), 0);
    }
};

QTEST_GUILESS_MAIN(TestDaemonRosterDashboard)
#include "tst_daemon_roster_dashboard.moc"
