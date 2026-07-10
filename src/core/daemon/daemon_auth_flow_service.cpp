// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_auth_flow_service.h"

#include "daemon/repositories.h"

namespace auth {

using daemonapp::daemon::AuthChallengeKind;
using daemonapp::daemon::AuthRepository;
using daemonapp::daemon::AuthStepInputKind;
using daemonapp::daemon::DecodedAuthBeginResponse;
using daemonapp::daemon::DecodedAuthChallenge;
using daemonapp::daemon::DecodedAuthCompleteResponse;
using daemonapp::daemon::DecodedAuthParamField;
using daemonapp::daemon::DecodedAuthProviderInfo;
using daemonapp::daemon::DecodedAuthStepResult;

namespace {
// [integrations wire v38] Project one enriched auth-param-field into the plain field map the seam
// carries (see the shape doc in iauth_flow_service.h). Shared by the provider params and the Form
// challenge fields — the render metadata (kind/default/placeholder/choices) is what lets A3's
// generic form mask secrets (kind == "Password"), prefill, hint, and offer choices without ever
// seeing a wire type. Absent optionals project as empty ("" / []), kind defaults to "Text".
QVariantMap fieldToVariant(const DecodedAuthParamField& f) {
    QVariantMap field;
    field[QStringLiteral("key")] = f.key;
    field[QStringLiteral("label")] = f.label;
    field[QStringLiteral("required")] = f.required;
    field[QStringLiteral("kind")] = f.kind;
    field[QStringLiteral("default")] = f.hasDefault ? f.defaultValue : QString();
    field[QStringLiteral("placeholder")] = f.hasPlaceholder ? f.placeholder : QString();
    field[QStringLiteral("choices")] = QVariant(f.choices);
    return field;
}

// Project a decoded (wire-derived) challenge into the plain challenge map the seam carries (keeping
// the wire types confined to the codec/repository layer). See the shape doc in
// iauth_flow_service.h.
QVariantMap challengeToVariant(const DecodedAuthChallenge& c) {
    QVariantMap map;
    switch (c.kind) {
    case AuthChallengeKind::Redirect:
        map[QStringLiteral("kind")] = QStringLiteral("redirect");
        map[QStringLiteral("authorizationUrl")] = c.authorizationUrl;
        break;
    case AuthChallengeKind::Form: {
        map[QStringLiteral("kind")] = QStringLiteral("form");
        map[QStringLiteral("title")] = c.formTitle;
        QVariantList fields;
        for (const DecodedAuthParamField& f : c.formFields) {
            fields.append(fieldToVariant(f));
        }
        map[QStringLiteral("fields")] = fields;
        break;
    }
    case AuthChallengeKind::Qr:
        map[QStringLiteral("kind")] = QStringLiteral("qr");
        map[QStringLiteral("payload")] = c.qrPayload;
        map[QStringLiteral("image")] = c.qrImage;
        map[QStringLiteral("pollIntervalMs")] = static_cast<qulonglong>(c.qrPollIntervalMs);
        break;
    case AuthChallengeKind::Message:
        map[QStringLiteral("kind")] = QStringLiteral("message");
        map[QStringLiteral("text")] = c.messageText;
        break;
    }
    return map;
}
} // namespace

DaemonAuthFlowService::DaemonAuthFlowService(AuthRepository* repository, QObject* parent)
    : IAuthFlowService(parent), m_repository(repository) {
    if (m_repository == nullptr) {
        return;
    }
    connect(m_repository, &AuthRepository::providersRefreshed, this,
            &IAuthFlowService::providersChanged);
    connect(m_repository, &AuthRepository::begun, this, [this](const DecodedAuthBeginResponse& r) {
        emit begun(r.flowId, challengeToVariant(r.challenge), r.expiresAt);
    });
    connect(m_repository, &AuthRepository::stepped, this, [this](const DecodedAuthStepResult& r) {
        if (r.completed) {
            emit completed(r.completion.credentialRef, r.completion.accountLabel,
                           r.completion.transportInstance,
                           r.completion.hasBoundProfile ? r.completion.boundProfile : QString());
        } else {
            emit challenged(challengeToVariant(r.challenge));
        }
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
            params.append(fieldToVariant(f));
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

void DaemonAuthFlowService::step(const QString& flowId, const StepInput& input) {
    if (m_repository == nullptr) {
        emit flowFailed(QStringLiteral("step"), QStringLiteral("No daemon connection"));
        return;
    }
    AuthStepInputKind kind = AuthStepInputKind::Poll;
    switch (input.kind) {
    case StepInputKind::Fields:
        kind = AuthStepInputKind::Fields;
        break;
    case StepInputKind::Callback:
        kind = AuthStepInputKind::Callback;
        break;
    case StepInputKind::Poll:
        kind = AuthStepInputKind::Poll;
        break;
    }
    m_repository->step(flowId, kind, input.fields, input.callback);
}

void DaemonAuthFlowService::cancel(const QString& flowId) {
    if (m_repository != nullptr) {
        m_repository->cancel(flowId);
    }
}

} // namespace auth
