// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "cache_test_support.h"
#include "models/mock_model_catalog.h"
#include "uimodels/variant_list_model.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using models::MockModelCatalog;
using uimodels::VariantListModel;

// Guards the Models hub mock: search + size filter, a download that ticks to
// completion and lands in Installed, activate flips the current model, and remove
// drops it.
class TestModelCatalog : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void init() { resetMockCache(); }

    void searchFiltersByQueryAndSize() {
        MockModelCatalog c;
        c.search(QStringLiteral("llama"), QString());
        auto* d = asModel(c.discover());
        QVERIFY(d->count() >= 2);
        for (const QVariantMap& m : d->rows()) {
            QVERIFY(m.value(QStringLiteral("name"))
                        .toString()
                        .toLower()
                        .contains(QStringLiteral("llama")));
        }

        c.search(QString(), QStringLiteral("<=8B"));
        for (const QVariantMap& m : asModel(c.discover())->rows()) {
            QVERIFY(m.value(QStringLiteral("paramsB")).toDouble() <= 8.0);
        }
    }

    void downloadTicksToInstalled() {
        MockModelCatalog c;
        auto* installed = asModel(c.installed());
        const int before = installed->count();

        QSignalSpy done(&c, &models::IModelCatalog::downloadFinished);
        c.download(QStringLiteral("mistral-7b-instruct"));
        QVERIFY(asModel(c.downloads())->count() == 1);

        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 8000));
        QCOMPARE(asModel(c.downloads())->count(), 0);
        QCOMPARE(installed->count(), before + 1);
        QVERIFY(installed->indexOfId(QStringLiteral("mistral-7b-instruct")) >= 0);
    }

    void activateAndRemove() {
        MockModelCatalog c;
        // The seeded default is installed; activate a freshly-installed one.
        QSignalSpy done(&c, &models::IModelCatalog::downloadFinished);
        c.download(QStringLiteral("gemma-2-9b-it"));
        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 8000));

        QSignalSpy cur(&c, &models::IModelCatalog::currentChanged);
        c.activate(QStringLiteral("gemma-2-9b-it"));
        QCOMPARE(c.currentModelId(), QStringLiteral("gemma-2-9b-it"));
        QCOMPARE(cur.count(), 1);

        c.remove(QStringLiteral("gemma-2-9b-it"));
        QVERIFY(asModel(c.installed())->indexOfId(QStringLiteral("gemma-2-9b-it")) < 0);
        QVERIFY(c.currentModelId().isEmpty());
    }

    void installedIdsMirrorsInstalledModel() {
        MockModelCatalog c;
        auto* installed = asModel(c.installed());
        QStringList ids = c.installedIds();
        QCOMPARE(ids.size(), installed->count());
        for (const QVariantMap& row : installed->rows()) {
            QVERIFY(ids.contains(row.value(QStringLiteral("id")).toString()));
        }

        // A newly installed model shows up in installedIds().
        QSignalSpy done(&c, &models::IModelCatalog::downloadFinished);
        c.download(QStringLiteral("phi-3.5-mini"));
        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 8000));
        QVERIFY(c.installedIds().contains(QStringLiteral("phi-3.5-mini")));
    }

    // Tier 3: the installed set + active model survive across construction via the
    // last-known on-disk cache (download/activate save; a fresh instance reloads).
    void installedAndCurrentPersistAcrossRestart() {
        {
            MockModelCatalog c;
            QSignalSpy done(&c, &models::IModelCatalog::downloadFinished);
            c.download(QStringLiteral("gemma-2-9b-it"));
            QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 8000));
            c.activate(QStringLiteral("gemma-2-9b-it"));
        }

        MockModelCatalog reborn;
        QVERIFY(reborn.installedIds().contains(QStringLiteral("gemma-2-9b-it")));
        QCOMPARE(reborn.currentModelId(), QStringLiteral("gemma-2-9b-it"));
    }
};

QTEST_MAIN(TestModelCatalog)
#include "tst_model_catalog.moc"
