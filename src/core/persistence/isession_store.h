// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/ids.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "domain/unit_node.h"

#include <QList>
#include <QObject>
#include <QString>

namespace persistence {

// The data seam. View models depend on this interface, never on a concrete
// store. The in-memory implementation is used now; a daemon-backed implementation
// (adapting `ControlApi.tree()` / `unit_outbound()`) can be dropped in later
// without touching the UI. Terminology mirrors the daemon: the supervision tree
// is made of UNITS (`UnitNode`/`UnitId`); a chat thread is a SESSION.
class ISessionStore : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~ISessionStore() override = default;

    // Sessions matching the given sidebar scope (metadata + content). For a Unit
    // scope this folds over the unit's whole subtree.
    [[nodiscard]] virtual QList<domain::Session> sessions(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual int sessionCount(const domain::ListScope& scope) const = 0;

    // --- SessionId-keyed API (the canonical session identity) ---
    // The authoritative string `SessionId` is the only session key into the store.
    // `domain::Session.id` (int) survives solely as a store-internal handle (the SQLite primary key
    // / list ordering) and is exposed by no method here.
    [[nodiscard]] virtual QString content(const domain::SessionId& id) const = 0;
    // The canonical session title (the same string the list shows), or an empty string when
    // unknown.
    [[nodiscard]] virtual QString title(const domain::SessionId& id) const = 0;
    [[nodiscard]] virtual bool isPinned(const domain::SessionId& id) const = 0;
    // Create a session under `unitId` and return its authoritative `SessionId`. Empty when the
    // backend owns lifecycle (the daemon cache).
    virtual domain::SessionId newSession(const domain::UnitId& unitId) = 0;
    virtual void setContent(const domain::SessionId& id, const QString& markdown) = 0;
    virtual void setArchived(const domain::SessionId& id, bool archived) = 0;
    virtual void renameSession(const domain::SessionId& id, const QString& title) = 0;
    virtual void deleteSession(const domain::SessionId& id) = 0;
    virtual void setPinned(const domain::SessionId& id, bool pinned) = 0;
    // Reorder within the store (delta < 0 moves earlier / up, > 0 later / down).
    virtual void moveSession(const domain::SessionId& id, int delta) = 0;

    // Ask the backend to (re)fetch the sessions bound to `profileId` — the per-agent view
    // (SessionScope::ByProfile, Fleet membership; daemon-supervision-spec §0). Default no-op: the
    // in-memory/mock store is already fully local; only the daemon cache issues a wire query and
    // then emits changed() when the rows land. Callers use it to drive an agent-scoped session list
    // from the authoritative node view.
    virtual void refreshSessionsForProfile(const QString& /*profileId*/) {}

    // Ask the backend to (re)fetch the ARCHIVED sessions (SessionScope::Archived; F6/DEL-6) — the
    // TopLevel roster excludes them, so an archived-scope surface (Settings ArchivedSection, the
    // Sessions/Fleet Archived toggle) requests this on entry and re-renders on changed(). Default
    // no-op: the in-memory/mock store already holds its archived rows locally.
    Q_INVOKABLE virtual void refreshArchivedSessions() {}

    // Ask the backend to (re)fetch the sessions routed over one transport instance
    // (SessionScope::ByTransport; B4/EIO-8) — the NODE resolves membership; the daemon store
    // records the id set and projects the ByTransport list scope from it, emitting changed()
    // when it lands. Default no-op: the in-memory/mock store derives the lens locally.
    virtual void refreshSessionsForTransport(const QString& /*transportId*/) {}

    // Node-authoritative session creation: ask the backend to CREATE a blank session bound to
    // `profileId` (empty = the node's active default agent). NOTHING is client-minted — the daemon
    // store sends a `SessionCreate` op and the node MINTS the id, replying `SessionCreated` +
    // emitting `RosterChanged`; the store then emits [`sessionCreated`] with the node-provided id
    // so the caller opens/selects it event-driven. Default: the in-memory/mock store mints locally
    // (`newSession`) and emits `sessionCreated` synchronously, so offscreen/mock coverage is
    // unaffected.
    Q_INVOKABLE virtual void requestNewSession(const QString& profileId = QString()) {
        const domain::SessionId id = newSession(domain::UnitId(profileId));
        if (!id.isEmpty()) {
            emit sessionCreated(id.toString(), profileId);
        }
    }
    // --- QML/TUI boundary: QString-keyed Q_INVOKABLE convenience overloads ---
    // QML and the TUI thread session ids as `QString` (empty = none); these inline shims construct
    // the typed `SessionId` and delegate to the canonical methods above. SessionId is not
    // implicitly constructible from QString, so these never clash with the int or SessionId
    // overloads.
    [[nodiscard]] Q_INVOKABLE QString content(const QString& id) const {
        return content(domain::SessionId(id));
    }
    [[nodiscard]] Q_INVOKABLE QString title(const QString& id) const {
        return title(domain::SessionId(id));
    }
    [[nodiscard]] Q_INVOKABLE bool isPinned(const QString& id) const {
        return isPinned(domain::SessionId(id));
    }
    Q_INVOKABLE void setContent(const QString& id, const QString& markdown) {
        setContent(domain::SessionId(id), markdown);
    }
    Q_INVOKABLE void setArchived(const QString& id, bool archived) {
        setArchived(domain::SessionId(id), archived);
    }
    Q_INVOKABLE void renameSession(const QString& id, const QString& title) {
        renameSession(domain::SessionId(id), title);
    }
    Q_INVOKABLE void deleteSession(const QString& id) { deleteSession(domain::SessionId(id)); }
    Q_INVOKABLE void setPinned(const QString& id, bool pinned) {
        setPinned(domain::SessionId(id), pinned);
    }
    Q_INVOKABLE void moveSession(const QString& id, int delta) {
        moveSession(domain::SessionId(id), delta);
    }
    // Create + return the new id as a QString (the QML/TUI form of newSession).
    Q_INVOKABLE QString newSessionId(const domain::UnitId& unitId) {
        return newSession(unitId).toString();
    }

signals:
    void changed();
    // A node-authoritative session was created (node-minted or, for the mock, local): `sessionId`
    // is the authoritative id, `profileId` the agent it was bound under (echoed from the request).
    // Callers open + auto-select this id (the node-authority replacement for a client-minted id).
    void sessionCreated(const QString& sessionId, const QString& profileId);
    // A node-owned metadata write (pin / archive / title) was REJECTED or failed. Daemon-backed
    // stores relay the node's reason here so the surface shows a failure signal instead of a silent
    // no-op that looks applied. The in-memory/mock store never emits it (its writes are local and
    // always succeed).
    void metaUpdateFailed(const QString& sessionId, const QString& message);
};

} // namespace persistence
