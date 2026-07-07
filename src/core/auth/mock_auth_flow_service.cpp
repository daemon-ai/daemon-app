// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/mock_auth_flow_service.h"

#include <QTimer>

namespace auth {

namespace {
// Build a Redirect challenge map (the SSO analogue) for `host` / `family`.
QVariantMap redirectChallenge(const QString& host, const QString& family) {
    QVariantMap challenge;
    challenge[QStringLiteral("kind")] = QStringLiteral("redirect");
    challenge[QStringLiteral("authorizationUrl")] =
        QStringLiteral("https://%1/_mock/sso?family=%2")
            .arg(host.isEmpty() ? QStringLiteral("auth.example.org") : host, family);
    return challenge;
}

// Build a Form challenge asking for a single `token` field.
QVariantMap formChallenge() {
    QVariantMap field;
    field[QStringLiteral("key")] = QStringLiteral("token");
    field[QStringLiteral("label")] = QStringLiteral("Access token");
    field[QStringLiteral("required")] = true;
    QVariantMap challenge;
    challenge[QStringLiteral("kind")] = QStringLiteral("form");
    challenge[QStringLiteral("title")] = QStringLiteral("Enter your access token");
    challenge[QStringLiteral("fields")] = QVariantList{field};
    return challenge;
}

// Build a Message challenge the client follows with a Poll.
QVariantMap messageChallenge() {
    QVariantMap challenge;
    challenge[QStringLiteral("kind")] = QStringLiteral("message");
    challenge[QStringLiteral("text")] = QStringLiteral("Approve the sign-in on your other device…");
    return challenge;
}
} // namespace

MockAuthFlowService::MockAuthFlowService(QObject* parent) : IAuthFlowService(parent) {
    // One SSO-shaped Redirect family (the Matrix analogue) so the begin form has a params schema to
    // render, mirroring what a live AuthProviders returns.
    QVariantMap homeserver;
    homeserver[QStringLiteral("key")] = QStringLiteral("homeserver");
    homeserver[QStringLiteral("label")] = QStringLiteral("Homeserver");
    homeserver[QStringLiteral("required")] = true;
    QVariantMap matrix;
    matrix[QStringLiteral("family")] = QStringLiteral("matrix");
    matrix[QStringLiteral("flowKind")] = QStringLiteral("MatrixSso");
    matrix[QStringLiteral("name")] = QStringLiteral("Matrix (SSO)");
    matrix[QStringLiteral("params")] = QVariantList{homeserver};
    m_providers.append(matrix);

    // One Form/multi-step family (the pasted-token analogue) so the non-redirect challenge kinds
    // are exercised too — its params are collected in-flow (Form challenge), not at begin.
    QVariantMap token;
    token[QStringLiteral("family")] = QStringLiteral("token");
    token[QStringLiteral("flowKind")] = QStringLiteral("BotToken");
    token[QStringLiteral("name")] = QStringLiteral("Access token");
    token[QStringLiteral("params")] = QVariantList{};
    m_providers.append(token);
}

void MockAuthFlowService::refreshProviders() {
    QTimer::singleShot(0, this, [this] { emit providersChanged(); });
}

void MockAuthFlowService::begin(const QString& family, const QVariantMap& params,
                                const QString& redirectUri, const QString& bindProfile) {
    Q_UNUSED(redirectUri)
    const QString host = params.value(QStringLiteral("homeserver")).toString();
    const quint64 flowSerial = ++m_flowSerial;
    const quint64 expiresAt = m_nextExpiresAt;
    m_nextExpiresAt = 0;
    QTimer::singleShot(0, this, [this, family, host, bindProfile, flowSerial, expiresAt] {
        if (!m_failBegin.isEmpty()) {
            const QString message = m_failBegin;
            m_failBegin.clear();
            emit flowFailed(QStringLiteral("begin"), message);
            return;
        }
        const QString flowId = QStringLiteral("mock-flow-%1").arg(flowSerial);
        m_flows.insert(flowId, FlowState{family, bindProfile, 0});
        const QVariantMap challenge =
            family == QStringLiteral("matrix") ? redirectChallenge(host, family) : formChallenge();
        emit begun(flowId, challenge, expiresAt);
    });
}

void MockAuthFlowService::step(const QString& flowId, const StepInput& input) {
    Q_UNUSED(input)
    QTimer::singleShot(0, this, [this, flowId] {
        if (!m_failStep.isEmpty()) {
            const QString message = m_failStep;
            m_failStep.clear();
            emit flowFailed(QStringLiteral("step"), message);
            return;
        }
        auto it = m_flows.find(flowId);
        if (it == m_flows.end()) {
            emit flowFailed(QStringLiteral("step"), QStringLiteral("Unknown or expired flow"));
            return;
        }
        it->stepsTaken += 1;
        // The Redirect (matrix) flow completes in one step; the Form (token) flow advances through
        // a Message challenge before completing on the second step.
        const bool completeNow = it->family == QStringLiteral("matrix") || it->stepsTaken >= 2;
        if (!completeNow) {
            emit challenged(messageChallenge());
            return;
        }
        const QString bind = it->bindProfile;
        m_flows.erase(it);
        emit completed(QStringLiteral("cred/%1").arg(flowId), QStringLiteral("@mock:example.org"),
                       QStringLiteral("matrix/@mock:example.org"), bind);
    });
}

void MockAuthFlowService::cancel(const QString& flowId) {
    m_lastCancelled = flowId;
    m_flows.remove(flowId);
}

} // namespace auth
