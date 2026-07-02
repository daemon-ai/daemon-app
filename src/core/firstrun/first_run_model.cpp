// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "firstrun/first_run_model.h"

#include "accounts/iaccounts_service.h"
#include "connection/iconnection_service.h"
#include "models/imodel_catalog.h"
#include "models/iprovider_catalog.h"
#include "profiles/iprofile_store.h"
#include "settings/isettings_store.h"

#include <QDateTime>
#include <QFile>

namespace firstrun {

FirstRunModel::FirstRunModel(settings::ISettingsStore* settings,
                             connection::IConnectionService* connection,
                             models::IModelCatalog* modelCatalog,
                             profiles::IProfileStore* profileStore,
                             accounts::IAccountsService* accounts,
                             models::IProviderCatalog* providerCatalog, QObject* parent)
    : QObject(parent), m_settings(settings), m_connection(connection), m_modelCatalog(modelCatalog),
      m_profiles(profileStore), m_accounts(accounts), m_providerCatalog(providerCatalog) {
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
    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"H-B\","
                                     "\"location\":\"first_run_model.cpp:begin\",\"message\":"
                                     "\"begin() gate\",\"data\":{\"setupComplete\":%1},"
                                     "\"timestamp\":%2}\n")
                          .arg(complete ? "true" : "false")
                          .arg(QDateTime::currentMSecsSinceEpoch())
                          .toUtf8());
    }
    // #endregion
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
        // #region agent log
        {
            QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
            if (dbg.open(QIODevice::Append | QIODevice::Text))
                dbg.write(
                    QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"H-A\","
                                   "\"location\":\"first_run_model.cpp:onConnectionStateChanged\","
                                   "\"message\":\"connection ready -> inference (no premature "
                                   "setupComplete)\",\"data\":{},\"timestamp\":%1}\n")
                        .arg(QDateTime::currentMSecsSinceEpoch())
                        .toUtf8());
        }
        // #endregion
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
    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"WIZARD-GATE\","
                                     "\"location\":\"first_run_model.cpp:evaluateWizardGate\","
                                     "\"message\":\"node-authoritative wizard gate\",\"data\":{"
                                     "\"activeModelEmpty\":%1,\"decision\":\"%2\"},"
                                     "\"timestamp\":%3}\n")
                          .arg(configured ? "false" : "true")
                          .arg(configured ? "skip-wizard" : "run-wizard")
                          .arg(QDateTime::currentMSecsSinceEpoch())
                          .toUtf8());
    }
    // #endregion
    if (configured) {
        // The node already has a configured active agent: skip the inference wizard entirely.
        m_gating = false;
        finish();
    } else {
        // Not configured (or not yet reflected): show the inference step. Stay gating so a late
        // reflection that reveals the node IS configured can still auto-skip.
        setPhase(QStringLiteral("inference"));
    }
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
                                         const QString& key) {
    // No profile store wired (mock/standalone): just finish, preserving the permissive path.
    if (m_profiles == nullptr || providerId.isEmpty()) {
        finish();
        return;
    }
    // Resolve the ProviderSelector + base URL the profile should persist from the descriptor (the
    // picker keys on the descriptor id; genai vendors share the "genai" selector).
    QString wireSelector = QStringLiteral("daemon_api");
    QString baseUrl;
    if (m_providerCatalog != nullptr) {
        const QVariantMap d = m_providerCatalog->descriptorFor(providerId);
        if (!d.isEmpty()) {
            wireSelector = d.value(QStringLiteral("wireSelector")).toString();
            baseUrl = d.value(QStringLiteral("defaultBaseUrl")).toString();
        }
    }
    // Configure the existing default profile when the node already has one (Track 1 boots an
    // unconfigured default); otherwise create a fresh one.
    QString profileId = m_profiles->defaultProfileId();
    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(
                QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"APPLY-INFER\","
                               "\"location\":\"first_run_model.cpp:applyInferenceChoice\","
                               "\"message\":\"wizard persist inputs\",\"data\":{"
                               "\"providerId\":\"%1\",\"model\":\"%2\",\"wireSelector\":\"%3\","
                               "\"defaultBefore\":\"%4\"},\"timestamp\":%5}\n")
                    .arg(providerId, model, wireSelector, profileId)
                    .arg(QDateTime::currentMSecsSinceEpoch())
                    .toUtf8());
    }
    // #endregion
    if (profileId.isEmpty()) {
        profileId = m_profiles->createProfile(QStringLiteral("default"));
    }
    QVariantMap fields;
    fields[QStringLiteral("provider")] = wireSelector;
    fields[QStringLiteral("model")] = model;
    fields[QStringLiteral("baseUrl")] = baseUrl;
    m_profiles->updateProfile(profileId, fields);
    // Profile-scoped credential for a key-requiring provider (the node defaults the profile's
    // credential_ref to its id when unset, so the key lands where inference looks it up).
    if (!key.isEmpty() && m_accounts != nullptr) {
        m_accounts->addApiKeyForProfile(profileId, providerId, QString(), key, QString());
    }
    m_profiles->setDefault(profileId); // the session binds the default profile
    setInferenceReady(true);
    finish();
}

void FirstRunModel::logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                     bool pass) const {
    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"KEY-VALIDATE\","
                                     "\"location\":\"first_run_model.cpp:logKeyValidation\","
                                     "\"message\":\"authenticated ProviderModels gate\",\"data\":{"
                                     "\"provider\":\"%1\",\"requiresKey\":%2,\"modelCount\":%3,"
                                     "\"gate\":\"%4\"},\"timestamp\":%5}\n")
                          .arg(provider)
                          .arg(requiresKey ? "true" : "false")
                          .arg(modelCount)
                          .arg(pass ? "pass" : "block")
                          .arg(QDateTime::currentMSecsSinceEpoch())
                          .toUtf8());
    }
    // #endregion
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
    begin();
}

void FirstRunModel::finish() {
    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"H-A\","
                                     "\"location\":\"first_run_model.cpp:finish\",\"message\":"
                                     "\"finish() -> setupComplete=true\",\"data\":{},"
                                     "\"timestamp\":%1}\n")
                          .arg(QDateTime::currentMSecsSinceEpoch())
                          .toUtf8());
    }
    // #endregion
    // Committing to done closes the connect-ready node gate so a late reflection cannot re-fire it.
    m_gating = false;
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
