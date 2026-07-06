// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "firstrun/first_run_model.h"

#include "accounts/iaccounts_service.h"
#include "connection/iconnection_service.h"
#include "models/imodel_catalog.h"
#include "models/iprovider_catalog.h"
#include "profiles/iprofile_store.h"
#include "settings/isettings_store.h"

#include <memory>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

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
    if (m_providerCatalog != nullptr) {
        // FIX 4: the key gate resolves off the provider catalog's (authenticated) model listings.
        connect(m_providerCatalog, &models::IProviderCatalog::offeredModelsChanged, this,
                &FirstRunModel::resolveKeyGate);
        // requiresKey is read live off the descriptor, so a provider-catalog change can flip the
        // gate without a listing; re-notify bindings.
        connect(m_providerCatalog, &models::IProviderCatalog::providersChanged, this,
                &FirstRunModel::keyGateChanged);
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
        if (inCommitPhase()) {
            return;
        }
        setPhase(m_agentTypeOffered ? QStringLiteral("agenttype") : QStringLiteral("inference"));
    }
}

void FirstRunModel::setAgentTypeOffered(bool offered) {
    m_agentTypeOffered = offered;
}

void FirstRunModel::chooseAgentType(const QString& acpAgent) {
    if (m_phase != QStringLiteral("agenttype")) {
        return;
    }
    if (acpAgent.isEmpty()) {
        // Native: continue into the existing inference step (CON-10..14) unchanged.
        setPhase(QStringLiteral("inference"));
    }
    // Foreign selections commit through applyAcpChoice (the front ends pass the chosen name).
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
    // No profile store wired (mock/standalone): just finish, preserving the permissive path.
    if (m_profiles == nullptr || providerId.isEmpty()) {
        finish();
        return;
    }
    if (m_applying) {
        return; // an apply continuation is already in flight (double-committed Finish)
    }
    m_applying = true;
    // The user has decided: close the auto-gate so a post-apply reflection that satisfies
    // evaluateWizardGate cannot race this flow into a second finish().
    m_gating = false;
    // Refetch-then-apply: request a fresh ProfileList reflection and apply on its arrival. The
    // pre-apply defaultProfileId() snapshot can be stale (the reflection race that used to
    // blind-create a "default" profile here); the node's CURRENT state is the only valid input.
    const QString trimmedName = name.trimmed();
    runOnNextProfilesChanged([this, providerId, model, key, trimmedName, customBaseUrl] {
        applyReflectedInferenceChoice(providerId, model, key, trimmedName, customBaseUrl);
    });
    m_profiles->refresh();
}

void FirstRunModel::applyReflectedInferenceChoice(const QString& providerId, const QString& model,
                                                  const QString& key, const QString& name,
                                                  const QString& customBaseUrl) {
    // Resolve the ProviderSelector + base URL the profile should persist from the descriptor (the
    // picker keys on the descriptor id; genai vendors share the "genai" selector). A1: a custom
    // endpoint carries no descriptor — the profile pins genai's OpenAI-compatible adapter
    // ("daemon_api") at the USER-supplied base URL instead.
    QString wireSelector = QStringLiteral("daemon_api");
    QString baseUrl = customBaseUrl.trimmed();
    if (baseUrl.isEmpty() && m_providerCatalog != nullptr) {
        const QVariantMap d = m_providerCatalog->descriptorFor(providerId);
        if (!d.isEmpty()) {
            wireSelector = d.value(QStringLiteral("wireSelector")).toString();
            baseUrl = d.value(QStringLiteral("defaultBaseUrl")).toString();
        }
    }
    // The node-seeded placeholder, read off the FRESH reflection that triggered this continuation.
    const QString profileId = m_profiles->defaultProfileId();
    QVariantMap fields;
    fields[QStringLiteral("provider")] = wireSelector;
    fields[QStringLiteral("model")] = model;
    fields[QStringLiteral("baseUrl")] = baseUrl;

    if (name.isEmpty() || name == profileId) {
        // Configure the node-seeded placeholder in place (also the TUI path, which offers no Name
        // field). It is already the active default — the wizard only runs when the ACTIVE profile
        // is unconfigured — so no ProfileSelect is needed. A node that reflects no profiles at
        // all hosts no profile management: nothing to configure (and nothing is blind-created).
        if (!profileId.isEmpty()) {
            m_profiles->updateProfile(profileId, fields);
            // Profile-scoped credential for a key-requiring provider (the node defaults the
            // profile's credential_ref to its id when unset, so the key lands where inference
            // looks it up).
            if (!key.isEmpty() && m_accounts != nullptr) {
                m_accounts->addApiKeyForProfile(profileId, providerId, QString(), key, QString());
            }
            activateLocalModel(model, profileId);
        }
        probeThenFinish(providerId, profileId);
        return;
    }

    // Named agent: ONE full-spec ProfileCreate under the chosen name. The follow-up select/delete
    // are sequenced on the node's reflections — the node dispatches pipelined Calls concurrently,
    // so firing ProfileSelect before the create is reflected could race it.
    const QString newId = m_profiles->createProfileWithSpec(name, fields);
    if (newId.isEmpty()) {
        finish(); // store without a live repo: nothing further to sequence
        return;
    }
    whenProfilesReflect(
        [this, newId] { return !m_profiles->profile(newId).isEmpty(); },
        [this, providerId, model, key, newId, profileId] {
            if (!key.isEmpty() && m_accounts != nullptr) {
                m_accounts->addApiKeyForProfile(newId, providerId, QString(), key, QString());
            }
            activateLocalModel(model, newId);
            m_profiles->setDefault(newId); // ProfileSelect: the session binds the default profile
            if (!profileId.isEmpty() && profileId != newId) {
                // Drop the seeded placeholder. No sessions exist pre-wizard, so no bound_profile
                // reference can break.
                m_profiles->remove(profileId);
            }
            // Finish only once the node reflects the NEW active default: the first chat issued on
            // finished() binds the node's active profile, so this ordering is what makes the new
            // agent (not the deleted placeholder) own the first session.
            whenProfilesReflect([this, newId] { return m_profiles->defaultProfileId() == newId; },
                                [this, providerId, newId] { probeThenFinish(providerId, newId); });
        });
}

void FirstRunModel::probeThenFinish(const QString& providerId, const QString& credentialRef) {
    // A7 (CON-7/CON-15): "done" means PROVEN — one real ProviderModels-class listing through the
    // committed provider before setSetupComplete. No catalog wired (mock/standalone) or no
    // catalog-backed provider id (the custom-endpoint path, which no wire op can list yet) stays
    // permissive: the send-time nudge (reenterProvider) remains the honest failure path there.
    if (m_providerCatalog == nullptr || providerId.isEmpty() ||
        m_providerCatalog->descriptorFor(providerId).isEmpty()) {
        setInferenceReady(true);
        finish();
        return;
    }
    m_probeProviderId = providerId;
    if (m_probeTimeout == nullptr) {
        m_probeTimeout = new QTimer(this);
        m_probeTimeout->setSingleShot(true);
        // Generous: a cold cloud listing can be slow; the mock/offscreen path resolves
        // synchronously and never arms the timer visibly.
        m_probeTimeout->setInterval(15000);
        connect(m_probeTimeout, &QTimer::timeout, this, [this] {
            if (m_probeProviderId.isEmpty()) {
                return;
            }
            QObject::disconnect(m_probeConnection);
            m_probeProviderId.clear();
            m_applying = false; // allow a re-committed Finish after the fix
            setError(tr("Couldn't reach your model — check the provider and try again."));
        });
    }
    QObject::disconnect(m_probeConnection);
    m_probeConnection = connect(
        m_providerCatalog, &models::IProviderCatalog::offeredModelsChanged, this,
        [this](const QString& pid) {
            if (pid != m_probeProviderId) {
                return; // another provider's listing, not the probe's
            }
            QObject::disconnect(m_probeConnection);
            m_probeTimeout->stop();
            m_probeProviderId.clear();
            // Count only REAL model rows: local providers always append the synthetic
            // "Discover More Models" affordance, which must never fake-pass the gate.
            int realModels = 0;
            const QVariantList rows = m_providerCatalog->offeredModels(pid);
            for (const QVariant& v : rows) {
                if (v.toMap().value(QStringLiteral("kind")).toString() !=
                    QStringLiteral("discover")) {
                    ++realModels;
                }
            }
            if (realModels > 0) {
                setError(QString());
                setInferenceReady(true);
                finish();
            } else {
                m_applying = false; // allow a re-committed Finish after the fix
                setError(tr("Couldn't reach your model — check the provider and try again."));
            }
        });
    m_probeTimeout->start();
    // The listing authenticates with the committed profile's stored credential (empty for
    // keyless providers; local engines resolve from the installed catalog).
    m_providerCatalog->refreshModels(providerId, credentialRef);
}

void FirstRunModel::applyAcpChoice(const QString& name, const QString& acpAgent) {
    // No profile store wired (mock/standalone) or nothing chosen: just finish (permissive path).
    if (m_profiles == nullptr || acpAgent.isEmpty()) {
        finish();
        return;
    }
    if (m_applying) {
        return; // an apply continuation is already in flight (double-committed Finish)
    }
    m_applying = true;
    m_gating = false;
    // Refetch-then-apply, exactly like the native path: the placeholder id must come off a
    // FRESH ProfileList reflection, never a stale snapshot.
    const QString trimmedName = name.trimmed();
    runOnNextProfilesChanged(
        [this, trimmedName, acpAgent] { applyReflectedAcpChoice(trimmedName, acpAgent); });
    m_profiles->refresh();
}

void FirstRunModel::applyReflectedAcpChoice(const QString& name, const QString& acpAgent) {
    // The node-seeded placeholder, read off the FRESH reflection that triggered this
    // continuation.
    const QString profileId = m_profiles->defaultProfileId();
    const QString agentName = name.isEmpty() ? acpAgent : name;
    // ONE named ProfileCreate carrying engine=Acp{agent} — a catalog NAME, never a recipe; no
    // provider/model/key/credential frames (the foreign agent brings its own model).
    // agentName is the display name (arg 'name'), acpAgent is the catalog agent id (arg
    // 'agent') — not swapped despite the token similarity the heuristic check flags.
    // NOLINTNEXTLINE(readability-suspicious-call-argument)
    const QString newId = m_profiles->createAcpProfile(agentName, acpAgent);
    if (newId.isEmpty()) {
        finish(); // store without a live repo: nothing further to sequence
        return;
    }
    whenProfilesReflect(
        [this, newId] { return !m_profiles->profile(newId).isEmpty(); },
        [this, newId, profileId] {
            m_profiles->setDefault(newId); // ProfileSelect: sessions bind the default profile
            if (!profileId.isEmpty() && profileId != newId) {
                // Drop the seeded placeholder (same rationale as the native named path: no
                // sessions exist pre-wizard, so no bound_profile reference can break).
                m_profiles->remove(profileId);
            }
            // Finish only once the node reflects the NEW active default, so the first chat's
            // SessionCreate binds the foreign agent. No inference probe applies: the agent
            // brings its own model (the node resolves the recipe by name at session open).
            whenProfilesReflect([this, newId] { return m_profiles->defaultProfileId() == newId; },
                                [this] {
                                    setInferenceReady(true);
                                    finish();
                                });
        });
}

void FirstRunModel::activateLocalModel(const QString& model, const QString& profileId) {
    // A LOCALLY INSTALLED model needs an explicit ModelActivate bound to the profile the wizard
    // just configured: the profile update alone records the model name, but the node's local
    // provider loads whatever the ACTIVE selection for that profile is (and ModelActivate without
    // a profile defaults to the node's launch profile name, not this agent).
    if (m_modelCatalog != nullptr && !model.isEmpty() && m_modelCatalog->isLocalInstalled(model)) {
        m_modelCatalog->activateForProfile(model, profileId);
    }
}

void FirstRunModel::runOnNextProfilesChanged(const std::function<void()>& then) {
    connect(
        m_profiles, &profiles::IProfileStore::changed, this,
        [this, then] {
            if (inCommitPhase()) {
                then();
            } // else: Skip/restart left the wizard first — abandon the continuation
        },
        static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
}

void FirstRunModel::whenProfilesReflect(const std::function<bool()>& reflected,
                                        const std::function<void()>& then) {
    if (reflected()) {
        then(); // synchronous stores reflect mutations immediately
        return;
    }
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn =
        connect(m_profiles, &profiles::IProfileStore::changed, this, [this, conn, reflected, then] {
            if (!inCommitPhase()) {
                QObject::disconnect(*conn); // Skip/restart: abandon the continuation
                return;
            }
            if (!reflected()) {
                return; // not this reflection; keep waiting
            }
            QObject::disconnect(*conn);
            then();
        });
}

bool FirstRunModel::keyGatePassed() const {
    // Mirrors the QML gate this hoists (`!providerRequiresKey || keyValidated`): pass unless the
    // reported provider requires a key that has not been proven since the last re-arm. No catalog
    // wired / nothing reported yet stays permissive, so the mock/standalone path is never trapped
    // (the front ends' completeness gates still require a provider + model + entered key).
    if (m_providerCatalog == nullptr || m_gateProviderId.isEmpty()) {
        return true;
    }
    const bool requiresKey = m_providerCatalog->descriptorFor(m_gateProviderId)
                                 .value(QStringLiteral("requiresKey"))
                                 .toBool();
    return !requiresKey || m_keyProven;
}

void FirstRunModel::setInferenceSelection(const QString& providerId, const QString& key) {
    // Unconditional re-arm - even a same-value re-report resets the proven bit, exactly like the
    // QML picker this hoists (every provider (re)selection and every keystroke reset
    // keyValidated); the next authenticated LIST re-proves it.
    m_gateProviderId = providerId;
    m_gateKey = key;
    m_keyProven = false;
    m_keyGateMessage.clear();
    emit keyGateChanged();
}

void FirstRunModel::resolveKeyGate(const QString& providerId) {
    if (m_providerCatalog == nullptr || providerId.isEmpty() || providerId != m_gateProviderId) {
        return; // not the reported selection's listing
    }
    const QVariantMap descriptor = m_providerCatalog->descriptorFor(providerId);
    if (!descriptor.value(QStringLiteral("requiresKey")).toBool() || m_gateKey.isEmpty()) {
        return; // keyless provider / no key entered yet: nothing to prove
    }
    // The LIST was authenticated with the entered key, so a non-empty result proves it (FIX 4).
    const int count = static_cast<int>(m_providerCatalog->offeredModels(providerId).size());
    m_keyProven = count > 0;
    const QString name = descriptor.value(QStringLiteral("name")).toString();
    const QString vendor = name.isEmpty() ? providerId : name;
    m_keyGateMessage =
        m_keyProven
            ? QString()
            : tr("Couldn't verify this API key with %1 — check it and try again.").arg(vendor);
    // The shared instrumentation seam, scoped to the wizard like the QML funnel it replaces (the
    // wizard's picker only existed during the inference phase).
    if (m_phase == QStringLiteral("inference")) {
        logKeyValidation(providerId, true, count, m_keyProven);
    }
    emit keyGateChanged();
}

void FirstRunModel::logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                     bool pass) const {
    // Instrumentation seam for the wizard's blocking key check (FIX 4): both front ends funnel
    // the authenticated-LIST outcome here so pass/block decisions share one trace point.
    qInfo("firstrun: key validation provider=%s requiresKey=%d models=%d gate=%s",
          qPrintable(provider), requiresKey ? 1 : 0, modelCount, pass ? "pass" : "block");
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
    m_applying = false; // any in-flight apply continuation is abandoned (phase leaves inference)
    // A dangling finish probe must not resolve into a stale finish() post-restart.
    m_probeProviderId.clear();
    QObject::disconnect(m_probeConnection);
    if (m_probeTimeout != nullptr) {
        m_probeTimeout->stop();
    }
    begin();
}

void FirstRunModel::reenterProvider() {
    // CON-8: deep-link back into the provider step from a live shell (the send-time "no provider
    // configured" nudge). setupComplete stays untouched — finishing (or Skip) re-persists it —
    // and the connection is already up, so the wizard re-opens directly at `inference`.
    if (m_phase != QStringLiteral("done")) {
        return; // already inside the wizard
    }
    m_applying = false;
    setError(QString());
    setPhase(QStringLiteral("inference"));
}

void FirstRunModel::finish() {
    // Committing to done closes the connect-ready node gate so a late reflection cannot re-fire it.
    m_gating = false;
    m_applying = false;
    // The probe (when one ran) resolved; drop any dangling listener/timer.
    m_probeProviderId.clear();
    QObject::disconnect(m_probeConnection);
    if (m_probeTimeout != nullptr) {
        m_probeTimeout->stop();
    }
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
