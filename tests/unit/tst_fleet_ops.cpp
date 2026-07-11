// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_session_roster.h"
#include "daemon/mirror_fleet_tree.h"
#include "daemon/mirror_session_store.h"
#include "daemon/mock_scenario.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "local_database.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "outbox.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using namespace fleet;
using uimodels::VariantListModel;

// Guards the surviving Phase 6 ops surfaces after AD: fleet pause/resume (the client-local
// overlay on the MIRROR fleet tree), approvals approve/deny, and the MockDashboard counter
// derivation — observed over the SAME roster + tree implementations both modes run
// (DaemonSessionRoster + MirrorFleetTree over the scenario-seeded mirror; the legacy
// InMemory/MockFleet fixtures died with AD). Row ids reference the one seed bundle
// (`s-help`, `n-coder`).
class TestFleetOps : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void fleetPauseResume() {
        mirror::MirrorService svc;
        svc.openInMemory();
        mirror::Seeder seeder(svc.store());
        seeder.seed(daemonapp::daemon::mockScenarioByName(QStringLiteral("default")).mirror.seed);
        fleet::MirrorFleetTree f(&svc.store(), &svc.ingestor(), nullptr);

        f.pause(QStringLiteral("n-coder"));
        QCOMPARE(asModel(f.nodes())
                     ->at(asModel(f.nodes())->indexOfId(QStringLiteral("n-coder")))
                     .value(QStringLiteral("status"))
                     .toString(),
                 QStringLiteral("paused"));
        f.resume(QStringLiteral("n-coder"));
        QCOMPARE(asModel(f.nodes())
                     ->at(asModel(f.nodes())->indexOfId(QStringLiteral("n-coder")))
                     .value(QStringLiteral("status"))
                     .toString(),
                 QStringLiteral("running"));
    }

    void approvalsResolve() {
        MockApprovalsInbox a;
        const int before = a.count();
        a.approve(QStringLiteral("a-1"));
        a.deny(QStringLiteral("a-2"));
        QCOMPARE(a.count(), before - 2);
    }

    // The mock dashboard derives its counters live from the seam interfaces — observed over the
    // one roster implementation (DaemonSessionRoster over the scenario-seeded MIRROR store,
    // exactly the mock graph's composition since AD).
    void dashboardDerivesCounters() {
        mirror::MirrorService svc;
        svc.openInMemory();
        mirror::Seeder seeder(svc.store());
        seeder.seed(daemonapp::daemon::mockScenarioByName(QStringLiteral("default")).mirror.seed);
        daemonapp::daemon::MirrorSessionStore store(&svc.store(), &svc.ingestor());
        mirror::LocalDatabase db(QStringLiteral(":memory:"));
        mirror::Outbox outbox(&db);
        outbox.load();
        store.setOutbox(&outbox);
        DaemonSessionRoster r(&store);
        fleet::MirrorFleetTree f(&svc.store(), &svc.ingestor(), nullptr);
        MockApprovalsInbox a;
        MockDashboard d(&r, &f, &a);

        const int activeBefore = d.activeSessions();
        const int approvalsBefore = d.pendingApprovals();
        QVERIFY(activeBefore >= 1);
        QVERIFY(approvalsBefore >= 1);
        QVERIFY(d.runningAgents() >= 1);

        QSignalSpy changed(&d, &IDashboard::changed);
        a.approve(QStringLiteral("a-1"));
        QCOMPARE(d.pendingApprovals(), approvalsBefore - 1);

        // The client-local suspend overlay flips the row's state out of "active".
        r.suspend(QStringLiteral("s-help"));
        QVERIFY(d.activeSessions() < activeBefore);

        // Closing archives the session: the intent rides the session-meta lane (durable op),
        // and the ROW leaves the roster only when the node's echo lands (§14.7 — never an
        // optimistic local flip). Simulate the echo through the seeder, as the mock host does.
        const int activeAfterSuspend = d.activeSessions();
        r.close(QStringLiteral("s-design"));
        const QList<mirror::PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 1);
        QCOMPARE(ops.first().verb, QStringLiteral("SessionUpdateMeta"));
        QCOMPARE(d.activeSessions(), activeAfterSuspend); // no local mutation before the echo
        const mirror::Session* row =
            svc.store().snapshot().sessions.find(mirror::SessionKey{QStringLiteral("s-design")});
        QVERIFY(row != nullptr);
        mirror::Session archived = *row;
        archived.archived = true;
        seeder.upsertSession(archived, ops.first().opId);
        QVERIFY(d.activeSessions() < activeAfterSuspend);
        QVERIFY(changed.count() >= 1);
    }

    // Approvals resolution appends a live activity entry (structural removal on the inbox model).
    // Roster-driven token totals stay gone: the shared roster projection carries no client-side
    // token data (the aligned mock==daemon degradation), so tokensToday reports the true zero.
    void dashboardActivityAndAlignedTokens() {
        mirror::MirrorService svc;
        svc.openInMemory();
        mirror::Seeder seeder(svc.store());
        seeder.seed(daemonapp::daemon::mockScenarioByName(QStringLiteral("default")).mirror.seed);
        daemonapp::daemon::MirrorSessionStore store(&svc.store(), &svc.ingestor());
        DaemonSessionRoster r(&store);
        fleet::MirrorFleetTree f(&svc.store(), &svc.ingestor(), nullptr);
        MockApprovalsInbox a;
        MockDashboard d(&r, &f, &a);

        QCOMPARE(d.tokensToday(), QStringLiteral("0"));

        const int activityBefore = asModel(d.activity())->count();
        a.approve(QStringLiteral("a-1"));
        QCOMPARE(asModel(d.activity())->count(), activityBefore + 1);
        QCOMPARE(asModel(d.activity())->rows().first().value(QStringLiteral("text")).toString(),
                 QStringLiteral("Approval resolved"));
    }
};

QTEST_MAIN(TestFleetOps)
#include "tst_fleet_ops.moc"
