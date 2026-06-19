#include "persistence/in_memory_chat_store.h"

#include <QDateTime>

namespace persistence {

using domain::Conversation;
using domain::Folder;
using domain::ListScope;
using domain::NodeType;
using domain::Tag;

InMemoryChatStore::InMemoryChatStore(QObject* parent)
    : IChatStore(parent)
{
    seedSampleData();
}

void InMemoryChatStore::seedSampleData()
{
    m_folders = {
        { 1, -1, QStringLiteral("Work") },
        { 2, -1, QStringLiteral("Personal") },
    };
    m_tags = {
        { 1, QStringLiteral("ideas"), QStringLiteral("#2383e2") },
        { 2, QStringLiteral("todo"), QStringLiteral("#e2a423") },
    };

    const QDateTime now = QDateTime::currentDateTime();
    auto make = [&](int folderId, const QList<int>& tagIds, bool archived,
                    const QString& title, const QString& content) {
        Conversation c;
        c.id = m_nextId++;
        c.folderId = folderId;
        c.tagIds = tagIds;
        c.isArchived = archived;
        c.title = title;
        c.content = content;
        c.created = now;
        c.modified = now;
        m_conversations.push_back(c);
    };

    make(1, { 1 }, false, QStringLiteral("Project kickoff"),
         QStringLiteral("# Project kickoff\n\nLet's outline the **goals** for the quarter:\n\n"
                        "- Ship the foundation\n- Wire the seams\n- Keep it _fast_\n"));
    make(1, { 2 }, false, QStringLiteral("Release checklist"),
         QStringLiteral("## Release checklist\n\n1. Tag the build\n2. Update `UPDATES.json`\n"
                        "3. Announce\n"));
    make(2, {}, false, QStringLiteral("Weekend plans"),
         QStringLiteral("Thinking about a hike.\n\n> Bring water and snacks.\n"));
    make(2, { 1 }, true, QStringLiteral("Old brainstorm"),
         QStringLiteral("Archived notes from an earlier session.\n"));
}

bool InMemoryChatStore::matchesScope(const Conversation& c, const ListScope& scope) const
{
    switch (scope.type) {
    case NodeType::AllConversations:
        return !c.isArchived;
    case NodeType::Archived:
        return c.isArchived;
    case NodeType::Folder:
        return !c.isArchived && c.folderId == scope.id;
    case NodeType::Tag:
        return !c.isArchived && c.tagIds.contains(scope.id);
    case NodeType::FolderSeparator:
    case NodeType::TagSeparator:
        return false;
    }
    return false;
}

QList<Folder> InMemoryChatStore::folders() const
{
    return m_folders;
}

QList<Tag> InMemoryChatStore::tags() const
{
    return m_tags;
}

QList<Conversation> InMemoryChatStore::conversations(const ListScope& scope) const
{
    QList<Conversation> out;
    for (const Conversation& c : m_conversations) {
        if (matchesScope(c, scope)) {
            out.push_back(c);
        }
    }
    return out;
}

int InMemoryChatStore::conversationCount(const ListScope& scope) const
{
    int count = 0;
    for (const Conversation& c : m_conversations) {
        if (matchesScope(c, scope)) {
            ++count;
        }
    }
    return count;
}

QString InMemoryChatStore::content(int conversationId) const
{
    for (const Conversation& c : m_conversations) {
        if (c.id == conversationId) {
            return c.content;
        }
    }
    return {};
}

int InMemoryChatStore::createConversation(int folderId)
{
    Conversation c;
    c.id = m_nextId++;
    c.folderId = folderId;
    c.title = QStringLiteral("New conversation");
    c.created = QDateTime::currentDateTime();
    c.modified = c.created;
    m_conversations.push_back(c);
    emit changed();
    return c.id;
}

void InMemoryChatStore::setContent(int conversationId, const QString& markdown)
{
    for (Conversation& c : m_conversations) {
        if (c.id == conversationId) {
            c.content = markdown;
            c.modified = QDateTime::currentDateTime();
            emit changed();
            return;
        }
    }
}

void InMemoryChatStore::setArchived(int conversationId, bool archived)
{
    for (Conversation& c : m_conversations) {
        if (c.id == conversationId) {
            c.isArchived = archived;
            emit changed();
            return;
        }
    }
}

} // namespace persistence
