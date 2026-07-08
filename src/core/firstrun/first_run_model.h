// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

namespace settings {
class ISettingsStore;
}
namespace connection {
class IConnectionService;
}
namespace models {
class IModelCatalog;
class IProviderCatalog;
} // namespace models
namespace profiles {
class IProfileStore;
}
namespace accounts {
class IAccountsService;
}
namespace setup {
class AgentSetupModel;
}

namespace firstrun {

// Shared first-run / onboarding gate. Drives the boot flow both front ends mount
// before the main shell:
//
//   connect  -> the connection picker (choose embedded[disabled]/local/remote)
//   connecting -> a full-screen splash while the chosen target comes up (first
//                 boot only); returns to `connect` on failure
//   auth     -> the login form when the node requires SASL credentials (between
//               connecting and inference); a wrong password keeps this phase + error
//   agenttype -> the native-vs-foreign engine step (CON-16), gated ahead of the
//                inference step and shown only when the node offers foreign ACP
//                agents (setAgentTypeOffered); native proceeds to `inference`,
//                a foreign choice commits engine=Acp{agent} and skips
//                provider/model entirely (the agent brings its own model)
//   inference -> the inference/catalog gate (a default model must be reachable)
//   done     -> setup complete; the shell is shown and never gated again
//
// Persistence is via ISettingsStore.setupComplete, so a returning user boots
// straight to `done`. The catalog gate is mock now (a flag the Models hub will
// satisfy in Phase 3); the flow shape matches the q1 OnboardingController.
class FirstRunModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    // Whether a usable model is reachable; the inference gate blocks until true.
    Q_PROPERTY(bool inferenceReady READ inferenceReady WRITE setInferenceReady NOTIFY
                   inferenceReadyChanged)
    // The wizard's blocking key check (FIX 4), shared by BOTH front ends: passes unless the
    // reported provider selection requires an API key whose entered value has not (yet) been
    // PROVEN by a non-empty authenticated ProviderModels listing. keyGateMessage carries the
    // actionable blocking reason after a failed validation (empty otherwise).
    Q_PROPERTY(bool keyGatePassed READ keyGatePassed NOTIFY keyGateChanged)
    Q_PROPERTY(QString keyGateMessage READ keyGateMessage NOTIFY keyGateChanged)

public:
    // `modelCatalog` (optional) drives inference readiness (CON-7): the inference gate's Finish is
    // enabled once a usable model is reachable (a current model resolves). Null keeps the gate
    // permissive (the mock/standalone path). `profiles`/`accounts`/`providers` (optional) let the
    // guided inference step persist a working profile (ProfileCreate/Update + profile-scoped key +
    // make default); null leaves applyInferenceChoice a no-op that just finishes. `providerCatalog`
    // additionally drives the shared key-validation gate (FIX 4, keyGatePassed); null keeps that
    // gate permissive too.
    // `agentSetup` (optional) is the shared write-side setup view-model this gate delegates the
    // inference/key-gate/commit sequencing to. Null (the standalone/test path) makes FirstRunModel
    // construct+own its own AgentSetupModel from the same seams, so behavior is identical whether
    // or not the service graph injects a shared instance.
    FirstRunModel(settings::ISettingsStore* settings, connection::IConnectionService* connection,
                  models::IModelCatalog* modelCatalog = nullptr,
                  profiles::IProfileStore* profileStore = nullptr,
                  accounts::IAccountsService* accounts = nullptr,
                  models::IProviderCatalog* providerCatalog = nullptr,
                  setup::AgentSetupModel* agentSetup = nullptr, QObject* parent = nullptr);

    [[nodiscard]] QString phase() const { return m_phase; }
    [[nodiscard]] bool active() const { return m_phase != QStringLiteral("done"); }
    [[nodiscard]] QString error() const { return m_error; }
    // The shared setup view-model this gate delegates to (injected, or owned when standalone). The
    // TUI wizard's AgentSetupView binds it directly so the engine/backend/inference steps and the
    // wizard's commit/gate sequencing share one instance (the GUI reaches the same instance via the
    // `AgentSetup` context property).
    [[nodiscard]] setup::AgentSetupModel* agentSetup() const { return m_agentSetup; }
    [[nodiscard]] bool inferenceReady() const { return m_inferenceReady; }
    void setInferenceReady(bool ready);
    // The hoisted key gate now lives in AgentSetupModel; these delegate to it (the QML/TUI wizard
    // still binds FirstRun.keyGatePassed / .keyGateMessage).
    [[nodiscard]] bool keyGatePassed() const;
    [[nodiscard]] QString keyGateMessage() const;

    // Compute the initial phase from persisted setupComplete. Call once at boot.
    Q_INVOKABLE void begin();
    // Whether the wizard offers the agent-type step (CON-16): true when the node's ACP catalog /
    // discovery reflected at least one foreign agent. Set from OUTSIDE (the service graph wires
    // the ACP repository's catalogRefreshed to this; firstrun cannot link the daemon layer). The
    // step is decided when the connect-ready gate routes into the wizard; a catalog landing
    // AFTER that decision does not yank an in-progress step (foreign agents stay reachable via
    // the "+ New agent" dialog).
    Q_INVOKABLE void setAgentTypeOffered(bool offered);
    [[nodiscard]] bool agentTypeOffered() const { return m_agentTypeOffered; }
    // The agent-type step's commit (CON-16). Empty `acpAgent` = the native engine: advance to
    // the inference step unchanged. Non-empty = a foreign agent: applyForeignChoice(name, agent)
    // is the full commit (no provider/model/key applies).
    Q_INVOKABLE void chooseAgentType(const QString& foreignAgent);
    // Phase G: advance from the agent-type pane into the inference step when the shared model says
    // inference is needed (a Core engine, or a foreign agent on the NodeProvider backend). A
    // foreign AgentNative selection needs no inference and instead finishes directly via
    // commitSetup(). The engine/backend selection already lives in the shared AgentSetupModel
    // (the pickers drive it), so this only routes the phase.
    Q_INVOKABLE void continueFromAgentType();
    // Phase G: commit the CURRENT shared-model selection (engine + backend + inference, whatever
    // the pickers set) under `name`, then finish. Foreign-aware: it commits a Core, foreign
    // AgentNative, or foreign NodeProvider profile identically because the engine/backend/inference
    // dimensions are all held by AgentSetupModel. Async -> committed()/failed() route through
    // onSetupCommitted/onSetupFailed as with the other apply paths.
    Q_INVOKABLE void commitSetup(const QString& name);
    // Foreign-agent commit (CON-16): ONE named ProfileCreate carrying engine=Foreign{agent} (the
    // catalog NAME; recipes stay node-side), sequenced on fresh reflections exactly like the
    // native named path (create -> reflect -> setDefault -> drop the seeded placeholder ->
    // reflect default -> finish). No credential/model frames. Empty/blank `name` falls back to
    // the agent name.
    Q_INVOKABLE void applyForeignChoice(const QString& name, const QString& foreignAgent);
    // The inference gate's "Continue" - records that an inference model is ready
    // and advances to done (persisting setupComplete).
    Q_INVOKABLE void completeInference();
    // Guided inference commit: persist a working profile for the chosen `providerId` + `model`
    // (the descriptor's ProviderSelector + base URL), store the entered `key` profile-scoped when
    // non-empty, then finish. `providerId` is the ProviderCatalog descriptor id. `name` is the
    // agent's chosen NAME (the profile id the node keys it by): empty or equal to the seeded
    // placeholder id configures the placeholder in place (ProfileUpdate — the TUI path); anything
    // else mints a named agent (one full-spec ProfileCreate -> ProfileSelect -> ProfileDelete of
    // the placeholder). The apply is a refetch-then-apply continuation: it acts on a FRESH
    // ProfileList reflection, never a possibly-stale defaultProfileId() snapshot, and the named
    // path finishes only once the node reflects the new active default so the first chat's
    // SessionCreate binds the NEW agent. A no-op that just finishes when no profile store is
    // wired.
    //
    // A1 (CON-10 custom endpoint): a non-empty `customBaseUrl` marks a self-hosted
    // OpenAI-compatible endpoint — the profile persists provider="daemon_api" (genai's
    // OpenAI-compatible adapter pinned at the base URL; NEVER the model-name-inferred GenAi
    // wire, which would mis-infer e.g. Anthropic framing against a vLLM server) + the
    // user-supplied base URL + the user-typed model. No catalog descriptor applies, so the
    // key gate and the A7 probe stay permissive there (no wire op can list an arbitrary
    // endpoint's models); the send-time nudge (reenterProvider) is the honest failure path.
    Q_INVOKABLE void applyInferenceChoice(const QString& providerId, const QString& model,
                                          const QString& key, const QString& name = QString(),
                                          const QString& customBaseUrl = QString());
    // Submit interactive credentials while in the `auth` phase (routes to the connection seam's
    // login()). On success the connection reaches ready and the gate advances to inference.
    Q_INVOKABLE void submitLogin(const QString& username, const QString& password);
    // Report the wizard's LIVE provider/key selection for the key gate (FIX 4). Call on every
    // provider (re)selection and every key edit: each report unconditionally re-arms the gate (a
    // proven key must re-prove after any change), mirroring the QML picker semantics this hoists.
    // The next offeredModelsChanged for `providerId` with a non-empty `key` on a key-requiring
    // provider resolves it: a non-empty authenticated listing passes, an empty one blocks with an
    // actionable keyGateMessage. The key is held transiently only; never persisted or logged.
    Q_INVOKABLE void setInferenceSelection(const QString& providerId, const QString& key);
    // Record a key-validation attempt + its gate outcome for the wizard's blocking key check
    // (FIX 4). The gate itself is evaluated by this model (resolveKeyGate, driven off the provider
    // catalog's offeredModelsChanged signal); this is the shared instrumentation seam so both pass
    // and block outcomes are captured.
    Q_INVOKABLE void logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                      bool pass) const;
    // Skip onboarding (dev / "I'll set this up later"): marks setup complete.
    Q_INVOKABLE void skip();
    // Re-run onboarding from settings (used by a "reset setup" affordance).
    Q_INVOKABLE void restart();
    // CON-8 (A7): re-enter the wizard AT the provider/inference step from a live shell — the
    // send-time "no provider configured" nudge's deep link. Unlike restart() it neither clears
    // setupComplete nor re-runs the connect step; finishing (or skipping) re-persists
    // setupComplete and returns to the shell.
    Q_INVOKABLE void reenterProvider();

signals:
    void phaseChanged();
    void errorChanged();
    void inferenceReadyChanged();
    // The key gate re-armed or resolved (keyGatePassed / keyGateMessage may have changed).
    void keyGateChanged();
    // Fired once when onboarding finishes, so the shell can mount.
    void finished();

private:
    void setPhase(const QString& phase);
    void setError(const QString& error);
    void onConnectionStateChanged();
    // Recompute inferenceReady from the model catalog (a current model is reachable). When no
    // catalog is wired the gate stays permissive (ready) so the mock/standalone path is unblocked.
    void refreshInferenceReady();
    void finish();
    // Node-authoritative wizard gate (A2): on connect-ready decide whether to run the wizard by the
    // NODE, not the client `setupComplete` hint — run it iff the node's active profile is NOT
    // configured (empty model). Re-evaluated as the reflected ProfileList / ModelCurrent land.
    void evaluateWizardGate();
    // Whether the node's active default profile already has a model configured (reflected from the
    // profile store). False when nothing is reflected yet or no active default exists.
    [[nodiscard]] bool activeModelConfigured() const;
    // AgentSetupModel commit outcomes (the moved inference/foreign apply + finish-probe resolve
    // here): a success sets inference readiness and finishes; a failure keeps the wizard open with
    // the actionable reason.
    void onSetupCommitted(const QString& profileId);
    void onSetupFailed(const QString& message);

    settings::ISettingsStore* m_settings = nullptr;
    connection::IConnectionService* m_connection = nullptr;
    models::IModelCatalog* m_modelCatalog = nullptr;
    profiles::IProfileStore* m_profiles = nullptr;
    accounts::IAccountsService* m_accounts = nullptr;
    models::IProviderCatalog* m_providerCatalog = nullptr;
    // The shared setup view-model this gate delegates inference/key-gate/commit to (injected by the
    // service graph, or owned when constructed standalone).
    setup::AgentSetupModel* m_agentSetup = nullptr;
    QString m_phase = QStringLiteral("connect");
    QString m_error;
    bool m_inferenceReady = false;
    // True while the connect-ready node gate is still deciding (waiting for the ProfileList /
    // ModelCurrent reflection). Cleared once we commit to `done` (configured) or the user finishes.
    bool m_gating = false;
    // CON-16: whether the node offered foreign ACP agents (drives the agenttype step).
    bool m_agentTypeOffered = false;
};

} // namespace firstrun
