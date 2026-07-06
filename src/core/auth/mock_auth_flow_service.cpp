// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/mock_auth_flow_service.h"

#include <QTimer>

namespace auth {

MockAuthFlowService::MockAuthFlowService(QObject* parent) : IAuthFlowService(parent) {
    // One SSO-shaped family (the Matrix analogue) so the begin form has a params schema to
    // render, mirroring what a live AuthProviders returns.
    QVariantMap field;
    field[QStringLiteral("key")] = QStringLiteral("homeserver");
    field[QStringLiteral("label")] = QStringLiteral("Homeserver");
    field[QStringLiteral("required")] = true;
    QVariantMap provider;
    provider[QStringLiteral("family")] = QStringLiteral("matrix");
    provider[QStringLiteral("flowKind")] = QStringLiteral("MatrixSso");
    provider[QStringLiteral("name")] = QStringLiteral("Matrix (SSO)");
    provider[QStringLiteral("params")] = QVariantList{field};
    m_providers.append(provider);
}

void MockAuthFlowService::refreshProviders() {
    QTimer::singleShot(0, this, [this] { emit providersChanged(); });
}

void MockAuthFlowService::begin(const QString& family, const QVariantMap& params,
                                const QString& redirectUri, const QString& bindProfile) {
    m_pendingBind = bindProfile;
    const QString host = params.value(QStringLiteral("homeserver")).toString();
    const quint64 flowSerial = ++m_flowSerial;
    const quint64 expiresAt = m_nextExpiresAt;
    m_nextExpiresAt = 0;
    QTimer::singleShot(0, this, [this, family, host, redirectUri, flowSerial, expiresAt] {
        if (!m_failBegin.isEmpty()) {
            const QString message = m_failBegin;
            m_failBegin.clear();
            emit flowFailed(QStringLiteral("begin"), message);
            return;
        }
        emit begun(QStringLiteral("mock-flow-%1").arg(flowSerial),
                   QStringLiteral("https://%1/_mock/sso?family=%2")
                       .arg(host.isEmpty() ? QStringLiteral("auth.example.org") : host, family),
                   redirectUri, expiresAt, QStringLiteral("MatrixSso"));
    });
}

void MockAuthFlowService::complete(const QString& flowId, const QString& callback) {
    QTimer::singleShot(0, this, [this, flowId, callback] {
        if (!m_failComplete.isEmpty()) {
            const QString message = m_failComplete;
            m_failComplete.clear();
            emit flowFailed(QStringLiteral("complete"), message);
            return;
        }
        Q_UNUSED(callback)
        emit completed(QStringLiteral("cred/%1").arg(flowId), QStringLiteral("@mock:example.org"),
                       QStringLiteral("matrix/@mock:example.org"), m_pendingBind);
    });
}

void MockAuthFlowService::cancel(const QString& flowId) {
    m_lastCancelled = flowId;
}

} // namespace auth
