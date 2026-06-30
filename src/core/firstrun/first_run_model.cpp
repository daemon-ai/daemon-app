// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "firstrun/first_run_model.h"

#include "connection/iconnection_service.h"
#include "models/imodel_catalog.h"
#include "settings/isettings_store.h"

namespace firstrun {

FirstRunModel::FirstRunModel(settings::ISettingsStore* settings,
                             connection::IConnectionService* connection,
                             models::IModelCatalog* modelCatalog, QObject* parent)
    : QObject(parent), m_settings(settings), m_connection(connection),
      m_modelCatalog(modelCatalog) {
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
    }
    refreshInferenceReady();
}

void FirstRunModel::begin() {
    if (m_settings != nullptr && m_settings->setupComplete()) {
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
        // CON-1: the connection succeeded, so persist setupComplete now - the next launch
        // auto-connects even if the user leaves before finishing the (placeholder) inference step.
        // finish() remains idempotent for the explicit "Finish"/"Skip" completion.
        if (m_settings != nullptr) {
            m_settings->setSetupComplete(true);
        }
        refreshInferenceReady();
        setPhase(QStringLiteral("inference"));
    } else if (s == QStringLiteral("offline")) {
        // Prefer a specific, actionable reason from the connection seam (e.g. the daemon binary
        // could not be found for managed local spawn) over the generic fallback.
        const QString detail = m_connection != nullptr ? m_connection->statusMessage() : QString();
        setError(detail.isEmpty() ? tr("Could not reach the node. Check the target and try again.")
                                  : detail);
        setPhase(QStringLiteral("connect"));
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
