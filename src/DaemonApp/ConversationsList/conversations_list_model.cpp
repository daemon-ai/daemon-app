#include "conversations_list_model.h"

#include "domain/agent_node.h"
#include "persistence/iconversation_store.h"

using domain::AgentNode;
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
    auto* conversationStore = qobject_cast<persistence::IConversationStore*>(store);
    if (m_store == conversationStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = conversationStore;
    if (m_store) {
        connect(m_store, &persistence::IConversationStore::changed, this,
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

void ConversationsListModel::setScope(int nodeType, int id, const QString& nodeId)
{
    m_scope = { static_cast<NodeType>(nodeType), id, nodeId };
    // A new scope is a fresh list of conversations; drop the old selection so a
    // stale id doesn't linger as a phantom highlight.
    if (m_currentId != -1) {
        m_currentId = -1;
        emit selectionChanged(-1);
    }
    reload();
    emit scopeChanged();
}

void ConversationsListModel::reload()
{
    rebuildLookups();
    m_all = m_store ? m_store->conversations(m_scope) : QList<Conversation>{};
    m_scopeTitle = computeScopeTitle();
    emit scopeChanged();
    applyFilter();
}

void ConversationsListModel::rebuildLookups()
{
    m_nodeInfo.clear();
    m_tagInfo.clear();
    if (!m_store) {
        return;
    }
    // Walk the whole agent tree via the single recursive primitive so each
    // conversation's owning-node name/kind can be resolved (any depth).
    persistence::IConversationStore* store = m_store;
    auto collect = [store, this](const QString& parentId, auto&& self) -> void {
        for (const AgentNode& n : store->agentChildren(parentId)) {
            m_nodeInfo.insert(n.id, { n.name, static_cast<int>(n.kind) });
            self(n.id, self);
        }
    };
    collect(QString(), collect);

    for (const domain::Tag& t : m_store->tags()) {
        m_tagInfo.insert(t.id, { t.name, t.color });
    }
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
    case NodeType::Node:
        if (m_store) {
            const AgentNode n = m_store->agentNode(m_scope.nodeId);
            if (n.isValid()) {
                return n.name;
            }
        }
        return tr("Agent");
    case NodeType::Tag:
        if (m_store) {
            for (const domain::Tag& t : m_store->tags()) {
                if (t.id == m_scope.id) {
                    return t.name;
                }
            }
        }
        return tr("Tag");
    case NodeType::FleetSeparator:
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
    case AgentNameRole:
        return m_nodeInfo.value(c.agentId).first;
    case AgentKindRole:
        return m_nodeInfo.value(c.agentId).second;
    case TagNamesRole: {
        QStringList names;
        for (int tagId : c.tagIds) {
            names.append(m_tagInfo.value(tagId).first);
        }
        return names;
    }
    case TagColorsRole: {
        QStringList colors;
        for (int tagId : c.tagIds) {
            colors.append(m_tagInfo.value(tagId).second);
        }
        return colors;
    }
    case CurrentRole:
        return c.id == m_currentId;
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
        { AgentNameRole, "agentName" },
        { AgentKindRole, "agentKind" },
        { TagNamesRole, "tagNames" },
        { TagColorsRole, "tagColors" },
        { CurrentRole, "current" },
    };
}

int ConversationsListModel::idAt(int row) const
{
    if (row < 0 || row >= m_filtered.size()) {
        return -1;
    }
    return m_filtered.at(row).id;
}

void ConversationsListModel::emitCurrentChanged()
{
    if (!m_filtered.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_filtered.size()) - 1), { CurrentRole });
    }
}

void ConversationsListModel::setCurrentId(int id)
{
    if (m_currentId == id) {
        return;
    }
    m_currentId = id;
    emit selectionChanged(id);
    emitCurrentChanged();
}

void ConversationsListModel::activate(int row)
{
    const int id = idAt(row);
    if (id >= 0) {
        setCurrentId(id);
    }
}

void ConversationsListModel::selectNext()
{
    const int cur = currentRow();
    const int next = cur < 0 ? 0 : cur + 1;
    if (next < m_filtered.size()) {
        setCurrentId(m_filtered.at(next).id);
    }
}

void ConversationsListModel::selectPrevious()
{
    const int cur = currentRow();
    if (cur > 0) {
        setCurrentId(m_filtered.at(cur - 1).id);
    }
}

int ConversationsListModel::currentRow() const
{
    if (m_currentId < 0) {
        return -1;
    }
    for (int i = 0; i < m_filtered.size(); ++i) {
        if (m_filtered.at(i).id == m_currentId) {
            return i;
        }
    }
    return -1;
}
