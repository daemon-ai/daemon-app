#pragma once

#include "persistence/iconversation_store.h"

namespace persistence {

// In-memory IConversationStore seeded with sample data, so the UI is fully alive
// without a database. Not persisted across runs.
class InMemoryConversationStore : public IConversationStore {
    Q_OBJECT

public:
    explicit InMemoryConversationStore(QObject* parent = nullptr);

    [[nodiscard]] QList<domain::Folder> folders() const override;
    [[nodiscard]] QList<domain::Tag> tags() const override;
    [[nodiscard]] QList<domain::Conversation>
    conversations(const domain::ListScope& scope) const override;
    [[nodiscard]] int conversationCount(const domain::ListScope& scope) const override;
    [[nodiscard]] QString content(int conversationId) const override;

    int createConversation(int folderId) override;
    void setContent(int conversationId, const QString& markdown) override;
    void setArchived(int conversationId, bool archived) override;

private:
    [[nodiscard]] bool matchesScope(const domain::Conversation& c,
                                    const domain::ListScope& scope) const;
    void seedSampleData();

    QList<domain::Folder> m_folders;
    QList<domain::Tag> m_tags;
    QList<domain::Conversation> m_conversations;
    int m_nextId = 1;
};

} // namespace persistence
