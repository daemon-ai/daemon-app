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

namespace firstrun {

// Shared first-run / onboarding gate. Drives the boot flow both front ends mount
// before the main shell:
//
//   connect  -> the connection picker (choose embedded[disabled]/local/remote)
//   connecting -> a full-screen splash while the chosen target comes up (first
//                 boot only); returns to `connect` on failure
//   auth     -> the login form when the node requires SASL credentials (between
//               connecting and inference); a wrong password keeps this phase + error
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

public:
    // `modelCatalog` (optional) drives inference readiness (CON-7): the inference gate's Finish is
    // enabled once a usable model is reachable (a current model resolves). Null keeps the gate
    // permissive (the mock/standalone path). `profiles`/`accounts`/`providers` (optional) let the
    // guided inference step persist a working profile (ProfileCreate/Update + profile-scoped key +
    // make default); null leaves applyInferenceChoice a no-op that just finishes.
    FirstRunModel(settings::ISettingsStore* settings, connection::IConnectionService* connection,
                  models::IModelCatalog* modelCatalog = nullptr,
                  profiles::IProfileStore* profileStore = nullptr,
                  accounts::IAccountsService* accounts = nullptr,
                  models::IProviderCatalog* providerCatalog = nullptr, QObject* parent = nullptr);

    [[nodiscard]] QString phase() const { return m_phase; }
    [[nodiscard]] bool active() const { return m_phase != QStringLiteral("done"); }
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] bool inferenceReady() const { return m_inferenceReady; }
    void setInferenceReady(bool ready);

    // Compute the initial phase from persisted setupComplete. Call once at boot.
    Q_INVOKABLE void begin();
    // The inference gate's "Continue" - records that an inference model is ready
    // and advances to done (persisting setupComplete).
    Q_INVOKABLE void completeInference();
    // Guided inference commit: persist a working profile for the chosen `providerId` + `model`
    // (ProfileCreate/Update with the descriptor's ProviderSelector + base URL), store the entered
    // `key` profile-scoped when non-empty, make the profile default, then finish. `providerId` is
    // the ProviderCatalog descriptor id. A no-op that just finishes when no profile store is wired.
    Q_INVOKABLE void applyInferenceChoice(const QString& providerId, const QString& model,
                                          const QString& key);
    // Submit interactive credentials while in the `auth` phase (routes to the connection seam's
    // login()). On success the connection reaches ready and the gate advances to inference.
    Q_INVOKABLE void submitLogin(const QString& username, const QString& password);
    // Record a key-validation attempt + its gate outcome for the wizard's blocking key check
    // (FIX 4). The gate itself is evaluated in QML (driven off the providerModelsRefreshed signal);
    // this is the shared instrumentation seam so both pass and block outcomes are captured.
    Q_INVOKABLE void logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                      bool pass) const;
    // Skip onboarding (dev / "I'll set this up later"): marks setup complete.
    Q_INVOKABLE void skip();
    // Re-run onboarding from settings (used by a "reset setup" affordance).
    Q_INVOKABLE void restart();

signals:
    void phaseChanged();
    void errorChanged();
    void inferenceReadyChanged();
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

    settings::ISettingsStore* m_settings = nullptr;
    connection::IConnectionService* m_connection = nullptr;
    models::IModelCatalog* m_modelCatalog = nullptr;
    profiles::IProfileStore* m_profiles = nullptr;
    accounts::IAccountsService* m_accounts = nullptr;
    models::IProviderCatalog* m_providerCatalog = nullptr;
    QString m_phase = QStringLiteral("connect");
    QString m_error;
    bool m_inferenceReady = false;
    // True while the connect-ready node gate is still deciding (waiting for the ProfileList /
    // ModelCurrent reflection). Cleared once we commit to `done` (configured) or the user finishes.
    bool m_gating = false;
};

} // namespace firstrun
