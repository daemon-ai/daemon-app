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

    // F6/DEL-6: the Active|Archived scope switcher re-projects the roster (archived rows only,
    // with the fetch going out as a SessionScope::Archived query), and restore() sends the
    // SessionUpdateMeta{archived:false} intent.
    void archivedScopeAndRestore() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("roster-arch.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(path), "listen");

        DaemonCacheStore cache(dir.filePath(QStringLiteral("r4.db")));
        QVERIFY(cache.upsertSession(
            session(QStringLiteral("s1"), QStringLiteral("Live one"), QStringLiteral("Active"))));
        CachedSessionRow archivedRow =
            session(QStringLiteral("s2"), QStringLiteral("Old child"), QStringLiteral("Ready"));
        archivedRow.archived = true;
        QVERIFY(cache.upsertSession(archivedRow));

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::SessionRepository sessions(&client, &cache);
        CachedSessionStore store(&cache, &sessions);
        DaemonSessionRoster roster(&store, &sessions);

        // Default (active) scope: the archived row is filtered out.
        QCOMPARE(roster.count(), 1);
        QCOMPARE(roster.scope(), QStringLiteral("active"));

        QSignalSpy scoped(&roster, &fleet::ISessionRoster::scopeChanged);
        roster.setScope(QStringLiteral("archived"));
        QCOMPARE(scoped.count(), 1);
        auto* m = model(roster.sessions());
        QCOMPARE(roster.count(), 1);
        QCOMPARE(m->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("s2"));

        // Entering the scope fetched the authoritative Archived listing: byte-identical to the
        // encoder's Archived-scope query.
        QTRY_VERIFY_WITH_TIMEOUT(!fake.callPayloads().isEmpty(), 3000);
        QCOMPARE(fake.callPayloads().first(),
                 daemonapp::daemon::NodeApiCodec::encodeSessionsQueryRequest(
                     /*hasSinceRev=*/false, /*sinceRev=*/0, /*byProfile=*/QString(),
                     /*after=*/QString(), /*archivedScope=*/true));

        // Restore goes out as the archived:false meta patch (node-authoritative un-archive).
        roster.restore(QStringLiteral("s2"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.callPayloads().size() >= 2, 3000);
        const QByteArray patch = fake.callPayloads().at(1);
        QVERIFY(patch.contains("SessionUpdateMeta"));
        QVERIFY(patch.contains("s2"));
        QVERIFY(patch.contains("archived"));
    }

    // F4/DEL-4: the operator steer/startTurn/interrupt row actions submit to the TARGET session
    // id (byte-identical to the Submit encoders), and a node Error surfaces via operationFailed.
    void steerStartTurnInterruptSubmitToSession() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("roster-steer.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(path), "listen");

        DaemonCacheStore cache(dir.filePath(QStringLiteral("r5.db")));
        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::SessionRepository sessions(&client, &cache);
        CachedSessionStore store(&cache, &sessions);
        DaemonSessionRoster roster(&store, &sessions);

        roster.steer(QStringLiteral("child-1"), QStringLiteral("focus on the tests"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.callPayloads().size() >= 1, 3000);
        QCOMPARE(fake.callPayloads().at(0),
                 daemonapp::daemon::NodeApiCodec::encodeSubmitSteerRequest(
                     QStringLiteral("child-1"), QStringLiteral("focus on the tests")));

        roster.startTurn(QStringLiteral("child-2"), QStringLiteral("begin the review"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.callPayloads().size() >= 2, 3000);
        QCOMPARE(fake.callPayloads().at(1),
                 daemonapp::daemon::NodeApiCodec::encodeSubmitStartTurnRequest(
                     QStringLiteral("child-2"), QStringLiteral("begin the review")));

        roster.interrupt(QStringLiteral("child-1"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.callPayloads().size() >= 3, 3000);
        QCOMPARE(fake.callPayloads().at(2),
                 daemonapp::daemon::NodeApiCodec::encodeSubmitInterruptRequest(
                     QStringLiteral("child-1")));

        // A node rejection (Error) surfaces through the roster's operationFailed - never silent.
        QByteArray err;
        err.append(static_cast<char>(0xA1));
        daemonapp::test::cborText(err, "Error");
        err.append(static_cast<char>(0xA1));
        daemonapp::test::cborText(err, "Forbidden");
        daemonapp::test::cborText(err, "not your session");
        fake.setReplyPayload(err);
        QSignalSpy failed(&roster, &fleet::ISessionRoster::operationFailed);
        roster.interrupt(QStringLiteral("child-9"));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("not your session"));
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
