// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "accounts/mock_accounts_service.h"
#include "cache_test_support.h"
#include "connection/mock_connection_service.h"
#include "firstrun/first_run_model.h"
#include "models/imodel_catalog.h"
#include "models/iprovider_catalog.h"
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
    [[nodiscard]] QString installedIdFor(const QString&, const QString&) const override {
        return {};
    }
    void dismissDownload(const QString&) override {}
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

// A controllable provider catalog for the FIX 4 key-gate tests: the test sets the descriptors and
// decides how many models each provider's (authenticated) LIST resolves with —
// resolveModels(id, n) is "a ProviderModels reply landed", the seam the gate is driven off.
class FakeProviderCatalog : public models::IProviderCatalog {
public:
    QVariantList descriptors;
    QHash<QString, QVariantList> offered;

    static QVariantMap descriptor(const QString& id, const QString& name, bool requiresKey) {
        QVariantMap d;
        d[QStringLiteral("id")] = id;
        d[QStringLiteral("name")] = name;
        d[QStringLiteral("requiresKey")] = requiresKey;
        return d;
    }
    [[nodiscard]] QVariantList providers() const override { return descriptors; }
    [[nodiscard]] QVariantMap descriptorFor(const QString& providerId) const override {
        for (const QVariant& v : descriptors) {
            const QVariantMap d = v.toMap();
            if (d.value(QStringLiteral("id")).toString() == providerId) {
                return d;
            }
        }
        return {};
    }
    void refreshModels(const QString& providerId, const QString& = QString(),
                       const QString& = QString()) override {
        emit offeredModelsChanged(providerId);
    }
    [[nodiscard]] QVariantList offeredModels(const QString& providerId) const override {
        return offered.value(providerId);
    }
    void resolveModels(const QString& providerId, int count) {
        QVariantList rows;
        for (int i = 0; i < count; ++i) {
            QVariantMap row;
            row[QStringLiteral("id")] = QStringLiteral("m-%1").arg(i);
            rows.append(row);
        }
        offered.insert(providerId, rows);
        emit offeredModelsChanged(providerId);
    }
};

// A profile store that can mint foreign (ACP) agents (CON-16), recording the reference it was
// asked to create — the mock default returns "" (cannot create foreign agents).
class AcpCapableProfileStore : public profiles::MockProfileStore {
public:
    using MockProfileStore::MockProfileStore;
    QString lastAcpName;
    QString lastAcpAgent;
    QString createAcpProfile(const QString& name, const QString& agent) override {
        lastAcpName = name;
        lastAcpAgent = agent;
        return createProfile(name);
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
        // Each case starts from the pristine mock seeds (the profile mock persists mutations to
        // its AppData cache) and an un-completed setup.
        resetMockCache();
        QtSettingsStore s;
        s.setSetupComplete(false);
    }

    // The A2 wizard gate is node-authoritative: it only runs while the ACTIVE profile is
    // unconfigured (empty model), which is exactly the node-seeded placeholder state. Blank the
    // mock's active profile so the gate opens like on a fresh node.
    static void makeActiveProfileUnconfigured(profiles::MockProfileStore& store) {
        QVariantMap fields;
        fields[QStringLiteral("model")] = QString();
        store.updateProfile(store.defaultProfileId(), fields);
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

    // A2 (node-authoritative gate): a live connection alone is NOT "setup complete" — persisting
    // it on ready would let a quit mid-wizard skip the wizard forever on the next launch. Only
    // finishing (or skipping) the inference step marks setup complete.
    void connectionSuccessDoesNotPersistSetupComplete() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();
        QVERIFY(!settings.setupComplete());

        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));
        // Still gated: only the finished/skipped inference step persists setupComplete.
        QVERIFY(!settings.setupComplete());
        m.completeInference(); // no catalog wired -> permissive readiness
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
        makeActiveProfileUnconfigured(profileStore); // fresh-node state: the A2 gate must open
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

    // W2: a chosen agent NAME mints a fresh named profile (one full-spec create), makes it the
    // default, and deletes the seeded placeholder — the wizard names the agent instead of leaving
    // the node's placeholder id. finished() fires exactly once.
    void guidedInferenceWithNameCreatesNamedAgentAndDropsPlaceholder() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FakeModelCatalog catalog;
        profiles::MockProfileStore profileStore;
        makeActiveProfileUnconfigured(profileStore); // fresh-node state: the A2 gate must open
        accounts::MockAccountsService accountsSvc;
        models::MockProviderCatalog providerCatalog;
        FirstRunModel m(&settings, &conn, &catalog, &profileStore, &accountsSvc, &providerCatalog);
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));

        const QString placeholder = profileStore.defaultProfileId();
        QVERIFY(!placeholder.isEmpty());

        QSignalSpy finished(&m, &FirstRunModel::finished);
        m.applyInferenceChoice(QStringLiteral("daemon_cloud"),
                               QStringLiteral("anthropic/claude-3.5-sonnet"), QString(),
                               QStringLiteral("anthropic"));
        QCOMPARE(m.phase(), QStringLiteral("done"));
        QCOMPARE(finished.count(), 1);

        // The default is a NEW profile named by the wizard, carrying the chosen spec.
        const QString def = profileStore.defaultProfileId();
        QVERIFY(!def.isEmpty());
        QVERIFY(def != placeholder);
        const QVariantMap p = profileStore.profile(def);
        QCOMPARE(p.value(QStringLiteral("name")).toString(), QStringLiteral("anthropic"));
        QCOMPARE(p.value(QStringLiteral("provider")).toString(), QStringLiteral("daemon_api"));
        QCOMPARE(p.value(QStringLiteral("model")).toString(),
                 QStringLiteral("anthropic/claude-3.5-sonnet"));
        // The seeded placeholder is gone (no ghost agent in the Fleet).
        QVERIFY(profileStore.profile(placeholder).isEmpty());
    }

    // W2: a chosen name EQUAL to the placeholder id configures it in place (no create/delete),
    // exactly like the nameless (TUI) path.
    void guidedInferenceWithPlaceholderNameUpdatesInPlace() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        FakeModelCatalog catalog;
        profiles::MockProfileStore profileStore;
        makeActiveProfileUnconfigured(profileStore); // fresh-node state: the A2 gate must open
        accounts::MockAccountsService accountsSvc;
        models::MockProviderCatalog providerCatalog;
        FirstRunModel m(&settings, &conn, &catalog, &profileStore, &accountsSvc, &providerCatalog);
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));

        const QString placeholder = profileStore.defaultProfileId();
        const qsizetype rowsBefore = profileStore.profileNames().size();
        m.applyInferenceChoice(QStringLiteral("daemon_cloud"),
                               QStringLiteral("anthropic/claude-3.5-sonnet"), QString(),
                               placeholder);
        QCOMPARE(m.phase(), QStringLiteral("done"));
        QCOMPARE(profileStore.defaultProfileId(), placeholder);
        QCOMPARE(profileStore.profileNames().size(), rowsBefore);
        QCOMPARE(profileStore.profile(placeholder).value(QStringLiteral("model")).toString(),
                 QStringLiteral("anthropic/claude-3.5-sonnet"));
    }

    // FIX 4 (hoisted key gate): permissive when no provider catalog is wired, before any
    // selection is reported, and for a keyless provider - the gate only ever blocks a
    // key-requiring selection.
    void keyGatePermissiveByDefaultAndForKeylessProviders() {
        QtSettingsStore settings;
        MockConnectionService conn;
        FirstRunModel bare(&settings, &conn); // no catalog: the mock/standalone path stays open
        QVERIFY(bare.keyGatePassed());

        FakeProviderCatalog catalog;
        catalog.descriptors = {FakeProviderCatalog::descriptor(QStringLiteral("llama_cpp"),
                                                               QStringLiteral("Llama"), false)};
        FirstRunModel m(&settings, &conn, nullptr, nullptr, nullptr, &catalog);
        QVERIFY(m.keyGatePassed()); // nothing reported yet
        m.setInferenceSelection(QStringLiteral("llama_cpp"), QString());
        QVERIFY(m.keyGatePassed()); // keyless provider never blocks
        catalog.resolveModels(QStringLiteral("llama_cpp"), 0);
        QVERIFY(m.keyGatePassed()); // even an empty local listing is not a key failure
        QVERIFY(m.keyGateMessage().isEmpty());
    }

    // FIX 4: a key-requiring provider blocks Finish until the entered key is PROVEN by a
    // non-empty authenticated LIST; an empty LIST blocks with an actionable vendor message; a
    // corrected key clears the message and passes on the next resolution.
    void keyGateBlocksThenPassesOnAuthenticatedList() {
        QtSettingsStore settings;
        MockConnectionService conn;
        FakeProviderCatalog catalog;
        catalog.descriptors = {
            FakeProviderCatalog::descriptor(QStringLiteral("acme"), QStringLiteral("Acme"), true)};
        FirstRunModel m(&settings, &conn, nullptr, nullptr, nullptr, &catalog);
        QSignalSpy gateChanged(&m, &FirstRunModel::keyGateChanged);

        m.setInferenceSelection(QStringLiteral("acme"), QString());
        QVERIFY(!m.keyGatePassed()); // requires a key; none entered
        catalog.resolveModels(QStringLiteral("acme"), 3);
        QVERIFY(!m.keyGatePassed()); // a keyless listing proves nothing
        QVERIFY(m.keyGateMessage().isEmpty());

        m.setInferenceSelection(QStringLiteral("acme"), QStringLiteral("sk-bad"));
        QVERIFY(!m.keyGatePassed());                      // typed but not yet proven
        catalog.resolveModels(QStringLiteral("acme"), 0); // authenticated LIST came back empty
        QVERIFY(!m.keyGatePassed());
        QVERIFY(m.keyGateMessage().contains(QStringLiteral("Acme")));

        m.setInferenceSelection(QStringLiteral("acme"), QStringLiteral("sk-good"));
        QVERIFY(m.keyGateMessage().isEmpty()); // editing the key clears the stale reason
        catalog.resolveModels(QStringLiteral("acme"), 2);
        QVERIFY(m.keyGatePassed());
        QVERIFY(m.keyGateMessage().isEmpty());
        QVERIFY(gateChanged.count() >= 5); // every re-arm + every resolution notifies bindings
    }

    // FIX 4: the gate re-arms on ANY selection report (provider switch, key edit, even a
    // same-value re-selection) - a previously proven key must re-prove - and resolutions for a
    // provider other than the reported one are ignored.
    void keyGateRearmsOnSelectionChange() {
        QtSettingsStore settings;
        MockConnectionService conn;
        FakeProviderCatalog catalog;
        catalog.descriptors = {
            FakeProviderCatalog::descriptor(QStringLiteral("acme"), QStringLiteral("Acme"), true),
            FakeProviderCatalog::descriptor(QStringLiteral("beta"), QStringLiteral("Beta"), true)};
        FirstRunModel m(&settings, &conn, nullptr, nullptr, nullptr, &catalog);

        m.setInferenceSelection(QStringLiteral("acme"), QStringLiteral("sk-1"));
        catalog.resolveModels(QStringLiteral("acme"), 1);
        QVERIFY(m.keyGatePassed());

        // Re-selecting the same provider re-arms (the QML picker reset on every activation).
        m.setInferenceSelection(QStringLiteral("acme"), QStringLiteral("sk-1"));
        QVERIFY(!m.keyGatePassed());
        catalog.resolveModels(QStringLiteral("acme"), 1);
        QVERIFY(m.keyGatePassed());

        // Switching provider re-arms; another provider's resolution must not satisfy it.
        m.setInferenceSelection(QStringLiteral("beta"), QStringLiteral("sk-2"));
        QVERIFY(!m.keyGatePassed());
        catalog.resolveModels(QStringLiteral("acme"), 5);
        QVERIFY(!m.keyGatePassed());
        catalog.resolveModels(QStringLiteral("beta"), 1);
        QVERIFY(m.keyGatePassed());
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

    // --- CON-16: the agent-type step (app-wizard-auth stream) --------------------------------

    // The agent-type step fronts the inference step ONLY when foreign agents were offered; the
    // native choice continues into the unchanged inference flow.
    void agentTypeStepOfferedRoutesBeforeInference() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        profiles::MockProfileStore profileStore;
        makeActiveProfileUnconfigured(profileStore);
        FirstRunModel m(&settings, &conn, nullptr, &profileStore);
        m.setAgentTypeOffered(true); // the graph wires this from the ACP catalog reflection
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("agenttype"); }, 3000));

        // Native: continue into the existing inference step (CON-10..14 unchanged).
        m.chooseAgentType(QString());
        QCOMPARE(m.phase(), QStringLiteral("inference"));
    }

    // An empty catalog keeps zero foreign noise: the wizard routes straight to inference.
    void agentTypeStepSkippedWhenNotOffered() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        profiles::MockProfileStore profileStore;
        makeActiveProfileUnconfigured(profileStore);
        FirstRunModel m(&settings, &conn, nullptr, &profileStore);
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));
    }

    // A foreign choice commits ONE named ProfileCreate carrying engine=Acp{agent} — no
    // provider/model/credential frames — makes it the default, drops the seeded placeholder,
    // and finishes exactly once.
    void agentTypeForeignChoiceCommitsAcpProfileAndFinishes() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        AcpCapableProfileStore profileStore;
        makeActiveProfileUnconfigured(profileStore);
        const QString placeholder = profileStore.defaultProfileId();
        FirstRunModel m(&settings, &conn, nullptr, &profileStore);
        m.setAgentTypeOffered(true);
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("agenttype"); }, 3000));

        QSignalSpy finished(&m, &FirstRunModel::finished);
        m.applyAcpChoice(QStringLiteral("gem"), QStringLiteral("gemini"));
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("done"); }, 3000));
        QCOMPARE(finished.count(), 1);
        QVERIFY(settings.setupComplete());
        QCOMPARE(profileStore.lastAcpName, QStringLiteral("gem"));
        QCOMPARE(profileStore.lastAcpAgent, QStringLiteral("gemini"));
        // The new agent owns the default; the seeded placeholder is gone.
        const QString def = profileStore.defaultProfileId();
        QVERIFY(!def.isEmpty());
        QVERIFY(def != placeholder);
        QVERIFY(profileStore.profile(placeholder).isEmpty());
    }

    // --- A7 (CON-7): the finish gate is PROVEN, not assumed ----------------------------------

    // An empty ProviderModels listing through the committed provider keeps the wizard open with
    // an actionable error; a later successful listing lets the re-committed Finish complete.
    void finishProbeBlocksOnEmptyListingThenPasses() {
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        QtSettingsStore settings;
        MockConnectionService conn;
        profiles::MockProfileStore profileStore;
        makeActiveProfileUnconfigured(profileStore);
        FakeProviderCatalog providerCatalog;
        providerCatalog.descriptors.append(FakeProviderCatalog::descriptor(
            QStringLiteral("acme"), QStringLiteral("Acme"), /*requiresKey=*/false));
        FirstRunModel m(&settings, &conn, nullptr, &profileStore, nullptr, &providerCatalog);
        m.begin();
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("inference"); }, 3000));

        // The probe's listing resolves EMPTY (the fake echoes its current offered set): the
        // wizard must stay open with the actionable reason, not silently finish.
        m.applyInferenceChoice(QStringLiteral("acme"), QStringLiteral("acme-model"), QString());
        QCOMPARE(m.phase(), QStringLiteral("inference"));
        QVERIFY(!m.error().isEmpty());
        QVERIFY(!settings.setupComplete());

        // The provider recovers (a non-empty listing will now resolve): re-commit finishes.
        providerCatalog.offered.insert(QStringLiteral("acme"),
                                       providerCatalog.offeredModels(QStringLiteral("acme")));
        providerCatalog.resolveModels(QStringLiteral("acme"), 2);
        QSignalSpy finished(&m, &FirstRunModel::finished);
        m.applyInferenceChoice(QStringLiteral("acme"), QStringLiteral("acme-model"), QString());
        QVERIFY(QTest::qWaitFor([&] { return m.phase() == QStringLiteral("done"); }, 3000));
        QCOMPARE(finished.count(), 1);
        QVERIFY(settings.setupComplete());
    }

    // --- CON-8 (A7): send-time re-entry ------------------------------------------------------

    // reenterProvider() re-opens the wizard AT the inference step from a live shell without
    // clearing setupComplete; finishing returns to done and re-persists it.
    void reenterProviderReopensInferenceStep() {
        QtSettingsStore settings;
        settings.setSetupComplete(true);
        MockConnectionService conn;
        FirstRunModel m(&settings, &conn);
        m.begin();
        QCOMPARE(m.phase(), QStringLiteral("done"));

        m.reenterProvider();
        QCOMPARE(m.phase(), QStringLiteral("inference"));
        QVERIFY(m.active());
        QVERIFY(settings.setupComplete()); // untouched: this is a re-entry, not a reset

        m.completeInference(); // permissive (no catalog wired)
        QCOMPARE(m.phase(), QStringLiteral("done"));
        QVERIFY(settings.setupComplete());
    }
};

QTEST_MAIN(TestFirstRunModel)
#include "tst_first_run_model.moc"
