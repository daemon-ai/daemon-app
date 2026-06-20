#pragma once

#include "domain/agent_node.h"
#include "domain/conversation.h"
#include "domain/sidebar_node.h"
#include "domain/tag.h"

#include <QList>
#include <QObject>
#include <QString>

namespace persistence {

// The data seam. View models depend on this interface, never on a concrete
// store. The in-memory implementation is used now; a daemon-backed
// implementation (adapting `ControlApi.tree()` / `unit_outbound()`) can be
// dropped in later without touching the UI.
class IConversationStore : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IConversationStore() override = default;

    // The single recursive tree primitive: the direct children of `parentId`,
    // or the top-level roots when `parentId` is empty. Mirrors the daemon
    // `TreeReport` (a flat node list + per-node `children` ids) and is
    // arbitrary-depth by construction - callers must never assume a fixed depth.
    [[nodiscard]] virtual QList<domain::AgentNode>
    agentChildren(const QString& parentId) const = 0;

    // One node by id (invalid AgentNode if unknown).
    [[nodiscard]] virtual domain::AgentNode agentNode(const QString& id) const = 0;

    [[nodiscard]] virtual QList<domain::Tag> tags() const = 0;

    // Conversations matching the given sidebar scope (metadata + content). For a
    // Node scope this folds over the node's whole subtree.
    [[nodiscard]] virtual QList<domain::Conversation>
    conversations(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual int conversationCount(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual QString content(int conversationId) const = 0;

    // Mutations. Each emits changed() so models can refresh.
    virtual int createConversation(const QString& agentId) = 0;
    // Create a tree node under `parentId` (empty = a new top-level root) and
    // return its new id. `kind` is cosmetic. Mirrors a future control-plane
    // "spawn unit" call.
    virtual QString createNode(const QString& parentId, domain::AgentNodeKind kind) = 0;
    // Create a cross-cutting tag and return its new id.
    virtual int createTag(const QString& name, const QString& color) = 0;
    virtual void setContent(int conversationId, const QString& markdown) = 0;
    virtual void setArchived(int conversationId, bool archived) = 0;

signals:
    void changed();
};

} // namespace persistence
