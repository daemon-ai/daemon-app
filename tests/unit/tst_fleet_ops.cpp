// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_session_roster.h"
#include "daemonnet/mock_fleet_source.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "persistence/in_memory_session_store.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using namespace fleet;
using uimodels::VariantListModel;

// Guards the surviving Phase 6 ops mocks after the M5 cutover: fleet pause/resume
// (MockFleetTree), approvals approve/deny, and the MockDashboard counter derivation —
// now observed over the SAME roster implementation both modes run (DaemonSessionRoster
// projecting an ISessionStore; MockSessionRoster died with M5). The fleet/session rows
// derive from the one seed bundle, so the tests reference its ids (`s-help`, `n-coder`).
class TestFleetOps : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void fleetPauseResume() {
        daemonnet::MockFleetSource net;
        MockFleetTree f(&net);
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
    // one roster implementation (DaemonSessionRoster over the seeded in-memory store, exactly the
    // mock graph's composition since M5).
    void dashboardDerivesCounters() {
        daemonnet::MockFleetSource net;
        persistence::InMemorySessionStore store(&net);
        DaemonSessionRoster r(&store);
        MockFleetTree f(&net);
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

        // Closing archives the session (client-local): it leaves the active roster entirely.
        const int activeAfterSuspend = d.activeSessions();
        r.close(QStringLiteral("s-design"));
        QVERIFY(d.activeSessions() < activeAfterSuspend);
        QVERIFY(changed.count() >= 1);
    }

    // Approvals resolution appends a live activity entry (structural removal on the inbox model).
    // Roster-driven token totals are gone with the mock roster: the shared roster projection
    // carries no client-side token data (the aligned mock==daemon degradation), so tokensToday
    // reports the true zero rather than a mock-only fiction.
    void dashboardActivityAndAlignedTokens() {
        daemonnet::MockFleetSource net;
        persistence::InMemorySessionStore store(&net);
        DaemonSessionRoster r(&store);
        MockFleetTree f(&net);
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
