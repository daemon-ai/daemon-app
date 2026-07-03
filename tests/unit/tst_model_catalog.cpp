// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "cache_test_support.h"
#include "models/mock_model_catalog.h"
#include "models/provider_filter.h"
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

    // C2: the provider filter helper matches installed rows by ProviderSelector kind, normalizing
    // the local "llama" engine tag to its "llama_cpp" selector; an empty/unmatched provider yields
    // an empty list (the catalog method then falls back to all ids).
    void filterHelperMatchesByProviderKind() {
        const auto row = [](const QString& id, const QString& provider) {
            return QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("provider"), provider}};
        };
        const QList<QVariantMap> rows = {
            row(QStringLiteral("m-llama"), QStringLiteral("llama")), // local engine tag
            row(QStringLiteral("m-mistral"), QStringLiteral("mistral_rs")),
            row(QStringLiteral("m-genai"), QStringLiteral("genai")),
            row(QStringLiteral("m-daemon"), QStringLiteral("daemon_api")),
        };
        QCOMPARE(models::filterInstalledIdsByProvider(rows, QStringLiteral("llama_cpp")),
                 QStringList{QStringLiteral("m-llama")});
        QCOMPARE(models::filterInstalledIdsByProvider(rows, QStringLiteral("mistral_rs")),
                 QStringList{QStringLiteral("m-mistral")});
        QCOMPARE(models::filterInstalledIdsByProvider(rows, QStringLiteral("genai")),
                 QStringList{QStringLiteral("m-genai")});
        QCOMPARE(models::filterInstalledIdsByProvider(rows, QStringLiteral("daemon_api")),
                 QStringList{QStringLiteral("m-daemon")});
        QVERIFY(models::filterInstalledIdsByProvider(rows, QString()).isEmpty());
        QVERIFY(models::filterInstalledIdsByProvider(rows, QStringLiteral("nope")).isEmpty());
    }

    // The catalog method falls back to ALL installed ids when the provider matches nothing
    // installed (the mock tags rows with vendor names, and remote providers have no local rows), so
    // the model combo is never wrongly empty.
    void installedIdsForProviderFallsBackToAll() {
        MockModelCatalog c;
        const QStringList all = c.installedIds();
        QVERIFY(!all.isEmpty());
        QCOMPARE(c.installedIdsForProvider(QStringLiteral("daemon_api")),
                 all);                                       // no daemon_api rows
        QCOMPARE(c.installedIdsForProvider(QString()), all); // empty provider
    }

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

    // Download rows carry the daemon-row render keys (repo/error/sizeLabel/downloadedLabel) plus
    // a brief queued state, and the shared helpers behave: an active row refuses dismissal, and
    // installedIdFor resolves iff installed (the mock "repo" IS the catalog model id).
    void downloadRowParityAndHelpers() {
        MockModelCatalog c;
        const QString modelId = QStringLiteral("mistral-7b-instruct");
        QCOMPARE(c.installedIdFor(modelId, QString()), QString()); // not installed yet

        c.download(modelId);
        auto* downloads = asModel(c.downloads());
        QCOMPARE(downloads->count(), 1);
        const QVariantMap row = downloads->at(0);
        QCOMPARE(row.value(QStringLiteral("state")).toString(), QStringLiteral("queued"));
        QCOMPARE(row.value(QStringLiteral("repo")).toString(), modelId);
        QVERIFY(row.contains(QStringLiteral("error")));
        QVERIFY(!row.value(QStringLiteral("sizeLabel")).toString().isEmpty());
        QVERIFY(!row.value(QStringLiteral("downloadedLabel")).toString().isEmpty());

        // Active (queued/downloading) rows refuse dismissal.
        const QString jobId = row.value(QStringLiteral("id")).toString();
        c.dismissDownload(jobId);
        QCOMPARE(downloads->count(), 1);

        // The first tick flips the brief queued phase to downloading.
        QVERIFY(QTest::qWaitFor(
            [&] {
                return downloads->count() == 1 &&
                       downloads->at(0).value(QStringLiteral("state")).toString() ==
                           QStringLiteral("downloading");
            },
            3000));

        QSignalSpy done(&c, &models::IModelCatalog::downloadFinished);
        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 8000));
        QCOMPARE(c.installedIdFor(modelId, QStringLiteral("any.gguf")), modelId);
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
