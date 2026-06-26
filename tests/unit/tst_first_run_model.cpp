#include "connection/mock_connection_service.h"
#include "firstrun/first_run_model.h"
#include "settings/qt_settings_store.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtTest/QtTest>

using connection::MockConnectionService;
using firstrun::FirstRunModel;
using settings::QtSettingsStore;

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
