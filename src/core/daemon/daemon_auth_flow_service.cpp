// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_auth_flow_service.h"

#include "daemon/repositories.h"

namespace auth {

using daemonapp::daemon::AuthRepository;
using daemonapp::daemon::DecodedAuthBeginResponse;
using daemonapp::daemon::DecodedAuthCompleteResponse;
using daemonapp::daemon::DecodedAuthParamField;
using daemonapp::daemon::DecodedAuthProviderInfo;

DaemonAuthFlowService::DaemonAuthFlowService(AuthRepository* repository, QObject* parent)
    : IAuthFlowService(parent), m_repository(repository) {
    if (m_repository == nullptr) {
        return;
    }
    connect(m_repository, &AuthRepository::providersRefreshed, this,
            &IAuthFlowService::providersChanged);
    connect(m_repository, &AuthRepository::begun, this, [this](const DecodedAuthBeginResponse& r) {
        emit begun(r.flowId, r.authorizationUrl, r.redirectUri, r.expiresAt, r.flowKind);
    });
    connect(m_repository, &AuthRepository::completed, this,
            [this](const DecodedAuthCompleteResponse& r) {
                emit completed(r.credentialRef, r.accountLabel, r.transportInstance,
                               r.hasBoundProfile ? r.boundProfile : QString());
            });
    connect(m_repository, &AuthRepository::failed, this, &IAuthFlowService::flowFailed);
}

QVariantList DaemonAuthFlowService::providers() const {
    QVariantList rows;
    if (m_repository == nullptr) {
        return rows;
    }
    for (const DecodedAuthProviderInfo& p : m_repository->providers()) {
        QVariantList params;
        for (const DecodedAuthParamField& f : p.paramsSchema) {
            QVariantMap field;
            field[QStringLiteral("key")] = f.key;
            field[QStringLiteral("label")] = f.label;
            field[QStringLiteral("required")] = f.required;
            params.append(field);
        }
        QVariantMap row;
        row[QStringLiteral("family")] = p.family;
        row[QStringLiteral("flowKind")] = p.flowKind;
        row[QStringLiteral("name")] = p.displayName.isEmpty() ? p.family : p.displayName;
        row[QStringLiteral("params")] = params;
        rows.append(row);
    }
    return rows;
}

void DaemonAuthFlowService::refreshProviders() {
    if (m_repository != nullptr) {
        m_repository->refreshProviders();
    }
}

void DaemonAuthFlowService::begin(const QString& family, const QVariantMap& params,
                                  const QString& redirectUri, const QString& bindProfile) {
    if (m_repository == nullptr) {
        emit flowFailed(QStringLiteral("begin"), QStringLiteral("No daemon connection"));
        return;
    }
    m_repository->begin(family, params, redirectUri, bindProfile);
}

void DaemonAuthFlowService::complete(const QString& flowId, const QString& callback) {
    if (m_repository == nullptr) {
        emit flowFailed(QStringLiteral("complete"), QStringLiteral("No daemon connection"));
        return;
    }
    m_repository->complete(flowId, callback);
}

void DaemonAuthFlowService::cancel(const QString& flowId) {
    if (m_repository != nullptr) {
        m_repository->cancel(flowId);
    }
}

} // namespace auth
