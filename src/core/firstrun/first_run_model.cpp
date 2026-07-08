// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "firstrun/first_run_model.h"

#include "accounts/iaccounts_service.h"
#include "connection/iconnection_service.h"
#include "models/imodel_catalog.h"
#include "models/iprovider_catalog.h"
#include "profiles/iprofile_store.h"
#include "settings/isettings_store.h"
#include "setup/agent_setup_model.h"

#include <QVariantMap>

namespace firstrun {

FirstRunModel::FirstRunModel(settings::ISettingsStore* settings,
                             connection::IConnectionService* connection,
                             models::IModelCatalog* modelCatalog,
                             profiles::IProfileStore* profileStore,
                             accounts::IAccountsService* accounts,
                             models::IProviderCatalog* providerCatalog,
                             setup::AgentSetupModel* agentSetup, QObject* parent)
    : QObject(parent), m_settings(settings), m_connection(connection), m_modelCatalog(modelCatalog),
      m_profiles(profileStore), m_accounts(accounts), m_providerCatalog(providerCatalog),
      m_agentSetup(agentSetup) {
    // The gate owns a setup view-model to delegate inference/key-gate/commit to. When the graph
    // does not inject a shared one (standalone / tests), construct+own an equivalent from the same
    // seams.
    if (m_agentSetup == nullptr) {
        m_agentSetup = new setup::AgentSetupModel(m_profiles, m_accounts, m_providerCatalog,
                                                  m_modelCatalog, this);
    }
    // A commit resolves (inference/foreign apply + finish-probe) -> finish; a failure keeps the
    // wizard open. The key gate re-arms/resolves inside the setup model; re-emit for the QML/TUI
    // bindings that watch FirstRun.keyGate*.
    connect(m_agentSetup, &setup::AgentSetupModel::committed, this,
            &FirstRunModel::onSetupCommitted);
    connect(m_agentSetup, &setup::AgentSetupModel::failed, this, &FirstRunModel::onSetupFailed);
    connect(m_agentSetup, &setup::AgentSetupModel::keyGateChanged, this,
            &FirstRunModel::keyGateChanged);
    if (m_connection != nullptr) {
        connect(m_connection, &connection::IConnectionService::stateChanged, this,
                &FirstRunModel::onConnectionStateChanged);
        // The node requires interactive credentials: surface the login form (no error yet).
        connect(m_connection, &connection::IConnectionService::authRequired, this, [this] {
            if (m_phase != QStringLiteral("done") && m_phase != QStringLiteral("inference")) {
                setError(QString());
                setPhase(QStringLiteral("auth"));
            }
        });
        // A login attempt failed (wrong password / disabled): stay on the form with the reason.
        connect(m_connection, &connection::IConnectionService::authFailed, this,
                [this](const QString& reason) {
                    if (m_phase != QStringLiteral("done") &&
                        m_phase != QStringLiteral("inference")) {
                        setError(reason);
                        setPhase(QStringLiteral("auth"));
                    }
                });
    }
    if (m_modelCatalog != nullptr) {
        connect(m_modelCatalog, &models::IModelCatalog::currentChanged, this,
                &FirstRunModel::refreshInferenceReady);
        // A2: a ModelCurrent reflection may resolve the node gate (node already configured ->
        // skip).
        connect(m_modelCatalog, &models::IModelCatalog::currentChanged, this,
                &FirstRunModel::evaluateWizardGate);
    }
    // A2: the reflected ProfileList (the node's active default + its model) is the authoritative
    // gate input; re-evaluate whenever it changes while we are still gating.
    if (m_profiles != nullptr) {
        connect(m_profiles, &profiles::IProfileStore::changed, this,
                &FirstRunModel::evaluateWizardGate);
    }
    refreshInferenceReady();
}

void FirstRunModel::begin() {
    const bool complete = (m_settings != nullptr && m_settings->setupComplete());
    if (complete) {
        setPhase(QStringLiteral("done"));
        return;
    }
    setError(QString());
    setPhase(QStringLiteral("connect"));
}

void FirstRunModel::onConnectionStateChanged() {
    // The connection seam only matters while we are gating on it (connect /
    // connecting). Once setup is done, footer state changes are cosmetic.
    if (m_phase == QStringLiteral("done") || m_phase == QStringLiteral("inference")) {
        return;
    }
    const QString s = m_connection != nullptr ? m_connection->state() : QString();
    if (s == QStringLiteral("checking") || s == QStringLiteral("connecting")) {
        setError(QString());
        setPhase(QStringLiteral("connecting"));
    } else if (s == QStringLiteral("authenticating")) {
        // The node needs credentials: show the login form. Don't clear m_error here - a wrong
        // password set it via authFailed and it must stay visible.
        setPhase(QStringLiteral("auth"));
    } else if (s == QStringLiteral("ready")) {
        setError(QString());
        // A live connection alone is NOT "setup complete": the user still has to pick a
        // provider/model in the inference step. Persisting setupComplete here would let a quit
        // mid-wizard skip the wizard forever on the next launch. Only finish() (a finished or
        // explicitly skipped inference step) marks setup complete.
        refreshInferenceReady();
        // A2: gate on the NODE, not solely the client setupComplete hint. Run the wizard iff the
        // node's active profile is not configured (empty model); if it is already configured, skip
        // straight to done. `m_gating` keeps this reactive to the ProfileList/ModelCurrent
        // reflection.
        m_gating = true;
        evaluateWizardGate();
    } else if (s == QStringLiteral("offline")) {
        // Prefer a specific, actionable reason from the connection seam (e.g. the daemon binary
        // could not be found for managed local spawn) over the generic fallback.
        const QString detail = m_connection != nullptr ? m_connection->statusMessage() : QString();
        setError(detail.isEmpty() ? tr("Could not reach the node. Check the target and try again.")
                                  : detail);
        setPhase(QStringLiteral("connect"));
    }
}

bool FirstRunModel::activeModelConfigured() const {
    if (m_profiles == nullptr) {
        return false;
    }
    const QString id = m_profiles->defaultProfileId();
    if (id.isEmpty()) {
        return false;
    }
    const QVariantMap p = m_profiles->profile(id);
    return !p.value(QStringLiteral("model")).toString().isEmpty();
}

void FirstRunModel::evaluateWizardGate() {
    // Only decides while the connect-ready gate is open (not once we are done or the user is past
    // it). Called on ready and whenever the reflected ProfileList / ModelCurrent changes.
    if (!m_gating) {
        return;
    }
    const bool configured = activeModelConfigured();
    if (configured) {
        // The node already has a configured active agent: skip the inference wizard entirely.
        m_gating = false;
        finish();
    } else {
        // Not configured (or not yet reflected): show the wizard. The agent-type step (CON-16)
        // fronts the inference step only when the node offered foreign ACP agents at decision
        // time — an empty catalog keeps zero foreign noise. Re-evaluations (later reflections)
        // must not stomp a step the user is already inside; stay gating so a late reflection
        // that reveals the node IS configured can still auto-skip.
        if (m_phase == QStringLiteral("inference") || m_phase == QStringLiteral("agenttype")) {
            return; // the user is already inside a committable step
        }
        setPhase(m_agentTypeOffered ? QStringLiteral("agenttype") : QStringLiteral("inference"));
    }
}

void FirstRunModel::setAgentTypeOffered(bool offered) {
    m_agentTypeOffered = offered;
}

void FirstRunModel::chooseAgentType(const QString& foreignAgent) {
    if (m_phase != QStringLiteral("agenttype")) {
        return;
    }
    if (foreignAgent.isEmpty()) {
        // Native: continue into the existing inference step (CON-10..14) unchanged.
        setPhase(QStringLiteral("inference"));
    }
    // Foreign selections commit through applyForeignChoice (the front ends pass the chosen name).
}

void FirstRunModel::setInferenceReady(bool ready) {
    if (m_inferenceReady == ready) {
        return;
    }
    m_inferenceReady = ready;
    emit inferenceReadyChanged();
}

void FirstRunModel::refreshInferenceReady() {
    // Ready when a usable model is reachable. With no catalog wired (mock/standalone), stay
    // permissive so the gate never traps that path.
    const bool ready = m_modelCatalog == nullptr || !m_modelCatalog->currentModelId().isEmpty();
    setInferenceReady(ready);
}

void FirstRunModel::completeInference() {
    // CON-7: only finish once a usable model is reachable; otherwise the user must pick one (or
    // Skip, which the front ends still offer and which leads to the first-send nudge, CON-8).
    if (!m_inferenceReady) {
        return;
    }
    finish();
}

void FirstRunModel::applyInferenceChoice(const QString& providerId, const QString& model,
                                         const QString& key, const QString& name,
                                         const QString& customBaseUrl) {
    // The user has decided: close the auto-gate so a post-apply reflection that satisfies
    // evaluateWizardGate cannot race this flow into a second finish(). The setup model owns the
    // create/apply/probe sequencing and answers with committed()/failed().
    m_gating = false;
    m_agentSetup->chooseEngine(QStringLiteral("Core"));
    m_agentSetup->setInference(providerId, model, key, customBaseUrl);
    m_agentSetup->commit(name);
}

void FirstRunModel::applyForeignChoice(const QString& name, const QString& foreignAgent) {
    // Foreign commit (CON-16): engine=Foreign{agent}, AgentNative backend (the agent brings its own
    // model). The setup model sequences the named ProfileCreate + set-default + placeholder drop.
    m_gating = false;
    m_agentSetup->chooseEngine(QStringLiteral("Foreign"), foreignAgent);
    m_agentSetup->chooseBackend(QStringLiteral("AgentNative"));
    m_agentSetup->commit(name);
}

void FirstRunModel::onSetupCommitted(const QString& profileId) {
    Q_UNUSED(profileId)
    // A commit proved out (or was the permissive no-store path): the inference gate is satisfied
    // and onboarding finishes (finished() opens the first chat, binding the node's new active
    // profile).
    setError(QString());
    setInferenceReady(true);
    finish();
}

void FirstRunModel::onSetupFailed(const QString& message) {
    // The provider probe failed: keep the wizard open at the current step with the actionable
    // reason (the setup model already re-armed so a corrected re-commit can proceed).
    setError(message);
}

bool FirstRunModel::keyGatePassed() const {
    return m_agentSetup->keyGatePassed();
}

QString FirstRunModel::keyGateMessage() const {
    return m_agentSetup->keyGateMessage();
}

void FirstRunModel::setInferenceSelection(const QString& providerId, const QString& key) {
    m_agentSetup->setInferenceSelection(providerId, key);
}

void FirstRunModel::logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                     bool pass) const {
    m_agentSetup->logKeyValidation(provider, requiresKey, modelCount, pass);
}

void FirstRunModel::submitLogin(const QString& username, const QString& password) {
    if (m_connection != nullptr) {
        setError(QString());
        m_connection->login(username, password);
    }
}

void FirstRunModel::skip() {
    finish();
}

void FirstRunModel::restart() {
    if (m_settings != nullptr) {
        m_settings->setSetupComplete(false);
    }
    setInferenceReady(false);
    // Abandon any in-flight commit continuation/probe + clear the stale selection for a fresh run
    // (the wizard leaves the inference/agenttype phase, so a stale apply must not fire post-done).
    m_agentSetup->reset();
    begin();
}

void FirstRunModel::reenterProvider() {
    // CON-8: deep-link back into the provider step from a live shell (the send-time "no provider
    // configured" nudge). setupComplete stays untouched — finishing (or Skip) re-persists it —
    // and the connection is already up, so the wizard re-opens directly at `inference`.
    if (m_phase != QStringLiteral("done")) {
        return; // already inside the wizard
    }
    m_agentSetup->cancelCommit(); // abandon any dangling continuation before re-opening the step
    setError(QString());
    setPhase(QStringLiteral("inference"));
}

void FirstRunModel::finish() {
    // Committing to done closes the connect-ready node gate so a late reflection cannot re-fire it.
    m_gating = false;
    // Abandon any in-flight commit continuation/probe in the setup model (a user Skip may reach
    // here while a stale continuation is pending).
    m_agentSetup->cancelCommit();
    // Emit-once: an in-flight apply continuation and a user Skip may both reach here; only the
    // first completion counts (a second finished() would open a second first-chat).
    if (m_phase == QStringLiteral("done")) {
        return;
    }
    if (m_settings != nullptr) {
        m_settings->setSetupComplete(true);
    }
    setPhase(QStringLiteral("done"));
    emit finished();
}

void FirstRunModel::setPhase(const QString& phase) {
    if (m_phase == phase) {
        return;
    }
    m_phase = phase;
    emit phaseChanged();
}

void FirstRunModel::setError(const QString& error) {
    if (m_error == error) {
        return;
    }
    m_error = error;
    emit errorChanged();
}

} // namespace firstrun
