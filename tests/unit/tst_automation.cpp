// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "automation/mock_cron_store.h"
#include "cache_test_support.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using namespace automation;
using uimodels::VariantListModel;

// Guards the Phase 7 cron mock: create/update/runNow/enable/remove + restart persistence. (The
// legacy intent->model routing mock was retired at wire v28 — routing is the origin->session pin
// table on the mirror store, covered by tst_routing_projection + tst_routing_repository.)
class TestAutomation : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void init() { resetMockCache(); }

    void cronLifecycle() {
        MockCronStore c;
        const int before = asModel(c.jobs())->count();
        const QString id = c.createJob(QStringLiteral("Test job"), QStringLiteral("0 6 * * *"),
                                       QStringLiteral("Coder"));
        QCOMPARE(asModel(c.jobs())->count(), before + 1);

        c.updateJob(id, {{QStringLiteral("schedule"), QStringLiteral("30 7 * * 1")}});
        QCOMPARE(c.job(id).value(QStringLiteral("schedule")).toString(),
                 QStringLiteral("30 7 * * 1"));

        c.setEnabled(id, false);
        QVERIFY(!c.job(id).value(QStringLiteral("enabled")).toBool());

        c.runNow(id);
        QCOMPARE(c.job(id).value(QStringLiteral("lastRun")).toString(), QStringLiteral("just now"));

        c.remove(id);
        QVERIFY(c.job(id).isEmpty());
    }

    // Tier 3: a created cron job survives across construction via the last-known
    // on-disk cache.
    void cronPersistsAcrossRestart() {
        QString id;
        {
            MockCronStore c;
            id = c.createJob(QStringLiteral("Nightly"), QStringLiteral("0 3 * * *"),
                             QStringLiteral("Coder"));
        }

        MockCronStore reborn;
        const QVariantMap job = reborn.job(id);
        QVERIFY(!job.isEmpty());
        QCOMPARE(job.value(QStringLiteral("name")).toString(), QStringLiteral("Nightly"));
    }
};

QTEST_MAIN(TestAutomation)
#include "tst_automation.moc"
