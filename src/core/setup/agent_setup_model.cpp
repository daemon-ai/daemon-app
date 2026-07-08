// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "setup/agent_setup_model.h"

#include "accounts/iaccounts_service.h"
#include "models/imodel_catalog.h"
#include "models/iprovider_catalog.h"
#include "profiles/iprofile_store.h"

#include <memory>
#include <QTimer>

namespace setup {

AgentSetupModel::AgentSetupModel(profiles::IProfileStore* profiles,
                                 accounts::IAccountsService* accounts,
                                 models::IProviderCatalog* providerCatalog,
                                 models::IModelCatalog* modelCatalog, QObject* parent)
    : QObject(parent), m_profiles(profiles), m_accounts(accounts),
      m_providerCatalog(providerCatalog), m_modelCatalog(modelCatalog) {
    if (m_providerCatalog != nullptr) {
        // FIX 4: the key gate resolves off the provider catalog's (authenticated) model listings.
        connect(m_providerCatalog, &models::IProviderCatalog::offeredModelsChanged, this,
                &AgentSetupModel::resolveKeyGate);
        // requiresKey is read live off the descriptor, so a provider-catalog change can flip the
        // gate (and needsKey) without a listing; re-notify bindings.
        connect(m_providerCatalog, &models::IProviderCatalog::providersChanged, this,
                &AgentSetupModel::keyGateChanged);
    }
    rebuildEngineChoices();
}

// --- derived predicates ---------------------------------------------------------------------

bool AgentSetupModel::needsBackendChoice() const {
    return m_engineKind == QStringLiteral("Foreign");
}

bool AgentSetupModel::needsInference() const {
    // THE unification: the shared inference sub-form is needed for a native engine, and for a
    // foreign agent routed through the node provider gateway.
    return m_engineKind == QStringLiteral("Core") ||
           (m_engineKind == QStringLiteral("Foreign") &&
            m_backendMode == QStringLiteral("NodeProvider"));
}

bool AgentSetupModel::providerRequiresKey(const QString& providerId) const {
    if (m_providerCatalog == nullptr || providerId.isEmpty()) {
        return false;
    }
    return m_providerCatalog->descriptorFor(providerId)
        .value(QStringLiteral("requiresKey"))
        .toBool();
}

bool AgentSetupModel::needsKey() const {
    if (!needsInference() || !providerRequiresKey(m_providerId)) {
        return false;
    }
    // Satisfied only once THIS provider's key has been proven by a non-empty authenticated listing.
    return !(m_keyProven && m_gateProviderId == m_providerId);
}

bool AgentSetupModel::gatewayRequired() const {
    return m_engineKind == QStringLiteral("Foreign") &&
           m_backendMode == QStringLiteral("NodeProvider") && !m_gatewayEnabled;
}

bool AgentSetupModel::engineReady() const {
    if (m_engineKind == QStringLiteral("Foreign")) {
        return !m_foreignAgent.isEmpty();
    }
    return m_engineKind == QStringLiteral("Core");
}

bool AgentSetupModel::backendReady() const {
    if (!needsBackendChoice()) {
        return true;
    }
    return m_backendMode == QStringLiteral("AgentNative") ||
           m_backendMode == QStringLiteral("NodeProvider");
}

bool AgentSetupModel::inferenceReady() const {
    if (!needsInference()) {
        return true; // AgentNative: the agent brings its own model
    }
    return !m_providerId.isEmpty() && !m_model.isEmpty() && !needsKey();
}

bool AgentSetupModel::commitReady() const {
    return engineReady() && backendReady() && inferenceReady() && !gatewayRequired();
}

bool AgentSetupModel::keyGatePassed() const {
    // Mirrors the wizard gate this hoists: pass unless the reported provider requires a key that
    // has not been proven since the last re-arm. No catalog / nothing reported stays permissive.
    if (m_providerCatalog == nullptr || m_gateProviderId.isEmpty()) {
        return true;
    }
    return !providerRequiresKey(m_gateProviderId) || m_keyProven;
}

QString AgentSetupModel::protocolFor(const QString& agent) const {
    if (agent.isEmpty()) {
        return {};
    }
    for (const QVariant& v : m_foreignAgentRows) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("name")).toString() == agent) {
            return row.value(QStringLiteral("protocol")).toString();
        }
    }
    return {};
}

QString AgentSetupModel::summary() const {
    const QString protocol = protocolFor(m_foreignAgent);
    const QString modelArg = (m_engineKind == QStringLiteral("Foreign") &&
                              m_backendMode == QStringLiteral("AgentNative"))
                                 ? m_agentNativeModel
                                 : m_model;
    if (m_engineIdentity != nullptr) {
        // Compose through EngineIdentity so the engine/protocol label rules stay in one place.
        QString out;
        const bool ok = QMetaObject::invokeMethod(
            m_engineIdentity, "summaryFor", Q_RETURN_ARG(QString, out),
            Q_ARG(QString, m_engineKind), Q_ARG(QString, m_foreignAgent), Q_ARG(QString, protocol),
            Q_ARG(QString, m_backendMode), Q_ARG(QString, m_providerId), Q_ARG(QString, modelArg));
        if (ok) {
            return out;
        }
    }
    // Fallback (no EngineIdentity injected, e.g. offscreen tests): a minimal identifier
    // composition.
    if (m_engineKind != QStringLiteral("Foreign")) {
        return m_providerId.isEmpty()
                   ? m_model
                   : (m_model.isEmpty() ? m_providerId
                                        : QStringLiteral("%1/%2").arg(m_providerId, m_model));
    }
    QString out = m_foreignAgent;
    if (!modelArg.isEmpty()) {
        out += QStringLiteral(" · %1").arg(modelArg);
    }
    return out;
}

// --- engine choices -------------------------------------------------------------------------

void AgentSetupModel::rebuildEngineChoices() {
    QVariantList choices;
    QVariantMap native;
    native[QStringLiteral("kind")] = QStringLiteral("Core");
    native[QStringLiteral("name")] = QStringLiteral("Native");
    native[QStringLiteral("agent")] = QString();
    native[QStringLiteral("protocol")] = QString();
    native[QStringLiteral("installed")] = true;
    native[QStringLiteral("verification")] = QString();
    native[QStringLiteral("disabled")] = false;
    choices.append(native);
    for (const QVariant& v : m_foreignAgentRows) {
        const QVariantMap in = v.toMap();
        QVariantMap row;
        const QString name = in.value(QStringLiteral("name")).toString();
        row[QStringLiteral("kind")] = QStringLiteral("Foreign");
        row[QStringLiteral("name")] = name;
        row[QStringLiteral("agent")] = name;
        row[QStringLiteral("protocol")] = in.value(QStringLiteral("protocol"));
        row[QStringLiteral("installed")] = in.value(QStringLiteral("installed"));
        // Node-derived verdict, rendered verbatim ("Verified"|"Unverified"|"NotInstalled").
        const QString verification = in.value(QStringLiteral("verification")).toString();
        row[QStringLiteral("verification")] = verification;
        row[QStringLiteral("disabled")] = verification == QStringLiteral("NotInstalled");
        choices.append(row);
    }
    m_engineChoices = choices;
}

void AgentSetupModel::setForeignAgents(const QVariantList& agents) {
    m_foreignAgentRows = agents;
    rebuildEngineChoices();
    emit engineChoicesChanged();
    emit stateChanged(); // the chosen agent's protocol (summary) may now resolve
}

void AgentSetupModel::setEngineIdentity(QObject* engineIdentity) {
    m_engineIdentity = engineIdentity;
    emit stateChanged();
}

// --- intents --------------------------------------------------------------------------------

void AgentSetupModel::chooseEngine(const QString& kind, const QString& agent) {
    if (m_engineLocked) {
        return; // editor mode: an existing profile's engine cannot change
    }
    m_engineKind =
        kind == QStringLiteral("Foreign") ? QStringLiteral("Foreign") : QStringLiteral("Core");
    m_foreignAgent = m_engineKind == QStringLiteral("Foreign") ? agent : QString();
    emit stateChanged();
}

void AgentSetupModel::chooseBackend(const QString& mode) {
    m_backendMode = mode == QStringLiteral("NodeProvider") ? QStringLiteral("NodeProvider")
                                                           : QStringLiteral("AgentNative");
    emit stateChanged();
}

void AgentSetupModel::setInference(const QString& provider, const QString& model,
                                   const QString& key, const QString& baseUrl) {
    m_providerId = provider;
    m_model = model;
    m_inferenceKey = key;
    m_baseUrl = baseUrl;
    emit stateChanged();
}

void AgentSetupModel::setAgentNativeModel(const QString& model) {
    m_agentNativeModel = model;
    emit stateChanged();
}

void AgentSetupModel::setInferenceSelection(const QString& providerId, const QString& key) {
    // Unconditional re-arm - even a same-value re-report resets the proven bit; the next
    // authenticated listing re-proves it.
    m_gateProviderId = providerId;
    m_gateKey = key;
    m_keyProven = false;
    m_keyGateMessage.clear();
    emit keyGateChanged();
}

void AgentSetupModel::resolveKeyGate(const QString& providerId) {
    if (m_providerCatalog == nullptr || providerId.isEmpty() || providerId != m_gateProviderId) {
        return; // not the reported selection's listing
    }
    const QVariantMap descriptor = m_providerCatalog->descriptorFor(providerId);
    if (!descriptor.value(QStringLiteral("requiresKey")).toBool() || m_gateKey.isEmpty()) {
        return; // keyless provider / no key entered yet: nothing to prove
    }
    // The listing was authenticated with the entered key, so a non-empty result proves it (FIX 4).
    const int count = static_cast<int>(m_providerCatalog->offeredModels(providerId).size());
    m_keyProven = count > 0;
    const QString name = descriptor.value(QStringLiteral("name")).toString();
    const QString vendor = name.isEmpty() ? providerId : name;
    m_keyGateMessage =
        m_keyProven
            ? QString()
            : tr("Couldn't verify this API key with %1 — check it and try again.").arg(vendor);
    logKeyValidation(providerId, true, count, m_keyProven);
    emit keyGateChanged();
}

void AgentSetupModel::logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                       bool pass) const {
    qInfo("agentsetup: key validation provider=%s requiresKey=%d models=%d gate=%s",
          qPrintable(provider), requiresKey ? 1 : 0, modelCount, pass ? "pass" : "block");
}

void AgentSetupModel::setGatewayEnabled(bool enabled) {
    if (m_gatewayEnabled == enabled) {
        return;
    }
    m_gatewayEnabled = enabled;
    emit stateChanged();
}

// --- commit ---------------------------------------------------------------------------------

void AgentSetupModel::commit(const QString& name) {
    if (m_applying) {
        return; // a commit continuation is already in flight (double-committed Finish)
    }
    const QString trimmed = name.trimmed();
    if (m_engineKind == QStringLiteral("Foreign")) {
        if (m_profiles == nullptr || m_foreignAgent.isEmpty()) {
            finishCommit(QString()); // permissive path (no store / nothing chosen)
            return;
        }
        m_applying = true;
        // Refetch-then-apply: the placeholder id must come off a FRESH ProfileList reflection.
        runOnNextProfilesChanged([this, trimmed] { applyReflectedForeign(trimmed); });
        m_profiles->refresh();
        return;
    }
    if (m_profiles == nullptr || m_providerId.isEmpty()) {
        finishCommit(QString()); // permissive path
        return;
    }
    m_applying = true;
    runOnNextProfilesChanged([this, trimmed] { applyReflectedCore(trimmed); });
    m_profiles->refresh();
}

void AgentSetupModel::applyReflectedCore(const QString& name) {
    // Resolve the ProviderSelector + base URL to persist from the descriptor; a custom endpoint
    // carries no descriptor - pin genai's OpenAI-compatible adapter at the user-supplied base URL.
    QString wireSelector = QStringLiteral("daemon_api");
    QString baseUrl = m_baseUrl.trimmed();
    if (baseUrl.isEmpty() && m_providerCatalog != nullptr) {
        const QVariantMap d = m_providerCatalog->descriptorFor(m_providerId);
        if (!d.isEmpty()) {
            wireSelector = d.value(QStringLiteral("wireSelector")).toString();
            baseUrl = d.value(QStringLiteral("defaultBaseUrl")).toString();
        }
    }
    const QString profileId = m_profiles->defaultProfileId();
    QVariantMap fields;
    fields[QStringLiteral("provider")] = wireSelector;
    fields[QStringLiteral("model")] = m_model;
    fields[QStringLiteral("baseUrl")] = baseUrl;

    if (name.isEmpty() || name == profileId) {
        // Configure the node-seeded placeholder in place (also the nameless TUI path).
        if (!profileId.isEmpty()) {
            m_profiles->updateProfile(profileId, fields);
            if (!m_inferenceKey.isEmpty() && m_accounts != nullptr) {
                m_accounts->addApiKeyForProfile(profileId, m_providerId, QString(), m_inferenceKey,
                                                QString());
            }
            activateLocalModel(m_model, profileId);
        }
        probeThenCommit(m_providerId, profileId, profileId);
        return;
    }

    // Named agent: ONE full-spec ProfileCreate under the chosen name, sequenced on the node's
    // reflections (a pipelined ProfileSelect before the create reflects could race it).
    const QString newId = m_profiles->createProfileWithSpec(name, fields);
    if (newId.isEmpty()) {
        finishCommit(QString()); // store without a live repo: nothing further to sequence
        return;
    }
    whenProfilesReflect(
        [this, newId] { return !m_profiles->profile(newId).isEmpty(); },
        [this, newId, profileId] {
            if (!m_inferenceKey.isEmpty() && m_accounts != nullptr) {
                m_accounts->addApiKeyForProfile(newId, m_providerId, QString(), m_inferenceKey,
                                                QString());
            }
            activateLocalModel(m_model, newId);
            m_profiles->setDefault(newId);
            if (!profileId.isEmpty() && profileId != newId) {
                m_profiles->remove(profileId); // drop the seeded placeholder (no sessions bind it)
            }
            whenProfilesReflect([this, newId] { return m_profiles->defaultProfileId() == newId; },
                                [this, newId] { probeThenCommit(m_providerId, newId, newId); });
        });
}

void AgentSetupModel::applyReflectedForeign(const QString& name) {
    const QString profileId = m_profiles->defaultProfileId();
    const QString agentName = name.isEmpty() ? m_foreignAgent : name;
    // The foreign-backend the profile persists (wire v30). AgentNative w/o a hint carries no
    // explicit backend (the node defaults absent -> AgentNative{model:null}); NodeProvider carries
    // the node provider+model (+ optional credential ref), the credential staying node-side.
    QVariantMap backend;
    if (m_backendMode == QStringLiteral("NodeProvider")) {
        backend[QStringLiteral("mode")] = QStringLiteral("NodeProvider");
        backend[QStringLiteral("provider")] = m_providerId;
        backend[QStringLiteral("model")] = m_model;
        if (!m_credentialRef.isEmpty()) {
            backend[QStringLiteral("credentialRef")] = m_credentialRef;
        }
    } else if (!m_agentNativeModel.isEmpty()) {
        backend[QStringLiteral("mode")] = QStringLiteral("AgentNative");
        backend[QStringLiteral("model")] = m_agentNativeModel;
    }
    // agentName is the display name; m_foreignAgent is the catalog agent id (not swapped despite
    // the token similarity the heuristic check flags).
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    const QString newId =
        backend.isEmpty()
            ? m_profiles->createForeignProfile(agentName, m_foreignAgent)
            : m_profiles->createForeignProfileWithBackend(agentName, m_foreignAgent, backend);
    if (newId.isEmpty()) {
        finishCommit(QString()); // store without a live repo: nothing further to sequence
        return;
    }
    whenProfilesReflect([this, newId] { return !m_profiles->profile(newId).isEmpty(); },
                        [this, newId, profileId] {
                            if (m_backendMode == QStringLiteral("NodeProvider") &&
                                !m_inferenceKey.isEmpty() && m_accounts != nullptr) {
                                // NodeProvider: the gateway resolves the credential node-side,
                                // referenced by the profile - store the entered key profile-scoped
                                // so inference can look it up.
                                m_accounts->addApiKeyForProfile(newId, m_providerId, QString(),
                                                                m_inferenceKey, QString());
                            }
                            m_profiles->setDefault(newId);
                            if (!profileId.isEmpty() && profileId != newId) {
                                m_profiles->remove(profileId);
                            }
                            // No inference probe: the agent brings its own model (AgentNative) or
                            // the node routes it through the gateway (NodeProvider); the node
                            // resolves the recipe at session open.
                            whenProfilesReflect(
                                [this, newId] { return m_profiles->defaultProfileId() == newId; },
                                [this, newId] { finishCommit(newId); });
                        });
}

void AgentSetupModel::probeThenCommit(const QString& providerId, const QString& credentialRef,
                                      const QString& committedId) {
    // "Committed" means PROVEN - one real ProviderModels listing through the committed provider.
    // No catalog / no catalog-backed provider id (custom endpoint) stays permissive.
    if (m_providerCatalog == nullptr || providerId.isEmpty() ||
        m_providerCatalog->descriptorFor(providerId).isEmpty()) {
        finishCommit(committedId);
        return;
    }
    m_probeProviderId = providerId;
    m_probeCommittedId = committedId;
    if (m_probeTimeout == nullptr) {
        m_probeTimeout = new QTimer(this);
        m_probeTimeout->setSingleShot(true);
        m_probeTimeout->setInterval(15000);
        connect(m_probeTimeout, &QTimer::timeout, this, [this] {
            if (m_probeProviderId.isEmpty()) {
                return;
            }
            failCommit(tr("Couldn't reach your model — check the provider and try again."));
        });
    }
    QObject::disconnect(m_probeConnection);
    m_probeConnection = connect(
        m_providerCatalog, &models::IProviderCatalog::offeredModelsChanged, this,
        [this](const QString& pid) {
            if (pid != m_probeProviderId) {
                return; // another provider's listing, not the probe's
            }
            const QString committedId = m_probeCommittedId;
            // Count only REAL model rows: local providers append the synthetic "Discover"
            // affordance, which must never fake-pass the gate.
            int realModels = 0;
            const QVariantList rows = m_providerCatalog->offeredModels(pid);
            for (const QVariant& v : rows) {
                if (v.toMap().value(QStringLiteral("kind")).toString() !=
                    QStringLiteral("discover")) {
                    ++realModels;
                }
            }
            if (realModels > 0) {
                finishCommit(committedId);
            } else {
                failCommit(tr("Couldn't reach your model — check the provider and try again."));
            }
        });
    m_probeTimeout->start();
    // The listing authenticates with the committed profile's stored credential (empty for keyless).
    m_providerCatalog->refreshModels(providerId, credentialRef);
}

void AgentSetupModel::finishCommit(const QString& profileId) {
    cancelCommit();
    emit committed(profileId);
}

void AgentSetupModel::failCommit(const QString& message) {
    cancelCommit();
    emit failed(message);
}

void AgentSetupModel::cancelCommit() {
    m_applying = false;
    m_probeProviderId.clear();
    m_probeCommittedId.clear();
    QObject::disconnect(m_probeConnection);
    if (m_probeTimeout != nullptr) {
        m_probeTimeout->stop();
    }
}

void AgentSetupModel::activateLocalModel(const QString& model, const QString& profileId) {
    if (m_modelCatalog != nullptr && !model.isEmpty() && m_modelCatalog->isLocalInstalled(model)) {
        m_modelCatalog->activateForProfile(model, profileId);
    }
}

void AgentSetupModel::runOnNextProfilesChanged(const std::function<void()>& then) {
    connect(
        m_profiles, &profiles::IProfileStore::changed, this,
        [this, then] {
            if (m_applying) {
                then();
            } // else: the commit was cancelled first - abandon the continuation
        },
        static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
}

void AgentSetupModel::whenProfilesReflect(const std::function<bool()>& reflected,
                                          const std::function<void()>& then) {
    if (reflected()) {
        then(); // synchronous stores reflect mutations immediately
        return;
    }
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn =
        connect(m_profiles, &profiles::IProfileStore::changed, this, [this, conn, reflected, then] {
            if (!m_applying) {
                QObject::disconnect(*conn); // the commit was cancelled - abandon the continuation
                return;
            }
            if (!reflected()) {
                return; // not this reflection; keep waiting
            }
            QObject::disconnect(*conn);
            then();
        });
}

// --- editor / reset -------------------------------------------------------------------------

void AgentSetupModel::loadProfile(const QString& id) {
    if (m_profiles == nullptr || id.isEmpty()) {
        return;
    }
    const QVariantMap p = m_profiles->profile(id);
    if (p.isEmpty()) {
        return;
    }
    m_editing = true;
    // The presentation row key stays `acpAgent` (the foreign agent name) - see
    // daemon_profile_store.
    m_engineKind = p.value(QStringLiteral("engine"), QStringLiteral("Core")).toString();
    m_foreignAgent = p.value(QStringLiteral("acpAgent")).toString();
    m_engineLocked = true; // an existing profile's engine cannot change
    m_providerId = p.value(QStringLiteral("provider")).toString();
    m_model = p.value(QStringLiteral("model")).toString();
    m_baseUrl = p.value(QStringLiteral("baseUrl")).toString();
    m_credentialRef = p.value(QStringLiteral("credentialRef")).toString();
    m_backendMode = QStringLiteral("AgentNative");
    m_agentNativeModel.clear();
    const QVariantMap fb = p.value(QStringLiteral("foreignBackend")).toMap();
    if (!fb.isEmpty()) {
        const QString mode = fb.value(QStringLiteral("mode")).toString();
        if (mode == QStringLiteral("NodeProvider")) {
            m_backendMode = QStringLiteral("NodeProvider");
            m_providerId = fb.value(QStringLiteral("provider")).toString();
            m_model = fb.value(QStringLiteral("model")).toString();
            m_credentialRef = fb.value(QStringLiteral("credentialRef")).toString();
        } else {
            m_backendMode = QStringLiteral("AgentNative");
            m_agentNativeModel = fb.value(QStringLiteral("model")).toString();
        }
    }
    m_inferenceKey.clear();
    m_gateProviderId.clear();
    m_gateKey.clear();
    m_keyProven = false;
    m_keyGateMessage.clear();
    emit stateChanged();
    emit keyGateChanged();
}

void AgentSetupModel::reset() {
    cancelCommit();
    m_engineKind = QStringLiteral("Core");
    m_foreignAgent.clear();
    m_backendMode = QStringLiteral("AgentNative");
    m_providerId.clear();
    m_model.clear();
    m_credentialRef.clear();
    m_baseUrl.clear();
    m_agentNativeModel.clear();
    m_inferenceKey.clear();
    m_editing = false;
    m_engineLocked = false;
    m_gateProviderId.clear();
    m_gateKey.clear();
    m_keyProven = false;
    m_keyGateMessage.clear();
    emit stateChanged();
    emit keyGateChanged();
}

} // namespace setup
