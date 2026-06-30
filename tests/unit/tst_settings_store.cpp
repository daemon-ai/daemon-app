// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/qt_settings_store.h"

#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest/QtTest>

using settings::QtSettingsStore;

// Guards the shared client-local preference store: typed get/set with defaults,
// the named convenience accessors (setupComplete, last connection), change
// signalling, and write de-duplication.
class TestSettingsStore : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Redirect QSettings to a throwaway location so the suite never touches
        // the developer's real config.
        QStandardPaths::setTestModeEnabled(true);
    }

    void init() {
        QtSettingsStore s;
        s.setValue(QStringLiteral("app/setupComplete"), false);
        s.setValue(QStringLiteral("conn/mode"), QStringLiteral("local"));
        s.setValue(QStringLiteral("conn/target"), QString());
    }

    void defaultsAndRoundTrip() {
        QtSettingsStore s;
        QCOMPARE(s.value(QStringLiteral("nope"), 42).toInt(), 42);
        s.setValue(QStringLiteral("ui/x"), 7);
        QCOMPARE(s.value(QStringLiteral("ui/x")).toInt(), 7);
    }

    void setupCompleteFlag() {
        QtSettingsStore s;
        QVERIFY(!s.setupComplete());
        s.setSetupComplete(true);
        QVERIFY(s.setupComplete());
    }

    void lastConnectionHelpers() {
        QtSettingsStore s;
        QCOMPARE(s.lastConnectionMode(), QStringLiteral("local"));
        s.setLastConnection(QStringLiteral("remote"), QStringLiteral("https://node.example"));
        QCOMPARE(s.lastConnectionMode(), QStringLiteral("remote"));
        QCOMPARE(s.lastConnectionTarget(), QStringLiteral("https://node.example"));
    }

    // Per-target server-token storage (auth6): tokens are keyed by target so multiple nodes don't
    // collide; clearing one (logout) leaves the others intact. In test mode the keychain is
    // bypassed so this exercises the QSettings fallback path deterministically.
    void perTargetTokenRoundTripAndClear() {
        QtSettingsStore s;
        const QString a = QStringLiteral("node-a:8443");
        const QString b = QStringLiteral("node-b:8443");
        QVERIFY(s.connectionToken(a).isEmpty());

        s.setConnectionToken(a, QStringLiteral("token-A"));
        s.setConnectionToken(b, QStringLiteral("token-B"));
        QCOMPARE(s.connectionToken(a), QStringLiteral("token-A"));
        QCOMPARE(s.connectionToken(b), QStringLiteral("token-B"));

        // Logout for A clears only A's token.
        s.clearConnectionToken(a);
        QVERIFY(s.connectionToken(a).isEmpty());
        QCOMPARE(s.connectionToken(b), QStringLiteral("token-B"));

        // An empty target is a no-op (never stores a token under a blank key).
        s.setConnectionToken(QString(), QStringLiteral("x"));
        QVERIFY(s.connectionToken(QString()).isEmpty());
    }

    void emitsChangeAndDeduplicates() {
        QtSettingsStore s;
        s.setValue(QStringLiteral("k"), 1);
        QSignalSpy spy(&s, &settings::ISettingsStore::changed);
        s.setValue(QStringLiteral("k"), 2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("k"));
        // Writing the same value again is a no-op (no signal).
        s.setValue(QStringLiteral("k"), 2);
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(TestSettingsStore)
#include "tst_settings_store.moc"
