#include "conversations_list_model.h"

#include "persistence/ichat_store.h"

using domain::Conversation;
using domain::ListScope;
using domain::NodeType;

namespace {

QString snippetOf(const Conversation& c)
{
    QString text = c.content;
    text.replace(QLatin1Char('\n'), QLatin1Char(' '));
    text = text.simplified();
    constexpr int kMax = 80;
    if (text.size() > kMax) {
        text.truncate(kMax);
        text += QStringLiteral("...");
    }
    return text;
}

} // namespace

ConversationsListModel::ConversationsListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QObject* ConversationsListModel::store() const
{
    return m_store;
}

void ConversationsListModel::setStore(QObject* store)
{
    auto* chatStore = qobject_cast<persistence::IChatStore*>(store);
    if (m_store == chatStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = chatStore;
    if (m_store) {
        connect(m_store, &persistence::IChatStore::changed, this,
                &ConversationsListModel::reload);
    }
    emit storeChanged();
    reload();
}

void ConversationsListModel::setSearch(const QString& search)
{
    if (m_search == search) {
        return;
    }
    m_search = search;
    emit searchChanged();
    applyFilter();
}

void ConversationsListModel::setScope(int nodeType, int nodeId)
{
    m_scope = { static_cast<NodeType>(nodeType), nodeId };
    reload();
    emit scopeChanged();
}

void ConversationsListModel::reload()
{
    m_all = m_store ? m_store->conversations(m_scope) : QList<Conversation>{};
    m_scopeTitle = computeScopeTitle();
    emit scopeChanged();
    applyFilter();
}

void ConversationsListModel::applyFilter()
{
    beginResetModel();
    m_filtered.clear();
    for (const Conversation& c : m_all) {
        if (m_search.isEmpty()
            || c.title.contains(m_search, Qt::CaseInsensitive)
            || c.content.contains(m_search, Qt::CaseInsensitive)) {
            m_filtered.push_back(c);
        }
    }
    endResetModel();
    emit countChanged();
}

QString ConversationsListModel::computeScopeTitle() const
{
    switch (m_scope.type) {
    case NodeType::AllConversations:
        return tr("All Conversations");
    case NodeType::Archived:
        return tr("Archived");
    case NodeType::Folder:
        if (m_store) {
            for (const domain::Folder& f : m_store->folders()) {
                if (f.id == m_scope.id) {
                    return f.name;
                }
            }
        }
        return tr("Folder");
    case NodeType::Tag:
        if (m_store) {
            for (const domain::Tag& t : m_store->tags()) {
                if (t.id == m_scope.id) {
                    return t.name;
                }
            }
        }
        return tr("Tag");
    case NodeType::FolderSeparator:
    case NodeType::TagSeparator:
        break;
    }
    return tr("Conversations");
}

int ConversationsListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_filtered.size());
}

QVariant ConversationsListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_filtered.size()) {
        return {};
    }
    const Conversation& c = m_filtered.at(index.row());
    switch (role) {
    case IdRole:
        return c.id;
    case TitleRole:
        return c.title;
    case SnippetRole:
        return snippetOf(c);
    case ModifiedRole:
        return c.modified;
    default:
        return {};
    }
}

QHash<int, QByteArray> ConversationsListModel::roleNames() const
{
    return {
        { IdRole, "conversationId" },
        { TitleRole, "title" },
        { SnippetRole, "snippet" },
        { ModifiedRole, "modified" },
    };
}

int ConversationsListModel::idAt(int row) const
{
    if (row < 0 || row >= m_filtered.size()) {
        return -1;
    }
    return m_filtered.at(row).id;
}
