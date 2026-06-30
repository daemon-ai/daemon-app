// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/node_api_codec.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace daemonapp::daemon {

// The authenticated principal as surfaced to both front ends (the "WhoAmI" model). Populated from
// the wire `PrincipalView` on AuthOk and cleared on disconnect/logout. These capabilities are
// ADVISORY - for UI gating only; the node independently enforces every capability server-side, so a
// client must never treat hasCapability() as authorization.
//
// The capability/role names mirror daemon_auth::Capability / Role (snake_case): e.g.
// `access_admin`, `session_read`, `session_write`, `fleet_read`, `fleet_write`, `profile_read`,
// `profile_write`, `credential_read`, `credential_write`, `messaging_read`, `messaging_write`.
class PrincipalModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY changed)
    Q_PROPERTY(QString userId READ userId NOTIFY changed)
    Q_PROPERTY(QString username READ username NOTIFY changed)
    Q_PROPERTY(QStringList roles READ roles NOTIFY changed)
    Q_PROPERTY(QStringList capabilities READ capabilities NOTIFY changed)

public:
    using QObject::QObject;

    [[nodiscard]] bool authenticated() const { return m_authenticated; }
    [[nodiscard]] QString userId() const { return m_view.userId; }
    [[nodiscard]] QString username() const { return m_view.username; }
    [[nodiscard]] QStringList roles() const { return m_view.roles; }
    [[nodiscard]] QStringList capabilities() const { return m_view.capabilities; }
    [[nodiscard]] const DecodedPrincipalView& view() const { return m_view; }

    // Advisory UI gate: does the bound principal hold `capability`? Always false when
    // unauthenticated (fail-closed) - and never a substitute for the server's own enforcement.
    [[nodiscard]] Q_INVOKABLE bool hasCapability(const QString& capability) const {
        return m_authenticated && m_view.capabilities.contains(capability);
    }
    [[nodiscard]] Q_INVOKABLE bool hasRole(const QString& role) const {
        return m_authenticated && m_view.roles.contains(role);
    }

public slots:
    // Bind the principal from an AuthOk PrincipalView (wired to NodeApiClient::authenticated).
    void setPrincipal(const daemonapp::daemon::DecodedPrincipalView& view) {
        m_view = view;
        m_authenticated = view.isAuthenticated();
        emit changed();
    }
    // Clear on disconnect/logout so capability-gated surfaces re-gate closed.
    void clear() {
        if (!m_authenticated && m_view.userId.isEmpty()) {
            return;
        }
        m_view = DecodedPrincipalView{};
        m_authenticated = false;
        emit changed();
    }

signals:
    void changed();

private:
    DecodedPrincipalView m_view;
    bool m_authenticated = false;
};

} // namespace daemonapp::daemon
