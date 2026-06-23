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
class TestFleetOps : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void rosterStateTransitions()
    {
        MockSessionRoster r;
        const int before = r.count();
        r.suspend(QStringLiteral("s-1"));
        QCOMPARE(asModel(r.sessions())->at(asModel(r.sessions())->indexOfId(QStringLiteral("s-1")))
                     .value(QStringLiteral("state"))
                     .toString(),
                 QStringLiteral("suspended"));
        r.resume(QStringLiteral("s-1"));
        QCOMPARE(asModel(r.sessions())->at(asModel(r.sessions())->indexOfId(QStringLiteral("s-1")))
                     .value(QStringLiteral("state"))
                     .toString(),
                 QStringLiteral("active"));
        r.close(QStringLiteral("s-1"));
        QCOMPARE(r.count(), before - 1);
    }

    void fleetPauseResume()
    {
        MockFleetTree f;
        f.pause(QStringLiteral("n-1"));
        QCOMPARE(asModel(f.nodes())->at(asModel(f.nodes())->indexOfId(QStringLiteral("n-1")))
                     .value(QStringLiteral("status"))
                     .toString(),
                 QStringLiteral("paused"));
        f.resume(QStringLiteral("n-1"));
        QCOMPARE(asModel(f.nodes())->at(asModel(f.nodes())->indexOfId(QStringLiteral("n-1")))
                     .value(QStringLiteral("status"))
                     .toString(),
                 QStringLiteral("running"));
    }

    void approvalsResolve()
    {
        MockApprovalsInbox a;
        const int before = a.count();
        a.approve(QStringLiteral("a-1"));
        a.deny(QStringLiteral("a-2"));
        QCOMPARE(a.count(), before - 2);
    }

    void dashboardDerivesCounters()
    {
        MockSessionRoster r;
        MockFleetTree f;
        MockApprovalsInbox a;
        MockDashboard d(&r, &f, &a);

        const int activeBefore = d.activeSessions();
        const int approvalsBefore = d.pendingApprovals();
        QVERIFY(activeBefore >= 1);
        QVERIFY(approvalsBefore >= 1);

        QSignalSpy changed(&d, &IDashboard::changed);
        a.approve(QStringLiteral("a-1"));
        QCOMPARE(d.pendingApprovals(), approvalsBefore - 1);

        r.suspend(QStringLiteral("s-1"));
        QVERIFY(d.activeSessions() < activeBefore);
        QVERIFY(changed.count() >= 1);
    }

    // tokensToday is derived from the roster (sum of session tokens), and closing
    // a session both lowers it and appends a "Session closed" activity entry.
    void dashboardTokensAndActivityAreLive()
    {
        MockSessionRoster r;
        MockFleetTree f;
        MockApprovalsInbox a;
        MockDashboard d(&r, &f, &a);

        const QString tokensBefore = d.tokensToday();
        QVERIFY(!tokensBefore.isEmpty());
        QVERIFY(tokensBefore != QLatin1String("0"));

        const int activityBefore = asModel(d.activity())->count();
        r.close(QStringLiteral("s-1"));
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
