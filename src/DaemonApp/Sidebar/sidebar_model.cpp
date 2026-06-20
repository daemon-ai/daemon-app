#include "sidebar_model.h"

#include "domain/agent_node.h"
#include "persistence/iconversation_store.h"

using domain::AgentNode;
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
    auto* conversationStore = qobject_cast<persistence::IConversationStore*>(store);
    if (m_store == conversationStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = conversationStore;
    if (m_store) {
        connect(m_store, &persistence::IConversationStore::changed, this, &SidebarModel::rebuild);
    }
    emit storeChanged();
    rebuild();
}

bool SidebarModel::isExpanded(const QString& id) const
{
    return !m_collapsed.contains(id);
}

void SidebarModel::appendNodeRows(const AgentNode& node, int depth)
{
    const QList<AgentNode> children = m_store->agentChildren(node.id);
    const bool expanded = isExpanded(node.id);

    Row r;
    r.label = node.name;
    r.count = m_store->conversationCount({ NodeType::Node, -1, node.id });
    r.type = NodeType::Node;
    r.agentId = node.id;
    r.selectable = true;
    r.depth = depth;
    r.hasChildren = !children.isEmpty();
    r.expanded = expanded;
    r.kind = static_cast<int>(node.kind);
    r.state = static_cast<int>(node.state);
    m_rows.push_back(r);

    // Uniformly recursive: descend into any expanded node, to any depth. No
    // branching on kind, no level cap.
    if (r.hasChildren && expanded) {
        for (const AgentNode& child : children) {
            appendNodeRows(child, depth + 1);
        }
    }
}

void SidebarModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    if (m_store) {
        m_rows.push_back({ tr("All Conversations"),
                           m_store->conversationCount({ NodeType::AllConversations, -1, {} }),
                           NodeType::AllConversations, -1, {}, false, true, {}, 0, false, false, 0, 0 });
        m_rows.push_back({ tr("Archived"),
                           m_store->conversationCount({ NodeType::Archived, -1, {} }),
                           NodeType::Archived, -1, {}, false, true, {}, 0, false, false, 0, 0 });

        m_rows.push_back({ tr("Fleet"), -1, NodeType::FleetSeparator, -1, {}, true, false, {},
                           0, false, false, 0, 0 });
        // Top-level roots have an empty parent; each may be a lone agent or the
        // head of an arbitrarily deep fleet.
        for (const AgentNode& root : m_store->agentChildren(QString())) {
            appendNodeRows(root, 0);
        }

        m_rows.push_back({ tr("Tags"), -1, NodeType::TagSeparator, -1, {}, true, false, {},
                           0, false, false, 0, 0 });
        for (const domain::Tag& t : m_store->tags()) {
            m_rows.push_back({ t.name, m_store->conversationCount({ NodeType::Tag, t.id, {} }),
                               NodeType::Tag, t.id, {}, false, true, t.color,
                               0, false, false, 0, 0 });
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
    case AgentIdRole:
        return r.agentId;
    case IsSeparatorRole:
        return r.separator;
    case SelectableRole:
        return r.selectable;
    case ColorRole:
        return r.color;
    case DepthRole:
        return r.depth;
    case HasChildrenRole:
        return r.hasChildren;
    case ExpandedRole:
        return r.expanded;
    case KindRole:
        return r.kind;
    case StateRole:
        return r.state;
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
        { AgentIdRole, "agentId" },
        { IsSeparatorRole, "isSeparator" },
        { SelectableRole, "selectable" },
        { ColorRole, "color" },
        { DepthRole, "depth" },
        { HasChildrenRole, "hasChildren" },
        { ExpandedRole, "expanded" },
        { KindRole, "kind" },
        { StateRole, "state" },
    };
}

void SidebarModel::activate(int row)
{
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    if (r.selectable) {
        emit scopeSelected(static_cast<int>(r.type), r.nodeId, r.agentId);
    }
}

void SidebarModel::toggleExpand(int row)
{
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    if (r.type != NodeType::Node || !r.hasChildren) {
        return;
    }
    if (m_collapsed.contains(r.agentId)) {
        m_collapsed.remove(r.agentId);
    } else {
        m_collapsed.insert(r.agentId);
    }
    rebuild();
}
