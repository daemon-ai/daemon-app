// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/auth_flow_controller.h"

#include "auth/iauth_flow_service.h"
#include "auth/redirect_sink.h"

#include <QDateTime>
#include <QTimer>

namespace auth {

AuthFlowController::AuthFlowController(IAuthFlowService* service, QObject* parent)
    : QObject(parent), m_service(service), m_sink(new RedirectSink(this)), m_ttl(new QTimer(this)) {
    m_ttl->setSingleShot(true);
    connect(m_ttl, &QTimer::timeout, this, [this] {
        if (m_phase == QStringLiteral("awaiting_browser")) {
            teardownFlow(/*cancelRemote=*/true);
            setError(tr("This sign-in link expired — try again."));
            setPhase(QStringLiteral("failed"));
        }
    });
    connect(m_sink, &RedirectSink::captured, this, &AuthFlowController::submitCallback);
    if (m_service != nullptr) {
        connect(m_service, &IAuthFlowService::begun, this,
                [this](const QString& flowId, const QString& authorizationUrl,
                       const QString& /*redirectUri*/, quint64 expiresAt, const QString& flowKind) {
                    if (m_phase != QStringLiteral("beginning")) {
                        return; // cancelled/superseded while the begin was in flight
                    }
                    m_flowId = flowId;
                    m_authorizationUrl = authorizationUrl;
                    m_flowKind = flowKind;
                    emit flowChanged();
                    if (expiresAt > 0) {
                        const qint64 msLeft = (static_cast<qint64>(expiresAt) * 1000) -
                                              QDateTime::currentMSecsSinceEpoch();
                        if (msLeft > 0) {
                            m_ttl->start(static_cast<int>(qMin<qint64>(msLeft, INT_MAX)));
                        }
                    }
                    setPhase(QStringLiteral("awaiting_browser"));
                });
        connect(m_service, &IAuthFlowService::completed, this,
                [this](const QString& credentialRef, const QString& accountLabel,
                       const QString& transportInstance, const QString& boundProfile) {
                    if (m_phase != QStringLiteral("completing")) {
                        return;
                    }
                    m_credentialRef = credentialRef;
                    m_accountLabel = accountLabel;
                    teardownFlow(/*cancelRemote=*/false); // consumed node-side
                    emit completedChanged();
                    setPhase(QStringLiteral("success"));
                    emit succeeded(credentialRef, accountLabel, transportInstance, boundProfile);
                });
        connect(m_service, &IAuthFlowService::flowFailed, this,
                [this](const QString& phase, const QString& message) {
                    // Provider-list failures are not flow failures; the sheet's form shows them
                    // through providers() staying empty (and the begin button disabled).
                    if (phase == QStringLiteral("providers")) {
                        return;
                    }
                    if (m_phase != QStringLiteral("beginning") &&
                        m_phase != QStringLiteral("awaiting_browser") &&
                        m_phase != QStringLiteral("completing")) {
                        return;
                    }
                    teardownFlow(/*cancelRemote=*/phase == QStringLiteral("begin") ? false : true);
                    setError(message);
                    setPhase(QStringLiteral("failed"));
                });
    }
}

bool AuthFlowController::sinkListening() const {
    return m_sink != nullptr && m_sink->listening();
}

QVariantList AuthFlowController::providers() const {
    return m_service != nullptr ? m_service->providers() : QVariantList{};
}

void AuthFlowController::refreshProviders() {
    if (m_service != nullptr) {
        m_service->refreshProviders();
    }
}

void AuthFlowController::start(const QString& family, const QVariantMap& params,
                               const QString& bindProfile, bool useSink) {
    if (m_service == nullptr) {
        setError(tr("Sign-in is not available in this build."));
        setPhase(QStringLiteral("failed"));
        return;
    }
    if (m_phase == QStringLiteral("beginning") || m_phase == QStringLiteral("completing")) {
        return; // a round-trip is already in flight
    }
    teardownFlow(/*cancelRemote=*/true); // supersede any parked flow
    m_accountLabel.clear();
    m_credentialRef.clear();
    emit completedChanged();
    setError(QString());
    // The sink is best-effort: no loopback (wasm/sandbox/headless) => URL-only with manual
    // paste. The redirect URI must still be non-empty on the wire — the node mints the
    // authorization URL against it — so the pasteback path sends the conventional out-of-band
    // marker used by CLI-style flows.
    QString redirectUri = QStringLiteral("urn:ietf:wg:oauth:2.0:oob");
    if (useSink && m_sink->start()) {
        redirectUri = m_sink->redirectUri();
    }
    setPhase(QStringLiteral("beginning"));
    m_service->begin(family, params, redirectUri, bindProfile);
}

void AuthFlowController::submitCallback(const QString& callback) {
    const QString trimmed = callback.trimmed();
    if (m_phase != QStringLiteral("awaiting_browser") || trimmed.isEmpty() ||
        m_service == nullptr) {
        return;
    }
    m_ttl->stop();
    m_sink->stop();
    setPhase(QStringLiteral("completing"));
    m_service->complete(m_flowId, trimmed);
}

void AuthFlowController::cancel() {
    if (!active()) {
        return;
    }
    teardownFlow(/*cancelRemote=*/true);
    setError(QString());
    setPhase(QStringLiteral("cancelled"));
}

void AuthFlowController::reset() {
    teardownFlow(/*cancelRemote=*/true);
    m_accountLabel.clear();
    m_credentialRef.clear();
    emit completedChanged();
    setError(QString());
    setPhase(QStringLiteral("idle"));
}

void AuthFlowController::teardownFlow(bool cancelRemote) {
    m_ttl->stop();
    m_sink->stop();
    if (cancelRemote && m_service != nullptr && !m_flowId.isEmpty()) {
        m_service->cancel(m_flowId);
    }
    m_flowId.clear();
    m_authorizationUrl.clear();
    m_flowKind.clear();
    emit flowChanged();
}

void AuthFlowController::setPhase(const QString& phase) {
    if (m_phase == phase) {
        return;
    }
    m_phase = phase;
    emit phaseChanged();
}

void AuthFlowController::setError(const QString& error) {
    if (m_error == error) {
        return;
    }
    m_error = error;
    emit errorChanged();
}

} // namespace auth
