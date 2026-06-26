#pragma once

#include "domain/unit_node.h"
#include "domain/session.h"
#include "domain/ids.h"
#include "domain/participant.h"
#include "domain/sidebar_node.h"
#include "domain/tag.h"

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

    // The single recursive tree primitive: the direct children of `parentId`, or
    // the top-level roots when `parentId` is empty. Mirrors the daemon
    // `TreeReport` (a flat node list + per-node `children` ids) and is
    // arbitrary-depth by construction - callers must never assume a fixed depth.
    [[nodiscard]] virtual QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parentId) const = 0;

    // One unit by id (invalid UnitNode if unknown).
    [[nodiscard]] virtual domain::UnitNode unit(const domain::UnitId& id) const = 0;

    [[nodiscard]] virtual QList<domain::Tag> tags() const = 0;

    // The participants of the active chat/transcript (copied from the DaemonNet seed, like tags).
    [[nodiscard]] virtual QList<domain::Participant> participants() const = 0;

    // Sessions matching the given sidebar scope (metadata + content). For a Unit
    // scope this folds over the unit's whole subtree.
    [[nodiscard]] virtual QList<domain::Session>
    sessions(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual int sessionCount(const domain::ListScope& scope) const = 0;

    // --- SessionId-keyed API (the canonical session identity) ---
    // The authoritative string `SessionId` is the only session key into the store. `domain::Session.id`
    // (int) survives solely as a store-internal handle (the SQLite primary key / list ordering) and is
    // exposed by no method here.
    [[nodiscard]] virtual QString content(const domain::SessionId& id) const = 0;
    // The canonical session title (the same string the list shows), or an empty string when unknown.
    [[nodiscard]] virtual QString title(const domain::SessionId& id) const = 0;
    [[nodiscard]] virtual bool isPinned(const domain::SessionId& id) const = 0;
    // Create a session under `unitId` and return its authoritative `SessionId`. Empty when the backend
    // owns lifecycle (the daemon cache).
    virtual domain::SessionId newSession(const domain::UnitId& unitId) = 0;
    virtual void setContent(const domain::SessionId& id, const QString& markdown) = 0;
    virtual void setArchived(const domain::SessionId& id, bool archived) = 0;
    virtual void renameSession(const domain::SessionId& id, const QString& title) = 0;
    virtual void deleteSession(const domain::SessionId& id) = 0;
    virtual void setPinned(const domain::SessionId& id, bool pinned) = 0;
    // Reorder within the store (delta < 0 moves earlier / up, > 0 later / down).
    virtual void moveSession(const domain::SessionId& id, int delta) = 0;

    // Create a tree unit under `parentId` (empty = a new top-level root) and
    // return its new id. `kind` is cosmetic. Mirrors a future control-plane
    // "spawn unit" call.
    virtual domain::UnitId createUnit(const domain::UnitId& parentId, domain::UnitKind kind) = 0;
    // Create a cross-cutting tag and return its new id.
    virtual int createTag(const QString& name, const QString& color) = 0;

    // --- QML/TUI boundary: QString-keyed Q_INVOKABLE convenience overloads ---
    // QML and the TUI thread session ids as `QString` (empty = none); these inline shims construct the
    // typed `SessionId` and delegate to the canonical methods above. SessionId is not implicitly
    // constructible from QString, so these never clash with the int or SessionId overloads.
    [[nodiscard]] Q_INVOKABLE QString content(const QString& id) const
    {
        return content(domain::SessionId(id));
    }
    [[nodiscard]] Q_INVOKABLE QString title(const QString& id) const
    {
        return title(domain::SessionId(id));
    }
    [[nodiscard]] Q_INVOKABLE bool isPinned(const QString& id) const
    {
        return isPinned(domain::SessionId(id));
    }
    Q_INVOKABLE void setContent(const QString& id, const QString& markdown)
    {
        setContent(domain::SessionId(id), markdown);
    }
    Q_INVOKABLE void setArchived(const QString& id, bool archived)
    {
        setArchived(domain::SessionId(id), archived);
    }
    Q_INVOKABLE void renameSession(const QString& id, const QString& title)
    {
        renameSession(domain::SessionId(id), title);
    }
    Q_INVOKABLE void deleteSession(const QString& id) { deleteSession(domain::SessionId(id)); }
    Q_INVOKABLE void setPinned(const QString& id, bool pinned)
    {
        setPinned(domain::SessionId(id), pinned);
    }
    Q_INVOKABLE void moveSession(const QString& id, int delta)
    {
        moveSession(domain::SessionId(id), delta);
    }
    // Create + return the new id as a QString (the QML/TUI form of newSession).
    Q_INVOKABLE QString newSessionId(const domain::UnitId& unitId)
    {
        return newSession(unitId).toString();
    }

signals:
    void changed();
};

} // namespace persistence
