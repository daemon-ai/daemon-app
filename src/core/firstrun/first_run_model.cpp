#include "firstrun/first_run_model.h"

#include "connection/iconnection_service.h"
#include "settings/isettings_store.h"

namespace firstrun {

FirstRunModel::FirstRunModel(settings::ISettingsStore* settings,
                             connection::IConnectionService* connection, QObject* parent)
    : QObject(parent), m_settings(settings), m_connection(connection) {
    if (m_connection != nullptr) {
        connect(m_connection, &connection::IConnectionService::stateChanged, this,
                &FirstRunModel::onConnectionStateChanged);
    }
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
    } else if (s == QStringLiteral("ready")) {
        setError(QString());
        // CON-1: the connection succeeded, so persist setupComplete now - the next launch
        // auto-connects even if the user leaves before finishing the (placeholder) inference step.
        // finish() remains idempotent for the explicit "Finish"/"Skip" completion.
        if (m_settings != nullptr) {
            m_settings->setSetupComplete(true);
        }
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

void FirstRunModel::completeInference() {
    setInferenceReady(true);
    finish();
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
