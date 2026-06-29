// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "automation/mock_cron_store.h"
#include "automation/mock_routing_store.h"
#include "cache_test_support.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using namespace automation;
using uimodels::VariantListModel;

// Guards the Phase 7 automation mocks: routing target/enable edits + add/remove,
// and cron create/update/runNow/enable/remove.
class TestAutomation : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void init() { resetMockCache(); }

    void routingEdits() {
        MockRoutingStore r;
        QVERIFY(asModel(r.rules())->count() >= 3);
        QVERIFY(!r.targets().isEmpty());

        r.setTarget(QStringLiteral("r-1"), QStringLiteral("gemma-2-9b-it"));
        QCOMPARE(asModel(r.rules())
                     ->at(asModel(r.rules())->indexOfId(QStringLiteral("r-1")))
                     .value(QStringLiteral("target"))
                     .toString(),
                 QStringLiteral("gemma-2-9b-it"));

        r.setEnabled(QStringLiteral("r-1"), false);
        QVERIFY(!asModel(r.rules())
                     ->at(asModel(r.rules())->indexOfId(QStringLiteral("r-1")))
                     .value(QStringLiteral("enabled"))
                     .toBool());

        r.setIntent(QStringLiteral("r-1"), QStringLiteral("Renamed intent"));
        QCOMPARE(asModel(r.rules())
                     ->at(asModel(r.rules())->indexOfId(QStringLiteral("r-1")))
                     .value(QStringLiteral("intent"))
                     .toString(),
                 QStringLiteral("Renamed intent"));

        r.setFallback(QStringLiteral("r-1"), QStringLiteral("phi-3.5-mini"));
        QCOMPARE(asModel(r.rules())
                     ->at(asModel(r.rules())->indexOfId(QStringLiteral("r-1")))
                     .value(QStringLiteral("fallback"))
                     .toString(),
                 QStringLiteral("phi-3.5-mini"));

        const int before = asModel(r.rules())->count();
        const QString id = r.addRule(QStringLiteral("Translation"));
        QCOMPARE(asModel(r.rules())->count(), before + 1);
        r.remove(id);
        QCOMPARE(asModel(r.rules())->count(), before);
    }

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
