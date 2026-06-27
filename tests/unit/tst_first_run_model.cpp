#include "connection/mock_connection_service.h"
#include "firstrun/first_run_model.h"
#include "models/imodel_catalog.h"
#include "settings/qt_settings_store.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using connection::MockConnectionService;
using firstrun::FirstRunModel;
using settings::QtSettingsStore;

namespace {
// A minimal IModelCatalog whose current model is controllable, to drive the inference readiness
// gate (CON-7). activate(id) sets the current model and emits currentChanged, like the real one.
class FakeModelCatalog : public models::IModelCatalog {
public:
    QString current;
    [[nodiscard]] QObject* discover() const override { return nullptr; }
    [[nodiscard]] QObject* files() const override { return nullptr; }
    [[nodiscard]] QObject* downloads() const override { return nullptr; }
    [[nodiscard]] QObject* installed() const override { return nullptr; }
    [[nodiscard]] QString currentModelId() const override { return current; }
    [[nodiscard]] QString filesRepo() const override { return {}; }
    [[nodiscard]] QStringList installedIds() const override { return {}; }
    [[nodiscard]] QVariantList providers() const override { return {}; }
    void search(const QString&, const QString&) override {}
    void repoFiles(const QString&) override {}
    void recommend(const QString&) override {}
    [[nodiscard]] QVariantMap recommendation() const override { return {}; }
    void download(const QString&) override {}
    void downloadFile(const QString&, const QString&, const QString&) override {}
    void pauseDownload(const QString&) override {}
    void resumeDownload(const QString&) override {}
    void cancelDownload(const QString&) override {}
    void activate(const QString& id) override {
        current = id;
        emit currentChanged();
    }
    void remove(const QString&) override {}
};
} // namespace

// Guards the shared onboarding gate: a fresh install starts at connect, a good
// connection drives connect -> connecting -> inference, completeInference
// finishes and persists setupComplete, and a returning user boots to done.
class TestFirstRunModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void init() {
        QtSettingsStore s;
        s.setSetupComplete(false);
    }

    void freshInstallStartsAtConnect() {
        QtSettingsStore settings;
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();
        QCOMPARE(m.phase(), QStringLiteral("connect"));
        QVERIFY(m.active());
    }

    void goodConnectionAdvancesToInferenceThenDone() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();

        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));

        QSignalSpy finished(&m, &FirstRunModel::finished);
        m.completeInference();
        QCOMPARE(m.phase(), QStringLiteral("done"));
        QVERIFY(!m.active());
        QCOMPARE(finished.count(), 1);
        QVERIFY(settings.setupComplete());
    }

    // CON-1: the connection succeeding (state -> ready) persists setupComplete immediately, before
    // the (placeholder) inference step, so the next launch auto-connects even if the user leaves.
    void connectionSuccessPersistsSetupComplete() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();
        QVERIFY(!settings.setupComplete());

        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));
        // Persisted on connection success, without needing the explicit completeInference().
        QVERIFY(settings.setupComplete());
    }

    // CON-7: with a model catalog wired, the inference gate's completeInference() is a no-op until
    // a usable model is reachable; once a model becomes current, readiness flips and Finish
    // completes.
    void inferenceGateBlocksUntilModelReachable() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FakeModelCatalog catalog; // no current model yet
        FirstRunModel m(&settings, &conn, &catalog);
        m.begin();

        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));
        QVERIFY(!m.inferenceReady());

        // Not ready: completeInference is a no-op, the gate stays on the inference step.
        m.completeInference();
        QCOMPARE(m.phase(), QStringLiteral("inference"));

        // A model becomes current -> readiness flips -> Finish completes.
        catalog.activate(QStringLiteral("claude-opus-4-8"));
        QVERIFY(m.inferenceReady());
        m.completeInference();
        QCOMPARE(m.phase(), QStringLiteral("done"));
    }

    void badConnectionReturnsToConnectWithError() {
        QtSettingsStore settings;
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();

        conn.connectTo(QStringLiteral("remote"), QStringLiteral("https://example.invalid"));
        QVERIFY(QTest::qWaitFor(
            [&] { return m.phase() == QStringLiteral("connect") && !m.error().isEmpty(); }, 3000));
    }

    void returningUserBootsToDone() {
        QtSettingsStore settings;
        settings.setSetupComplete(true);
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();
        QCOMPARE(m.phase(), QStringLiteral("done"));
        QVERIFY(!m.active());
    }
};

QTEST_MAIN(TestFirstRunModel)
#include "tst_first_run_model.moc"
