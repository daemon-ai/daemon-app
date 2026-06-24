#pragma once

#include "domain/unit_node.h"
#include "domain/session.h"
#include "domain/ids.h"
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

    // Sessions matching the given sidebar scope (metadata + content). For a Unit
    // scope this folds over the unit's whole subtree.
    [[nodiscard]] virtual QList<domain::Session>
    sessions(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual int sessionCount(const domain::ListScope& scope) const = 0;

    [[nodiscard]] Q_INVOKABLE virtual QString content(int sessionId) const = 0;

    // The canonical session title (the same string the list shows), or an empty
    // string when unknown. Q_INVOKABLE so QML tab hosts can label chips with the
    // real title instead of guessing from content.
    [[nodiscard]] Q_INVOKABLE virtual QString title(int sessionId) const = 0;

    // Mutations. Each emits changed() so models can refresh.
    virtual int createSession(const domain::UnitId& unitId) = 0;
    // Create a tree unit under `parentId` (empty = a new top-level root) and
    // return its new id. `kind` is cosmetic. Mirrors a future control-plane
    // "spawn unit" call.
    virtual domain::UnitId createUnit(const domain::UnitId& parentId, domain::UnitKind kind) = 0;
    // Create a cross-cutting tag and return its new id.
    virtual int createTag(const QString& name, const QString& color) = 0;
    virtual void setContent(int sessionId, const QString& markdown) = 0;
    // Q_INVOKABLE so the Settings -> Archived chats section can archive/unarchive.
    Q_INVOKABLE virtual void setArchived(int sessionId, bool archived) = 0;

    // Session actions (client-side now; the daemon adapter forwards them later).
    // Each emits changed() so every bound model refreshes.
    Q_INVOKABLE virtual void renameSession(int sessionId, const QString& title) = 0;
    Q_INVOKABLE virtual void deleteSession(int sessionId) = 0;
    Q_INVOKABLE virtual void setPinned(int sessionId, bool pinned) = 0;
    [[nodiscard]] Q_INVOKABLE virtual bool isPinned(int sessionId) const = 0;
    // Reorder within the store (delta < 0 moves earlier / up, > 0 later / down).
    Q_INVOKABLE virtual void moveSession(int sessionId, int delta) = 0;

signals:
    void changed();
};

} // namespace persistence
