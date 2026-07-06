// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/isession_roster.h"

#include <QSet>
#include <QString>

namespace uimodels {
class VariantListModel;
}
namespace persistence {
class ISessionStore;
}
namespace daemonapp::daemon {
class SessionRepository;
}

namespace fleet {

// Daemon-backed, offline-first session roster: projects the (already offline-first)
// CachedSessionStore into the ISessionRoster VariantListModel, mirroring DaemonFleetTree. Renders
// the last-known sessions from cache with no connection and rebuilds on ISessionStore::changed.
//
// Scope (F6/DEL-6): "active" projects the AllSessions scope; "archived" projects the Archived
// scope (fetched on demand from the node — TopLevel excludes archived rows) with per-row
// restore() (SessionUpdateMeta{archived:false} via the store's setArchived).
//
// Operator steer/interrupt (F4/DEL-4): forwarded to the SessionRepository's session-addressable
// Submit ops; a node rejection surfaces via operationFailed.
//
// There is no session-lifecycle wire op today, so the client suspend/resume buttons keep a
// client-local cosmetic overlay (m_suspended) over the wire state; close archives client-side
// (CachedSessionStore::setArchived), which drops the row from the AllSessions scope. A durable
// Suspend/Close op is a follow-up.
class DaemonSessionRoster : public ISessionRoster {
    Q_OBJECT

public:
    explicit DaemonSessionRoster(persistence::ISessionStore* store,
                                 daemonapp::daemon::SessionRepository* repo = nullptr,
                                 QObject* parent = nullptr);

    [[nodiscard]] QObject* sessions() const override;
    [[nodiscard]] int count() const override;
    [[nodiscard]] QString scope() const override { return m_scope; }
    void setScope(const QString& scope) override;
    void suspend(const QString& id) override;
    void resume(const QString& id) override;
    void close(const QString& id) override;
    void restore(const QString& id) override;
    void steer(const QString& sessionId, const QString& text) override;
    void startTurn(const QString& sessionId, const QString& text) override;
    void interrupt(const QString& sessionId) override;

private:
    void rebuild();

    persistence::ISessionStore* m_store = nullptr;
    daemonapp::daemon::SessionRepository* m_repo = nullptr; // operator submit ops (may be null)
    uimodels::VariantListModel* m_sessions = nullptr;
    QString m_scope = QStringLiteral("active");
    QSet<QString> m_suspended; // client-local cosmetic overlay (no session-suspend wire op)
};

} // namespace fleet
