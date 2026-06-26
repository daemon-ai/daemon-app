#include "cache_test_support.h"
#include "config/mock_daemon_config.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using config::MockDaemonConfig;

// Guards the daemon-config facade mock: seeded defaults, typed round-trip,
// enumerated options for combo-backed keys, and change signalling/dedup.
class TestDaemonConfig : public QObject {
    Q_OBJECT

private slots:
    void init() { resetMockCache(); }

    void seededDefaults() {
        MockDaemonConfig c;
        QCOMPARE(c.value(QStringLiteral("model/effort")).toString(), QStringLiteral("Balanced"));
        QCOMPARE(c.value(QStringLiteral("chat/streaming")).toBool(), true);
        QCOMPARE(c.value(QStringLiteral("memory/contextWindow")).toInt(), 128000);
    }

    void optionsForEnumKeys() {
        MockDaemonConfig c;
        const QStringList policy = c.options(QStringLiteral("safety/approvalPolicy"));
        QVERIFY(policy.contains(QStringLiteral("On request")));
        QVERIFY(c.options(QStringLiteral("model/default")).isEmpty()); // not an enum key
    }

    void roundTripAndSignal() {
        MockDaemonConfig c;
        QSignalSpy spy(&c, &config::IDaemonConfig::changed);
        c.setValue(QStringLiteral("safety/allowNetwork"), true);
        QCOMPARE(c.value(QStringLiteral("safety/allowNetwork")).toBool(), true);
        QCOMPARE(spy.count(), 1);
        c.setValue(QStringLiteral("safety/allowNetwork"), true); // no-op
        QCOMPARE(spy.count(), 1);
    }

    // Tier 3: a changed value survives across construction, overlaying the seed.
    void valuePersistsAcrossRestart() {
        {
            MockDaemonConfig c;
            c.setValue(QStringLiteral("memory/contextWindow"), 64000);
        }

        MockDaemonConfig reborn;
        QCOMPARE(reborn.value(QStringLiteral("memory/contextWindow")).toInt(), 64000);
        // Untouched keys still come from the seed.
        QCOMPARE(reborn.value(QStringLiteral("model/effort")).toString(),
                 QStringLiteral("Balanced"));
    }
};

QTEST_MAIN(TestDaemonConfig)
#include "tst_daemon_config.moc"
