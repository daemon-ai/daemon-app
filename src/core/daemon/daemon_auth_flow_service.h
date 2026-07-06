// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "auth/iauth_flow_service.h"

namespace daemonapp::daemon {
class AuthRepository;
}

namespace auth {

// Daemon-backed interactive-auth seam: a thin projection of AuthRepository (the wire seam) onto
// IAuthFlowService rows/signals. Stateless — flow state lives in AuthFlowController; wire op
// names live in the repository + codec only. Lives under src/core/daemon because it depends on
// the daemon repositories (same placement rationale as DaemonAccountsService).
class DaemonAuthFlowService : public IAuthFlowService {
    Q_OBJECT

public:
    explicit DaemonAuthFlowService(daemonapp::daemon::AuthRepository* repository,
                                   QObject* parent = nullptr);

    [[nodiscard]] QVariantList providers() const override;
    void refreshProviders() override;
    void begin(const QString& family, const QVariantMap& params, const QString& redirectUri,
               const QString& bindProfile = QString()) override;
    void complete(const QString& flowId, const QString& callback) override;
    void cancel(const QString& flowId) override;

private:
    daemonapp::daemon::AuthRepository* m_repository = nullptr;
};

} // namespace auth
