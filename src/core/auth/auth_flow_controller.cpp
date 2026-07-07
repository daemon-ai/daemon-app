// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/auth_flow_controller.h"

#include "auth/iauth_flow_service.h"
#include "auth/redirect_sink.h"

#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QTimer>
#include <QUrl>

namespace auth {

namespace {
constexpr int kDefaultPollMs =
    2000; // fallback cadence for a Message challenge (Qr carries its own)
} // namespace

AuthFlowController::AuthFlowController(IAuthFlowService* service, QObject* parent)
    : QObject(parent), m_service(service), m_sink(new RedirectSink(this)), m_ttl(new QTimer(this)),
      m_poll(new QTimer(this)) {
    m_ttl->setSingleShot(true);
    connect(m_ttl, &QTimer::timeout, this, [this] {
        if (inFlight()) {
            teardownFlow(/*cancelRemote=*/true);
            setError(tr("This sign-in link expired — try again."));
            setPhase(QStringLiteral("failed"));
        }
    });
    connect(m_poll, &QTimer::timeout, this, [this] { poll(); });
    connect(m_sink, &RedirectSink::captured, this, &AuthFlowController::submitCallback);
    if (m_service != nullptr) {
        connect(m_service, &IAuthFlowService::begun, this,
                [this](const QString& flowId, const QVariantMap& challenge, quint64 expiresAt) {
                    if (m_phase != QStringLiteral("beginning")) {
                        return; // cancelled/superseded while the begin was in flight
                    }
                    m_flowId = flowId;
                    m_expiresAt = expiresAt;
                    if (expiresAt > 0) {
                        const qint64 msLeft = (static_cast<qint64>(expiresAt) * 1000) -
                                              QDateTime::currentMSecsSinceEpoch();
                        if (msLeft <= 0) {
                            // Already expired on arrival (clock skew / stale park): fail closed
                            // immediately rather than presenting a challenge that cannot complete.
                            teardownFlow(/*cancelRemote=*/true);
                            setError(tr("This sign-in link expired — try again."));
                            setPhase(QStringLiteral("failed"));
                            return;
                        }
                        m_ttl->start(static_cast<int>(qMin<qint64>(msLeft, INT_MAX)));
                    }
                    applyChallenge(challenge);
                });
        connect(m_service, &IAuthFlowService::challenged, this,
                [this](const QVariantMap& challenge) {
                    if (m_phase != QStringLiteral("challenge") &&
                        m_phase != QStringLiteral("stepping")) {
                        return; // cancelled/superseded
                    }
                    applyChallenge(challenge);
                });
        connect(m_service, &IAuthFlowService::completed, this,
                [this](const QString& credentialRef, const QString& accountLabel,
                       const QString& transportInstance, const QString& boundProfile) {
                    if (!inFlight()) {
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
                    if (!inFlight()) {
                        return;
                    }
                    teardownFlow(/*cancelRemote=*/phase != QStringLiteral("begin"));
                    setError(message);
                    setPhase(QStringLiteral("failed"));
                });
    }
}

bool AuthFlowController::sinkListening() const {
    return m_sink != nullptr && m_sink->listening();
}

bool AuthFlowController::inFlight() const {
    return m_phase == QStringLiteral("beginning") || m_phase == QStringLiteral("challenge") ||
           m_phase == QStringLiteral("stepping");
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
    if (m_phase == QStringLiteral("beginning") || m_phase == QStringLiteral("stepping")) {
        return; // a round-trip is already in flight
    }
    teardownFlow(/*cancelRemote=*/true); // supersede any parked flow
    m_accountLabel.clear();
    m_credentialRef.clear();
    emit completedChanged();
    setError(QString());
    // The sink is best-effort: no loopback (wasm/sandbox/headless) => URL-only with manual
    // paste for a Redirect challenge. The redirect URI must still be non-empty on the wire — the
    // node mints the authorization URL against it — so the pasteback path sends the conventional
    // out-of-band marker used by CLI-style flows.
    QString redirectUri = QStringLiteral("urn:ietf:wg:oauth:2.0:oob");
    if (useSink && m_sink->start()) {
        redirectUri = m_sink->redirectUri();
    }
    setPhase(QStringLiteral("beginning"));
    m_service->begin(family, params, redirectUri, bindProfile);
}

void AuthFlowController::openInBrowser() {
    if (m_phase == QStringLiteral("challenge") && m_challengeKind == QStringLiteral("redirect") &&
        !m_authorizationUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_authorizationUrl));
    }
}

void AuthFlowController::submitCallback(const QString& callback) {
    const QString trimmed = callback.trimmed();
    if (m_phase != QStringLiteral("challenge") || m_challengeKind != QStringLiteral("redirect") ||
        trimmed.isEmpty() || m_service == nullptr) {
        return;
    }
    m_poll->stop();
    m_sink->stop();
    setPhase(QStringLiteral("stepping"));
    StepInput input;
    input.kind = StepInputKind::Callback;
    input.callback = trimmed;
    m_service->step(m_flowId, input);
}

void AuthFlowController::submitFields(const QVariantMap& values) {
    if (m_phase != QStringLiteral("challenge") || m_challengeKind != QStringLiteral("form") ||
        m_service == nullptr) {
        return;
    }
    m_poll->stop();
    setPhase(QStringLiteral("stepping"));
    StepInput input;
    input.kind = StepInputKind::Fields;
    input.fields = values;
    m_service->step(m_flowId, input);
}

void AuthFlowController::poll() {
    if (m_phase != QStringLiteral("challenge") ||
        (m_challengeKind != QStringLiteral("qr") && m_challengeKind != QStringLiteral("message")) ||
        m_service == nullptr) {
        return;
    }
    m_poll->stop();
    setPhase(QStringLiteral("stepping"));
    StepInput input;
    input.kind = StepInputKind::Poll;
    m_service->step(m_flowId, input);
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

void AuthFlowController::applyChallenge(const QVariantMap& challenge) {
    m_poll->stop();
    m_challengeKind = challenge.value(QStringLiteral("kind")).toString();
    m_authorizationUrl = challenge.value(QStringLiteral("authorizationUrl")).toString();
    m_formTitle = challenge.value(QStringLiteral("title")).toString();
    m_formFields = challenge.value(QStringLiteral("fields")).toList();
    m_qrPayload = challenge.value(QStringLiteral("payload")).toString();
    const QByteArray image = challenge.value(QStringLiteral("image")).toByteArray();
    m_qrImageSource = image.isEmpty() ? QString()
                                      : QStringLiteral("data:image/png;base64,") +
                                            QString::fromLatin1(image.toBase64());
    m_pollIntervalMs = challenge.value(QStringLiteral("pollIntervalMs")).toInt();
    m_messageText = challenge.value(QStringLiteral("text")).toString();
    setPhase(QStringLiteral("challenge"));
    emit challengeChanged();
    // Qr/Message challenges advance by polling — arm the auto-poll on the advertised cadence (a
    // Message carries none, so fall back to a sane default). Redirect/Form await user input.
    if (m_challengeKind == QStringLiteral("qr") || m_challengeKind == QStringLiteral("message")) {
        const int interval = m_pollIntervalMs > 0 ? m_pollIntervalMs : kDefaultPollMs;
        m_poll->start(interval);
    }
}

void AuthFlowController::clearChallenge() {
    m_challengeKind.clear();
    m_authorizationUrl.clear();
    m_formTitle.clear();
    m_formFields.clear();
    m_qrPayload.clear();
    m_qrImageSource.clear();
    m_pollIntervalMs = 0;
    m_messageText.clear();
    m_expiresAt = 0;
}

void AuthFlowController::teardownFlow(bool cancelRemote) {
    m_ttl->stop();
    m_poll->stop();
    m_sink->stop();
    if (cancelRemote && m_service != nullptr && !m_flowId.isEmpty()) {
        m_service->cancel(m_flowId);
    }
    m_flowId.clear();
    clearChallenge();
    emit challengeChanged();
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
