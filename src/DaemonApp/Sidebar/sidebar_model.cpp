#include "sidebar_model.h"

#include "persistence/ichat_store.h"

using domain::ListScope;
using domain::NodeType;

SidebarModel::SidebarModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QObject* SidebarModel::store() const
{
    return m_store;
}

void SidebarModel::setStore(QObject* store)
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
        connect(m_store, &persistence::IChatStore::changed, this, &SidebarModel::rebuild);
    }
    emit storeChanged();
    rebuild();
}

void SidebarModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    if (m_store) {
        m_rows.push_back({ tr("All Conversations"),
                           m_store->conversationCount({ NodeType::AllConversations, -1 }),
                           NodeType::AllConversations, -1, false, true, {} });
        m_rows.push_back({ tr("Archived"),
                           m_store->conversationCount({ NodeType::Archived, -1 }),
                           NodeType::Archived, -1, false, true, {} });

        m_rows.push_back({ tr("Folders"), -1, NodeType::FolderSeparator, -1, true, false, {} });
        for (const domain::Folder& f : m_store->folders()) {
            m_rows.push_back({ f.name, m_store->conversationCount({ NodeType::Folder, f.id }),
                               NodeType::Folder, f.id, false, true, {} });
        }

        m_rows.push_back({ tr("Tags"), -1, NodeType::TagSeparator, -1, true, false, {} });
        for (const domain::Tag& t : m_store->tags()) {
            m_rows.push_back({ t.name, m_store->conversationCount({ NodeType::Tag, t.id }),
                               NodeType::Tag, t.id, false, true, t.color });
        }
    }
    endResetModel();
}

int SidebarModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant SidebarModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case LabelRole:
        return r.label;
    case CountRole:
        return r.count;
    case NodeTypeRole:
        return static_cast<int>(r.type);
    case NodeIdRole:
        return r.nodeId;
    case IsSeparatorRole:
        return r.separator;
    case SelectableRole:
        return r.selectable;
    case ColorRole:
        return r.color;
    default:
        return {};
    }
}

QHash<int, QByteArray> SidebarModel::roleNames() const
{
    return {
        { LabelRole, "label" },
        { CountRole, "count" },
        { NodeTypeRole, "nodeType" },
        { NodeIdRole, "nodeId" },
        { IsSeparatorRole, "isSeparator" },
        { SelectableRole, "selectable" },
        { ColorRole, "color" },
    };
}

void SidebarModel::activate(int row)
{
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    if (r.selectable) {
        emit scopeSelected(static_cast<int>(r.type), r.nodeId);
    }
}
