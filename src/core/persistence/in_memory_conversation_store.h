#pragma once

#include "persistence/iconversation_store.h"

namespace persistence {

// In-memory IConversationStore seeded with sample data, so the UI is fully alive
// without a backend. Not persisted across runs. The sample data is a deliberate
// fleet-of-fleets that exercises arbitrary depth (orchestrators nested inside
// orchestrators) and a lone-agent root, so the recursive tree code is tested.
class InMemoryConversationStore : public IConversationStore {
    Q_OBJECT

public:
    explicit InMemoryConversationStore(QObject* parent = nullptr);

    [[nodiscard]] QList<domain::AgentNode>
    agentChildren(const QString& parentId) const override;
    [[nodiscard]] domain::AgentNode agentNode(const QString& id) const override;
    [[nodiscard]] QList<domain::Tag> tags() const override;
    [[nodiscard]] QList<domain::Conversation>
    conversations(const domain::ListScope& scope) const override;
    [[nodiscard]] int conversationCount(const domain::ListScope& scope) const override;
    [[nodiscard]] QString content(int conversationId) const override;

    int createConversation(const QString& agentId) override;
    QString createNode(const QString& parentId, domain::AgentNodeKind kind) override;
    int createTag(const QString& name, const QString& color) override;
    void setContent(int conversationId, const QString& markdown) override;
    void setArchived(int conversationId, bool archived) override;

private:
    [[nodiscard]] bool matchesScope(const domain::Conversation& c,
                                    const domain::ListScope& scope) const;
    // True when `nodeId` is `rootId` or any descendant of it (subtree fold).
    // Walks the parent chain - a single recursive rule for every depth.
    [[nodiscard]] bool isInSubtree(const QString& nodeId, const QString& rootId) const;
    void seedSampleData();
    // Canonical demo transcript markdown exercising every Phase 1 agent block,
    // seeded as a conversation for visual inspection of the renderers.
    [[nodiscard]] static QString agentBlocksSampleMarkdown();

    QList<domain::AgentNode> m_nodes;
    QList<domain::Tag> m_tags;
    QList<domain::Conversation> m_conversations;
    int m_nextId = 1;        // conversation ids
    int m_nextNodeSeq = 1;   // suffix for generated node ids
    int m_nextTagId = 1;     // tag ids
};

} // namespace persistence
