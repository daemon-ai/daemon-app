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
}

namespace firstrun {

// Shared first-run / onboarding gate. Drives the boot flow both front ends mount
// before the main shell:
//
//   connect  -> the connection picker (choose embedded[disabled]/local/remote)
//   connecting -> a full-screen splash while the chosen target comes up (first
//                 boot only); returns to `connect` on failure
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
    // permissive (the mock/standalone path).
    FirstRunModel(settings::ISettingsStore* settings, connection::IConnectionService* connection,
                  models::IModelCatalog* modelCatalog = nullptr, QObject* parent = nullptr);

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

    settings::ISettingsStore* m_settings = nullptr;
    connection::IConnectionService* m_connection = nullptr;
    models::IModelCatalog* m_modelCatalog = nullptr;
    QString m_phase = QStringLiteral("connect");
    QString m_error;
    bool m_inferenceReady = false;
};

} // namespace firstrun
