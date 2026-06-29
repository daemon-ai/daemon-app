// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/mock_daemonnet.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fleet/mock_session_roster.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using namespace fleet;
using uimodels::VariantListModel;

// Guards the Phase 6 ops mocks: roster suspend/resume/close, fleet pause/resume,
// approvals approve/deny, and a dashboard that derives counters live from them.
// The roster/fleet mocks now derive their rows from the unified mock DaemonNet
// (the single source), so the tests seed one and reference its ids (`s-help`, `n-coder`).
class TestFleetOps : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void rosterStateTransitions() {
        daemonnet::MockDaemonNet net;
        MockSessionRoster r(&net);
        const int before = r.count();
        r.suspend(QStringLiteral("s-help"));
        QCOMPARE(asModel(r.sessions())
                     ->at(asModel(r.sessions())->indexOfId(QStringLiteral("s-help")))
                     .value(QStringLiteral("state"))
                     .toString(),
                 QStringLiteral("suspended"));
        r.resume(QStringLiteral("s-help"));
        QCOMPARE(asModel(r.sessions())
                     ->at(asModel(r.sessions())->indexOfId(QStringLiteral("s-help")))
                     .value(QStringLiteral("state"))
                     .toString(),
                 QStringLiteral("active"));
        r.close(QStringLiteral("s-help"));
        QCOMPARE(r.count(), before - 1);
    }

    void fleetPauseResume() {
        daemonnet::MockDaemonNet net;
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

    void dashboardDerivesCounters() {
        daemonnet::MockDaemonNet net;
        MockSessionRoster r(&net);
        MockFleetTree f(&net);
        MockApprovalsInbox a;
        MockDashboard d(&r, &f, &a);

        const int activeBefore = d.activeSessions();
        const int approvalsBefore = d.pendingApprovals();
        QVERIFY(activeBefore >= 1);
        QVERIFY(approvalsBefore >= 1);

        QSignalSpy changed(&d, &IDashboard::changed);
        a.approve(QStringLiteral("a-1"));
        QCOMPARE(d.pendingApprovals(), approvalsBefore - 1);

        r.suspend(QStringLiteral("s-help"));
        QVERIFY(d.activeSessions() < activeBefore);
        QVERIFY(changed.count() >= 1);
    }

    // tokensToday is derived from the roster (sum of session tokens), and closing
    // a session both lowers it and appends a "Session closed" activity entry.
    void dashboardTokensAndActivityAreLive() {
        daemonnet::MockDaemonNet net;
        MockSessionRoster r(&net);
        MockFleetTree f(&net);
        MockApprovalsInbox a;
        MockDashboard d(&r, &f, &a);

        const QString tokensBefore = d.tokensToday();
        QVERIFY(!tokensBefore.isEmpty());
        QVERIFY(tokensBefore != QLatin1String("0"));

        const int activityBefore = asModel(d.activity())->count();
        r.close(QStringLiteral("s-help"));
        // A new activity row is prepended for the close...
        QCOMPARE(asModel(d.activity())->count(), activityBefore + 1);
        QCOMPARE(asModel(d.activity())->rows().first().value(QStringLiteral("text")).toString(),
                 QStringLiteral("Session closed"));
        // ...and the derived token total changed.
        QVERIFY(d.tokensToday() != tokensBefore);

        // Resolving an approval also logs activity.
        const int afterClose = asModel(d.activity())->count();
        a.approve(QStringLiteral("a-1"));
        QCOMPARE(asModel(d.activity())->count(), afterClose + 1);
        QCOMPARE(asModel(d.activity())->rows().first().value(QStringLiteral("text")).toString(),
                 QStringLiteral("Approval resolved"));
    }
};

QTEST_MAIN(TestFleetOps)
#include "tst_fleet_ops.moc"
