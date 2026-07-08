// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "models/iprovider_catalog.h"
#include "profiles/iprofile_store.h"
#include "setup/agent_setup_model.h"

#include <QHash>
#include <QSignalSpy>
#include <QtTest/QtTest>

using setup::AgentSetupModel;

namespace {

// A controllable provider catalog: the test sets the descriptors (incl. requiresKey) and decides
// how many models each provider's authenticated listing resolves with - resolveModels(id, n)
// stands in for "a ProviderModels reply landed", the seam the key gate + commit probe are driven
// off. Mirrors the FakeProviderCatalog in tst_first_run_model.
class FakeProviderCatalog : public models::IProviderCatalog {
public:
    QVariantList descriptors;
    QHash<QString, QVariantList> offered;

    static QVariantMap descriptor(const QString& id, const QString& name, bool requiresKey) {
        QVariantMap d;
        d[QStringLiteral("id")] = id;
        d[QStringLiteral("name")] = name;
        d[QStringLiteral("requiresKey")] = requiresKey;
        d[QStringLiteral("wireSelector")] = id;
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

// A recording profile store: captures the commit payload (which create path + the foreign-backend
// map) and reflects mutations synchronously so the commit continuations complete in-line.
class FakeProfileStore : public profiles::IProfileStore {
public:
    QHash<QString, QVariantMap> rows;
    QString defaultId;
    // Recorded last-commit projections.
    QString lastForeignName;
    QString lastForeignAgent;
    QVariantMap lastForeignBackend;
    bool usedBackendCreate = false;
    bool usedPlainForeignCreate = false;

    [[nodiscard]] QObject* profiles() const override { return nullptr; }
    [[nodiscard]] QString defaultProfileId() const override { return defaultId; }
    [[nodiscard]] QVariantMap profile(const QString& id) const override { return rows.value(id); }
    [[nodiscard]] QStringList profileNames() const override { return rows.keys(); }
    [[nodiscard]] QVariantList availableSkills() const override { return {}; }
    [[nodiscard]] QVariantList availableTools() const override { return {}; }

    QString createProfile(const QString& name) override {
        QVariantMap row;
        row[QStringLiteral("id")] = name;
        rows.insert(name, row);
        emit changed();
        return name;
    }
    QString cloneProfile(const QString&, const QString& newId) override {
        return createProfile(newId);
    }
    void updateProfile(const QString& id, const QVariantMap& fields) override {
        QVariantMap row = rows.value(id);
        row[QStringLiteral("id")] = id;
        for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
            row.insert(it.key(), it.value());
        }
        rows.insert(id, row);
        emit changed();
    }
    void remove(const QString& id) override {
        rows.remove(id);
        emit changed();
    }
    void setDefault(const QString& id) override {
        defaultId = id;
        emit changed();
    }
    void refresh() override { emit changed(); }

    QString createForeignProfile(const QString& name, const QString& agent) override {
        usedPlainForeignCreate = true;
        lastForeignName = name;
        lastForeignAgent = agent;
        return createProfile(name);
    }
    QString createForeignProfileWithBackend(const QString& name, const QString& agent,
                                            const QVariantMap& backend) override {
        usedBackendCreate = true;
        lastForeignName = name;
        lastForeignAgent = agent;
        lastForeignBackend = backend;
        return createProfile(name);
    }
};

} // namespace

// Guards the shared setup pipeline predicates and the commit payload projection: the engine/backend
// fork (Core, Foreign+AgentNative, Foreign+NodeProvider) drives needsInference / needsBackendChoice
// / needsKey / gatewayRequired / commitReady, and a NodeProvider commit persists the right
// foreign_backend.
class TestAgentSetupModel : public QObject {
    Q_OBJECT

private slots:
    // Core: the inference sub-form is required, no backend choice, no gateway. A keyless provider
    // with provider+model is commit-ready.
    void coreNeedsInferenceNoBackend() {
        FakeProviderCatalog providers;
        providers.descriptors = {FakeProviderCatalog::descriptor(
            QStringLiteral("daemon_api"), QStringLiteral("Daemon API"), false)};
        AgentSetupModel m(nullptr, nullptr, &providers, nullptr);
        m.chooseEngine(QStringLiteral("Core"));
        QVERIFY(m.needsInference());
        QVERIFY(!m.needsBackendChoice());
        QVERIFY(!m.gatewayRequired());
        QVERIFY(m.engineReady());
        QVERIFY(!m.commitReady()); // no provider/model yet

        m.setInference(QStringLiteral("daemon_api"), QStringLiteral("gpt-x"));
        QVERIFY(!m.needsKey()); // keyless provider
        QVERIFY(m.inferenceReady());
        QVERIFY(m.commitReady());
    }

    // Core with a key-requiring provider: needsKey blocks inference/commit until the key is proven
    // by a non-empty authenticated listing.
    void coreKeyRequiringProviderGatesCommit() {
        FakeProviderCatalog providers;
        providers.descriptors = {
            FakeProviderCatalog::descriptor(QStringLiteral("acme"), QStringLiteral("Acme"), true)};
        AgentSetupModel m(nullptr, nullptr, &providers, nullptr);
        m.chooseEngine(QStringLiteral("Core"));
        m.setInference(QStringLiteral("acme"), QStringLiteral("acme-model"));
        QVERIFY(m.needsKey());
        QVERIFY(!m.inferenceReady());
        QVERIFY(!m.commitReady());

        // Report the selection + a proving listing.
        m.setInferenceSelection(QStringLiteral("acme"), QStringLiteral("sk-good"));
        providers.resolveModels(QStringLiteral("acme"), 2);
        QVERIFY(m.keyGatePassed());
        QVERIFY(!m.needsKey());
        QVERIFY(m.inferenceReady());
        QVERIFY(m.commitReady());
    }

    // Foreign + AgentNative: a backend choice IS needed but NO inference/key/gateway - the agent
    // brings its own model. Commit-ready as soon as an agent is chosen.
    void foreignAgentNativeNeedsNoInference() {
        AgentSetupModel m(nullptr, nullptr, nullptr, nullptr);
        m.chooseEngine(QStringLiteral("Foreign"), QStringLiteral("claude-code"));
        m.chooseBackend(QStringLiteral("AgentNative"));
        QVERIFY(m.needsBackendChoice());
        QVERIFY(!m.needsInference());
        QVERIFY(!m.needsKey());
        QVERIFY(!m.gatewayRequired());
        QVERIFY(m.inferenceReady()); // no inference required
        QVERIFY(m.engineReady());
        QVERIFY(m.commitReady());
    }

    // Foreign + NodeProvider: inference IS required (the node routes through its gateway), and the
    // commit is gated on the gateway being enabled.
    void foreignNodeProviderNeedsInferenceAndGateway() {
        FakeProviderCatalog providers;
        providers.descriptors = {FakeProviderCatalog::descriptor(QStringLiteral("openai"),
                                                                 QStringLiteral("OpenAI"), false)};
        AgentSetupModel m(nullptr, nullptr, &providers, nullptr);
        m.chooseEngine(QStringLiteral("Foreign"), QStringLiteral("codex"));
        m.chooseBackend(QStringLiteral("NodeProvider"));
        QVERIFY(m.needsBackendChoice());
        QVERIFY(m.needsInference());
        m.setInference(QStringLiteral("openai"), QStringLiteral("gpt-5.3"));
        // Gateway disabled by default -> required -> not commit-ready even with provider+model.
        QVERIFY(m.gatewayRequired());
        QVERIFY(!m.commitReady());

        m.setGatewayEnabled(true);
        QVERIFY(!m.gatewayRequired());
        QVERIFY(m.commitReady());
    }

    // engineChoices = the Native row + the injected foreign catalog rows, each carrying the
    // node-derived verification verdict verbatim and a `disabled` flag for NotInstalled agents.
    void engineChoicesComposeNativeAndForeign() {
        AgentSetupModel m(nullptr, nullptr, nullptr, nullptr);
        QVariantList agents;
        auto row = [](const QString& name, const QString& verification, bool installed) {
            QVariantMap r;
            r[QStringLiteral("name")] = name;
            r[QStringLiteral("protocol")] = QStringLiteral("Acp");
            r[QStringLiteral("installed")] = installed;
            r[QStringLiteral("verification")] = verification;
            return QVariant(r);
        };
        agents << row(QStringLiteral("claude-code"), QStringLiteral("Verified"), true)
               << row(QStringLiteral("goose"), QStringLiteral("NotInstalled"), false);
        QSignalSpy spy(&m, &AgentSetupModel::engineChoicesChanged);
        m.setForeignAgents(agents);
        QCOMPARE(spy.count(), 1);

        const QVariantList choices = m.engineChoices();
        QCOMPARE(choices.size(), 3);
        QCOMPARE(choices.at(0).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("Core"));
        const QVariantMap claude = choices.at(1).toMap();
        QCOMPARE(claude.value(QStringLiteral("kind")).toString(), QStringLiteral("Foreign"));
        QCOMPARE(claude.value(QStringLiteral("verification")).toString(),
                 QStringLiteral("Verified"));
        QCOMPARE(claude.value(QStringLiteral("disabled")).toBool(), false);
        const QVariantMap goose = choices.at(2).toMap();
        QCOMPARE(goose.value(QStringLiteral("verification")).toString(),
                 QStringLiteral("NotInstalled"));
        QCOMPARE(goose.value(QStringLiteral("disabled")).toBool(), true); // not installed -> gated
    }

    // A NodeProvider commit projects a foreign profile carrying foreign_backend = NodeProvider with
    // the chosen provider+model (the shared sub-form routed through the node gateway).
    void nodeProviderCommitProjectsForeignBackend() {
        FakeProviderCatalog providers;
        providers.descriptors = {FakeProviderCatalog::descriptor(QStringLiteral("openai"),
                                                                 QStringLiteral("OpenAI"), false)};
        FakeProfileStore store;
        AgentSetupModel m(&store, nullptr, &providers, nullptr);
        m.setGatewayEnabled(true);
        m.chooseEngine(QStringLiteral("Foreign"), QStringLiteral("codex"));
        m.chooseBackend(QStringLiteral("NodeProvider"));
        m.setInference(QStringLiteral("openai"), QStringLiteral("gpt-5.3"));
        QVERIFY(m.commitReady());

        QSignalSpy committed(&m, &AgentSetupModel::committed);
        m.commit(QStringLiteral("coder"));
        QCOMPARE(committed.count(), 1);
        QVERIFY(store.usedBackendCreate);
        QVERIFY(!store.usedPlainForeignCreate);
        QCOMPARE(store.lastForeignAgent, QStringLiteral("codex"));
        QCOMPARE(store.lastForeignBackend.value(QStringLiteral("mode")).toString(),
                 QStringLiteral("NodeProvider"));
        QCOMPARE(store.lastForeignBackend.value(QStringLiteral("provider")).toString(),
                 QStringLiteral("openai"));
        QCOMPARE(store.lastForeignBackend.value(QStringLiteral("model")).toString(),
                 QStringLiteral("gpt-5.3"));
    }

    // An AgentNative commit with no steer hint takes the plain foreign-create path (the node
    // defaults an absent backend to AgentNative{model:null}); with a hint it carries the backend.
    void agentNativeCommitPaths() {
        FakeProfileStore store;
        AgentSetupModel m(&store, nullptr, nullptr, nullptr);
        m.chooseEngine(QStringLiteral("Foreign"), QStringLiteral("claude-code"));
        m.chooseBackend(QStringLiteral("AgentNative"));
        QSignalSpy committed(&m, &AgentSetupModel::committed);
        m.commit(QStringLiteral("claude"));
        QCOMPARE(committed.count(), 1);
        QVERIFY(store.usedPlainForeignCreate);
        QVERIFY(!store.usedBackendCreate);
        QCOMPARE(store.lastForeignAgent, QStringLiteral("claude-code"));

        FakeProfileStore store2;
        AgentSetupModel m2(&store2, nullptr, nullptr, nullptr);
        m2.chooseEngine(QStringLiteral("Foreign"), QStringLiteral("claude-code"));
        m2.chooseBackend(QStringLiteral("AgentNative"));
        m2.setAgentNativeModel(QStringLiteral("sonnet"));
        m2.commit(QStringLiteral("claude"));
        QVERIFY(store2.usedBackendCreate);
        QCOMPARE(store2.lastForeignBackend.value(QStringLiteral("mode")).toString(),
                 QStringLiteral("AgentNative"));
        QCOMPARE(store2.lastForeignBackend.value(QStringLiteral("model")).toString(),
                 QStringLiteral("sonnet"));
    }

    // A Core commit through a proven provider probes the listing before signalling committed.
    void coreCommitProbesThenCommits() {
        FakeProviderCatalog providers;
        providers.descriptors = {
            FakeProviderCatalog::descriptor(QStringLiteral("acme"), QStringLiteral("Acme"), false)};
        FakeProfileStore store;
        store.defaultId = QStringLiteral("seed");
        store.rows.insert(QStringLiteral("seed"),
                          QVariantMap{{QStringLiteral("id"), QStringLiteral("seed")}});
        AgentSetupModel m(&store, nullptr, &providers, nullptr);
        m.chooseEngine(QStringLiteral("Core"));
        m.setInference(QStringLiteral("acme"), QStringLiteral("acme-model"));

        // Empty listing -> the probe fails, no commit.
        QSignalSpy committed(&m, &AgentSetupModel::committed);
        QSignalSpy failed(&m, &AgentSetupModel::failed);
        m.commit(QStringLiteral("seed")); // configure the placeholder in place
        QCOMPARE(committed.count(), 0);
        QCOMPARE(failed.count(), 1);

        // The provider recovers: a re-commit proves out and commits.
        providers.resolveModels(QStringLiteral("acme"), 2);
        m.commit(QStringLiteral("seed"));
        QCOMPARE(committed.count(), 1);
    }
};

QTEST_MAIN(TestAgentSetupModel)
#include "tst_agent_setup_model.moc"
