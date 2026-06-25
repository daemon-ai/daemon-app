#pragma once

#include "daemon/daemon_cache_store.h"
#include "persistence/isession_store.h"

#include <QList>

namespace daemonapp::daemon {

class SessionRepository;

// Read-through ISessionStore projection over the DaemonCacheStore.
//
// This is the seam that lets existing sidebar/list view models consume daemon-backed sessions
// without knowing about CBOR, repositories, or the cache: it maps cached daemon session rows onto
// domain::Session and refreshes whenever the SessionRepository writes new data. Local int ids are
// presentation handles (snapshot indices); the daemon-authoritative key is domain::SessionId.
//
// Scope is the first daemon slice: sessions are flat (no unit tree / tags yet), mutations that the
// daemon owns (create/rename/delete/move/content) are intentionally inert here, while the
// client-local pinned/archived flags are written back into the cache so the UI stays responsive.
class CachedSessionStore : public persistence::ISessionStore {
    Q_OBJECT

public:
    CachedSessionStore(DaemonCacheStore* cache, SessionRepository* sessions,
                       QObject* parent = nullptr);

    [[nodiscard]] QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parentId) const override;
    [[nodiscard]] domain::UnitNode unit(const domain::UnitId& id) const override;
    [[nodiscard]] QList<domain::Tag> tags() const override;

    [[nodiscard]] QList<domain::Session> sessions(const domain::ListScope& scope) const override;
    [[nodiscard]] int sessionCount(const domain::ListScope& scope) const override;

    domain::UnitId createUnit(const domain::UnitId& parentId, domain::UnitKind kind) override;
    int createTag(const QString& name, const QString& color) override;

    // SessionId-keyed canonical API (matches the daemon-authoritative row.sessionId).
    [[nodiscard]] QString content(const domain::SessionId& id) const override;
    [[nodiscard]] QString title(const domain::SessionId& id) const override;
    [[nodiscard]] bool isPinned(const domain::SessionId& id) const override;
    domain::SessionId newSession(const domain::UnitId& unitId) override;
    void setContent(const domain::SessionId& id, const QString& markdown) override;
    void setArchived(const domain::SessionId& id, bool archived) override;
    void renameSession(const domain::SessionId& id, const QString& title) override;
    void deleteSession(const domain::SessionId& id) override;
    void setPinned(const domain::SessionId& id, bool pinned) override;
    void moveSession(const domain::SessionId& id, int delta) override;

private:
    void reload();
    [[nodiscard]] static bool matchesScope(const CachedSessionRow& row,
                                           const domain::ListScope& scope);
    // Index of the snapshot row with this SessionId, or -1. The bridge from the canonical id to the
    // cache row (and the int shims' resolution target).
    [[nodiscard]] int indexOfSessionId(const domain::SessionId& id) const;

    DaemonCacheStore* m_cache = nullptr;
    SessionRepository* m_sessions = nullptr;
    // Snapshot of the cache; the list index is the local int id handed to the UI.
    QList<CachedSessionRow> m_snapshot;
};

} // namespace daemonapp::daemon
