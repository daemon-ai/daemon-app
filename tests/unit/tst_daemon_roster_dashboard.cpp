// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/iconnection_service.h"
#include "daemon/daemon_dashboard.h"
#include "daemon/daemon_session_roster.h"
#include "daemon/daemon_transport.h"
#include "daemon/mirror_session_store.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "fleet/ifleet_tree.h"
#include "fleet/mock_approvals_inbox.h"
#include "local_database.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "outbox.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <memory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::MirrorSessionStore;
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

// A recording FetchExecutor: the roster's scoped refreshes are ingestor FETCH intents now (the
// wire encode itself is the DaemonFetchExecutor's, covered by its own suite); the recording
// proves the §5.4 job (op + scope) the store schedules.
class RecordingExecutor final : public mirror::FetchExecutor {
public:
    void execute(const mirror::FetchJob& job) override { jobs.append(job); }
    QList<mirror::FetchJob> jobs;
};

// An empty IFleetTree stand-in for the dashboard counter tests (no fleet rows).
class EmptyFleetTree : public fleet::IFleetTree {
public:
    EmptyFleetTree() : m_model(new uimodels::VariantListModel(this)) {}
    [[nodiscard]] QObject* nodes() const override { return m_model; }
    void pause(const QString&) override {}
    void resume(const QString&) override {}

private:
    uimodels::VariantListModel* m_model = nullptr;
};

mirror::Session sess(const QString& id, const QString& title, const QString& state,
                     bool archived = false) {
    mirror::Session s;
    s.session = id;
    s.title = title;
    s.state = state; // canonical PascalCase: "Active" | "Ready" | "Suspended" | ...
    s.lifecycle = QStringLiteral("Live");
    s.archived = archived;
    return s;
}

uimodels::VariantListModel* model(QObject* m) {
    return qobject_cast<uimodels::VariantListModel*>(m);
}

// The mirror fixture every case builds on: an in-memory mirror + the production store.
struct Fixture {
    mirror::MirrorService svc;
    RecordingExecutor executor;
    std::unique_ptr<MirrorSessionStore> store;
    Fixture() {
        svc.openInMemory();
        svc.setFetchExecutor(&executor);
        store = std::make_unique<MirrorSessionStore>(&svc.store(), &svc.ingestor());
    }
    void seed(const std::vector<mirror::Session>& rows) {
        mirror::Seeder seeder(svc.store());
        for (const mirror::Session& s : rows) {
            seeder.upsertSession(s);
        }
    }
};

} // namespace

// DaemonSessionRoster + DaemonDashboard over the MIRROR store (AD: the CachedSessionStore
// fixtures died with the class): roster projection + lowercased states, the client-local
// suspend overlay, close/restore as session-meta LANE intents (never a local flip), the
// archived/ByTransport scopes as ingestor fetch intents + node-resolved projections, operator
// submit over the surviving SessionRepository seam, and the dashboard counter derivation.
class TestDaemonRosterDashboard : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // The roster renders the mirror rows (offline: whatever the persisted mirror reloaded),
    // with wire-state lowercased.
    void rosterRendersMirrorRows() {
        Fixture fx;
        fx.seed({sess(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active")),
                 sess(QStringLiteral("s2"), QStringLiteral("Idle one"), QStringLiteral("Ready"))});
        DaemonSessionRoster roster(fx.store.get());
        auto* m = model(roster.sessions());
        QVERIFY(m != nullptr);
        QCOMPARE(roster.count(), 2);
        const int s1 = m->indexOfId(QStringLiteral("s1"));
        QVERIFY(s1 >= 0);
        QCOMPARE(m->at(s1).value(QStringLiteral("state")).toString(), QStringLiteral("active"));
        QCOMPARE(m->at(s1).value(QStringLiteral("lifecycle")).toString(), QStringLiteral("live"));
        const int s2 = m->indexOfId(QStringLiteral("s2"));
        QCOMPARE(m->at(s2).value(QStringLiteral("state")).toString(), QStringLiteral("idle"));
    }

    // suspend overlay -> "suspended"; resume reverts to the wire state.
    void suspendResumeOverlay() {
        Fixture fx;
        fx.seed({sess(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active"))});
        DaemonSessionRoster roster(fx.store.get());
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

    // close -> archive is NODE-AUTHORITATIVE: a durable SessionUpdateMeta{archived:true} op in
    // the session-meta lane (§6.4), never a local mirror write — the row leaves the roster only
    // when the node's echo lands.
    void closeEnqueuesArchiveIntent() {
        Fixture fx;
        fx.seed({sess(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active")),
                 sess(QStringLiteral("s2"), QStringLiteral("Two"), QStringLiteral("Active"))});
        mirror::LocalDatabase db(QStringLiteral(":memory:"));
        mirror::Outbox outbox(&db);
        outbox.load();
        fx.store->setOutbox(&outbox);
        DaemonSessionRoster roster(fx.store.get());
        QCOMPARE(roster.count(), 2);

        roster.close(QStringLiteral("s1"));

        const QList<mirror::PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 1);
        QCOMPARE(ops.first().verb, QStringLiteral("SessionUpdateMeta"));
        const QJsonObject args = QJsonDocument::fromJson(ops.first().payload).object();
        QCOMPARE(args.value(QStringLiteral("session")).toString(), QStringLiteral("s1"));
        QVERIFY(args.value(QStringLiteral("archived")).toBool());
        QCOMPARE(roster.count(), 2); // no local flip before the node's echo
    }

    // F6/DEL-6: the Active|Archived scope switcher re-projects the roster (archived rows only),
    // entering the scope schedules the Archived SessionsQuery fetch intent, and restore()
    // enqueues the SessionUpdateMeta{archived:false} lane op.
    void archivedScopeAndRestore() {
        Fixture fx;
        fx.seed({sess(QStringLiteral("s1"), QStringLiteral("Live one"), QStringLiteral("Active")),
                 sess(QStringLiteral("s2"), QStringLiteral("Old child"), QStringLiteral("Ready"),
                      /*archived=*/true)});
        mirror::LocalDatabase db(QStringLiteral(":memory:"));
        mirror::Outbox outbox(&db);
        outbox.load();
        fx.store->setOutbox(&outbox);
        DaemonSessionRoster roster(fx.store.get());

        // Default (active) scope: the archived row is filtered out.
        QCOMPARE(roster.count(), 1);
        QCOMPARE(roster.scope(), QStringLiteral("active"));

        QSignalSpy scoped(&roster, &fleet::ISessionRoster::scopeChanged);
        roster.setScope(QStringLiteral("archived"));
        QCOMPARE(scoped.count(), 1);
        auto* m = model(roster.sessions());
        QCOMPARE(roster.count(), 1);
        QCOMPARE(m->at(0).value(QStringLiteral("id")).toString(), QStringLiteral("s2"));

        // Entering the scope scheduled the authoritative Archived fetch (§5.4 job intent).
        bool sawArchivedFetch = false;
        for (const mirror::FetchJob& j : fx.executor.jobs) {
            sawArchivedFetch = sawArchivedFetch || (j.op == mirror::FetchOp::SessionsQuery &&
                                                    j.scope == QStringLiteral("archived"));
        }
        QVERIFY(sawArchivedFetch);

        // Restore enqueues the archived:false meta patch (node-authoritative un-archive).
        roster.restore(QStringLiteral("s2"));
        const QList<mirror::PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 1);
        const QJsonObject args = QJsonDocument::fromJson(ops.first().payload).object();
        QCOMPARE(args.value(QStringLiteral("session")).toString(), QStringLiteral("s2"));
        QVERIFY(!args.value(QStringLiteral("archived")).toBool());
    }

    // F4/DEL-4: the operator steer/startTurn/interrupt row actions submit to the TARGET session
    // id (byte-identical to the Submit encoders) over the surviving SessionRepository seam, and
    // a node Error surfaces via operationFailed.
    void steerStartTurnInterruptSubmitToSession() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("roster-steer.sock"));
        daemonapp::test::WireMuxServer fake;
        QByteArray ok;
        daemonapp::test::cborText(ok, "Ok");
        fake.setReplyPayload(ok);
        QVERIFY2(fake.start(path), "listen");

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::SessionRepository sessions(&client, nullptr);
        Fixture fx;
        DaemonSessionRoster roster(fx.store.get(), &sessions);

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

    // B4/EIO-8 ByTransport scope: the refresh schedules the ByTransport fetch intent; the
    // NODE-resolved membership set backs the scoped list projection (never re-derived locally),
    // and an unfetched lens honestly projects empty.
    void byTransportScopeProjectsNodeMembership() {
        Fixture fx;
        fx.seed(
            {sess(QStringLiteral("s1"), QStringLiteral("Local"), QStringLiteral("Active")),
             sess(QStringLiteral("s2"), QStringLiteral("Matrix bound"), QStringLiteral("Active"))});

        // Unfetched lens: honest empty projection (no client-side re-derivation).
        domain::ListScope scope;
        scope.type = domain::NodeType::ByTransport;
        scope.lensKey = QStringLiteral("matrix/@bot:hs.org");
        QCOMPARE(fx.store->sessions(scope).size(), 0);

        fx.store->refreshSessionsForTransport(scope.lensKey);
        bool sawTransportFetch = false;
        for (const mirror::FetchJob& j : fx.executor.jobs) {
            sawTransportFetch = sawTransportFetch || (j.op == mirror::FetchOp::SessionsQuery &&
                                                      j.scope == QStringLiteral("transport") +
                                                                     QChar(0x1f) + scope.lensKey);
        }
        QVERIFY(sawTransportFetch);

        // The node resolves: only s2 rides this transport.
        fx.svc.ingestor().deliverTransportSessions(
            scope.lensKey,
            {sess(QStringLiteral("s2"), QStringLiteral("Matrix bound"), QStringLiteral("Active"))},
            /*isFinalPage=*/true);
        const QList<domain::Session> scoped = fx.store->sessions(scope);
        QCOMPARE(scoped.size(), 1);
        QCOMPARE(scoped.first().sessionId.toString(), QStringLiteral("s2"));
    }

    // Dashboard counters derive from the roster + approvals + connection, and react to changes.
    void dashboardDerivesCounters() {
        Fixture fx;
        fx.seed({sess(QStringLiteral("s1"), QStringLiteral("Build"), QStringLiteral("Active")),
                 sess(QStringLiteral("s2"), QStringLiteral("Idle"), QStringLiteral("Ready"))});
        DaemonSessionRoster roster(fx.store.get());
        EmptyFleetTree fleetTree;
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

QTEST_MAIN(TestDaemonRosterDashboard)
#include "tst_daemon_roster_dashboard.moc"
