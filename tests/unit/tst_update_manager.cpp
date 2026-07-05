// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/update_manager.h"

#include <QRegularExpression>
#include <QtTest>

using update::UpdateManager;
using Capability = update::UpdateManager::Capability;
using State = update::UpdateManager::State;

// Locks the UpdateManager surface (packaging/UPDATES.md): the package-time
// capability dial defaults to None (this test tree sets no dial), the capability
// vocabulary round-trips with unknown input failing CLOSED to None, and an inert
// (None) build performs no feed activity - check() is a no-op and the
// download/apply actions fail closed since nothing may be downloaded.
class UpdateManagerTests : public QObject {
    Q_OBJECT

private slots:
    void defaults();
    void capabilityRoundTrip();
    void unknownCapabilityFailsClosed();
    void inertBuildDoesNothing();
};

void UpdateManagerTests::defaults() {
    UpdateManager manager;

    // Built without -DDAEMON_APP_UPDATE_CAPABILITY / _FEED_URL: fully inert.
    QCOMPARE(manager.capability(), Capability::None);
    QVERIFY(manager.feedUrl().isEmpty());
    QCOMPARE(manager.state(), State::Idle);
    QVERIFY(manager.lastError().isEmpty());

    // currentVersion carries the generated build string: SemVer base, with the
    // git build-metadata suffix present except on a clean release tag.
    QVERIFY(!manager.currentVersion().isEmpty());
    QVERIFY(QRegularExpression(QStringLiteral(R"(^\d+\.\d+\.\d+([+-]|$))"))
                .match(manager.currentVersion())
                .hasMatch());
}

void UpdateManagerTests::capabilityRoundTrip() {
    const auto all = {Capability::None, Capability::Notify, Capability::DownloadAndOpen,
                      Capability::SelfApply};
    for (const Capability capability : all) {
        const QString name = UpdateManager::capabilityToString(capability);
        QVERIFY(!name.isEmpty());
        QCOMPARE(UpdateManager::capabilityFromString(name), capability);
    }

    // The string vocabulary is the manifest's updateCapability field.
    QCOMPARE(UpdateManager::capabilityToString(Capability::DownloadAndOpen),
             QStringLiteral("DownloadAndOpen"));

    // Q_ENUM registration: the meta-object round-trips the enumerator names too.
    const QMetaEnum meta = QMetaEnum::fromType<Capability>();
    QCOMPARE(meta.keyCount(), 4);
    QCOMPARE(QString::fromLatin1(meta.valueToKey(static_cast<quint64>(Capability::SelfApply))),
             QStringLiteral("SelfApply"));
}

void UpdateManagerTests::unknownCapabilityFailsClosed() {
    // Unknown or differently-cased input must map to the inert dial — the
    // parser can lower, never escalate.
    QCOMPARE(UpdateManager::capabilityFromString(QStringLiteral("Everything")), Capability::None);
    QCOMPARE(UpdateManager::capabilityFromString(QStringLiteral("selfapply")), Capability::None);
    QCOMPARE(UpdateManager::capabilityFromString(QString()), Capability::None);
}

void UpdateManagerTests::inertBuildDoesNothing() {
    UpdateManager manager;
    QSignalSpy stateSpy(&manager, &UpdateManager::stateChanged);
    QSignalSpy errorSpy(&manager, &UpdateManager::errorOccurred);

    // None build: check() never touches the network and never leaves Idle.
    manager.check();
    QCOMPARE(manager.state(), State::Idle);
    QCOMPARE(stateSpy.count(), 0);
    QCOMPARE(errorSpy.count(), 0);

    // download()/apply() fail closed: an inert build may not download, and there
    // is nothing verified to apply.
    manager.download();
    QCOMPARE(manager.state(), State::Error);
    QVERIFY(!manager.lastError().isEmpty());
    QVERIFY(errorSpy.count() >= 1);

    manager.apply();
    QVERIFY(errorSpy.count() >= 2);

    // The effective capability never exceeds the compiled ceiling (None here).
    QCOMPARE(manager.effectiveCapability(), Capability::None);
}

QTEST_MAIN(UpdateManagerTests)
#include "tst_update_manager.moc"
