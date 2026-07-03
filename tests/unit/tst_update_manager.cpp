// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/update_manager.h"

#include <QRegularExpression>
#include <QtTest>

using update::UpdateManager;
using Capability = update::UpdateManager::Capability;
using State = update::UpdateManager::State;

// Locks the UpdateManager scaffold (packaging/UPDATES.md): the package-time
// capability dial defaults to None (this test tree sets no dial), the
// capability vocabulary round-trips with unknown input failing CLOSED to None,
// and the descoped check/download/apply stubs report not-implemented instead
// of pretending to work.
class UpdateManagerTests : public QObject {
    Q_OBJECT

private slots:
    void defaults();
    void capabilityRoundTrip();
    void unknownCapabilityFailsClosed();
    void stubsReportNotImplemented();
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

void UpdateManagerTests::stubsReportNotImplemented() {
    UpdateManager manager;
    QSignalSpy stateSpy(&manager, &UpdateManager::stateChanged);
    QSignalSpy errorSpy(&manager, &UpdateManager::errorOccurred);

    manager.check();
    QCOMPARE(manager.state(), State::Error);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(manager.lastError().contains(QStringLiteral("not implemented")));
    QVERIFY(errorSpy.last().first().toString().contains(QStringLiteral("check")));

    // download()/apply() are equally honest; the state stays Error (deduped)
    // while every call still reports its own error.
    manager.download();
    manager.apply();
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 3);
    QVERIFY(errorSpy.at(1).first().toString().contains(QStringLiteral("download")));
    QVERIFY(errorSpy.at(2).first().toString().contains(QStringLiteral("apply")));
}

QTEST_MAIN(UpdateManagerTests)
#include "tst_update_manager.moc"
