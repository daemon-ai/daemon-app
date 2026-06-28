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

namespace fleet {

// Daemon-backed, offline-first session roster: projects the (already offline-first)
// CachedSessionStore into the ISessionRoster VariantListModel, mirroring DaemonFleetTree. Renders
// the last-known sessions from cache with no connection and rebuilds on ISessionStore::changed.
//
// There is no session-lifecycle wire op today, so the client suspend/resume buttons keep a
// client-local cosmetic overlay (m_suspended) over the wire state; close archives client-side
// (CachedSessionStore::setArchived), which drops the row from the AllSessions scope. A durable
// Suspend/Close op is a follow-up.
class DaemonSessionRoster : public ISessionRoster {
    Q_OBJECT

public:
    explicit DaemonSessionRoster(persistence::ISessionStore* store, QObject* parent = nullptr);

    [[nodiscard]] QObject* sessions() const override;
    [[nodiscard]] int count() const override;
    void suspend(const QString& id) override;
    void resume(const QString& id) override;
    void close(const QString& id) override;

private:
    void rebuild();

    persistence::ISessionStore* m_store = nullptr;
    uimodels::VariantListModel* m_sessions = nullptr;
    QSet<QString> m_suspended; // client-local cosmetic overlay (no session-suspend wire op)
};

} // namespace fleet
