// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "accounts/mock_accounts_service.h"
#include "connection/mock_connection_service.h"
#include "firstrun/first_run_model.h"
#include "models/imodel_catalog.h"
#include "models/mock_provider_catalog.h"
#include "profiles/mock_profile_store.h"
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

// A connection seam that models an auth-required node: connectTo() drives straight to the
// "authenticating" state + authRequired(), and login() succeeds only for the password "good".
class FakeAuthConnection : public connection::IConnectionService {
public:
    using IConnectionService::IConnectionService;
    void connectTo(const QString&, const QString&, const QString&) override {
        setState(QStringLiteral("authenticating"));
        emit authRequired();
    }
    void disconnect() override { setState(QStringLiteral("offline")); }
    void testConnection(const QString&, const QString&, const QString&) override {}
    void login(const QString&, const QString& password) override {
        if (password == QStringLiteral("good")) {
            emit authenticated();
            setState(QStringLiteral("ready"));
        } else {
            emit authFailed(QStringLiteral("bad password"));
            setState(QStringLiteral("authenticating"));
        }
    }
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

    // An auth-required node drives the gate to the `auth` phase, and the shell never mounts
    // pre-auth: active() stays true throughout connecting/authenticating and only clears at done.
    void authRequiredShowsLoginAndGatesShell() {
        QtSettingsStore settings;
        FakeAuthConnection conn;
        FakeModelCatalog catalog;
        FirstRunModel m(&settings, &conn, &catalog);
        m.begin();
        QVERIFY(m.active());

        conn.connectTo(QStringLiteral("remote"), QStringLiteral("node.example:8443"), QString());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("auth"); }, 3000));
        QVERIFY(m.active()); // shell still gated - no surface mounted pre-auth
        QVERIFY(m.error().isEmpty());
    }

    // Wrong password keeps the auth phase with the error; the gate never advances / mounts.
    void wrongPasswordStaysInAuth() {
        QtSettingsStore settings;
        FakeAuthConnection conn;
        FakeModelCatalog catalog;
        FirstRunModel m(&settings, &conn, &catalog);
        m.begin();
        conn.connectTo(QStringLiteral("remote"), QStringLiteral("node.example:8443"), QString());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("auth"); }, 3000));

        m.submitLogin(QStringLiteral("user"), QStringLiteral("WRONG"));
        QVERIFY(QTest::qWaitFor(
            [&] {
                return m.phase() == QStringLiteral("auth") &&
                       m.error() == QStringLiteral("bad password");
            },
            3000));
        QVERIFY(m.active());
    }

    // Correct password authenticates and advances to the inference gate.
    void correctPasswordAdvancesToInference() {
        QtSettingsStore settings;
        FakeAuthConnection conn;
        FakeModelCatalog catalog;
        FirstRunModel m(&settings, &conn, &catalog);
        m.begin();
        conn.connectTo(QStringLiteral("remote"), QStringLiteral("node.example:8443"), QString());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("auth"); }, 3000));

        m.submitLogin(QStringLiteral("user"), QStringLiteral("good"));
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));
    }

    // Guided inference: applyInferenceChoice persists a working profile (the descriptor's
    // ProviderSelector + base URL + the chosen model), makes it default, and finishes - zero env.
    void guidedInferencePersistsProfile() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FakeModelCatalog catalog;
        profiles::MockProfileStore profileStore;
        accounts::MockAccountsService accountsSvc;
        models::MockProviderCatalog providerCatalog;
        FirstRunModel m(&settings, &conn, &catalog, &profileStore, &accountsSvc, &providerCatalog);
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));

        QSignalSpy finished(&m, &FirstRunModel::finished);
        // Model layer is permissive: an empty key still persists + finishes (requiresKey only
        // drives the QML key-collection UX), so Daemon Cloud with a chosen model finishes here.
        m.applyInferenceChoice(QStringLiteral("daemon_cloud"),
                               QStringLiteral("anthropic/claude-3.5-sonnet"), QString());
        QCOMPARE(m.phase(), QStringLiteral("done"));
        QCOMPARE(finished.count(), 1);

        const QString def = profileStore.defaultProfileId();
        QVERIFY(!def.isEmpty());
        const QVariantMap p = profileStore.profile(def);
        // Daemon Cloud maps to the daemon_api ProviderSelector + its base URL; the model is the
        // node-offered id, persisted verbatim.
        QCOMPARE(p.value(QStringLiteral("provider")).toString(), QStringLiteral("daemon_api"));
        QCOMPARE(p.value(QStringLiteral("model")).toString(),
                 QStringLiteral("anthropic/claude-3.5-sonnet"));
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
