#include "sidebar_model.h"

#include "domain/agent_node.h"
#include "persistence/iconversation_store.h"

using domain::AgentNode;
using domain::AgentNodeKind;
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
    emit treeChanged();
}

int SidebarModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

bool SidebarModel::rowIsCurrent(const Row& r) const
{
    if (r.separator || !r.selectable) {
        return false;
    }
    switch (r.type) {
    case NodeType::Node:
        return m_selType == NodeType::Node && r.agentId == m_selAgent;
    case NodeType::Tag:
        return m_selType == NodeType::Tag && r.nodeId == m_selTag;
    default:
        return m_selType == r.type;
    }
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
    case CurrentRole:
        return rowIsCurrent(r);
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
        { CurrentRole, "current" },
    };
}

void SidebarModel::emitCurrentChanged()
{
    if (!m_rows.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_rows.size()) - 1), { CurrentRole });
    }
}

void SidebarModel::setSelectionFromRow(int row)
{
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    if (!r.selectable) {
        return;
    }
    m_selType = r.type;
    m_selTag = r.nodeId;
    m_selAgent = r.agentId;
    emit scopeSelected(static_cast<int>(r.type), r.nodeId, r.agentId);
    emitCurrentChanged();
}

void SidebarModel::activate(int row)
{
    setSelectionFromRow(row);
}

int SidebarModel::currentRow() const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (rowIsCurrent(m_rows.at(i))) {
            return i;
        }
    }
    return -1;
}

int SidebarModel::adjacentSelectableRow(int from, int delta) const
{
    for (int i = from + delta; i >= 0 && i < m_rows.size(); i += delta) {
        const Row& r = m_rows.at(i);
        if (r.selectable && !r.separator) {
            return i;
        }
    }
    return -1;
}

int SidebarModel::parentRow(int row) const
{
    if (row < 0 || row >= m_rows.size() || !m_store) {
        return -1;
    }
    const Row& r = m_rows.at(row);
    if (r.type != NodeType::Node) {
        return -1;
    }
    const QString parentId = m_store->agentNode(r.agentId).parentId;
    if (parentId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row& cand = m_rows.at(i);
        if (cand.type == NodeType::Node && cand.agentId == parentId) {
            return i;
        }
    }
    return -1;
}

bool SidebarModel::selectionInSubtree(const QString& rootId) const
{
    if (m_selType != NodeType::Node || !m_store) {
        return false;
    }
    QString cur = m_selAgent;
    for (int guard = 0; !cur.isEmpty(); ++guard) {
        if (cur == rootId) {
            return true;
        }
        // Bounded walk; the store's tree is finite and acyclic.
        if (guard > 4096) {
            break;
        }
        cur = m_store->agentNode(cur).parentId;
    }
    return false;
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
    const QString id = r.agentId;
    const bool collapsing = isExpanded(id);

    // If we are about to hide the current selection, move it up to this node so
    // a highlighted row stays visible (and the middle pane follows).
    bool moved = false;
    if (collapsing && selectionInSubtree(id)) {
        m_selType = NodeType::Node;
        m_selTag = -1;
        m_selAgent = id;
        moved = true;
    }

    if (collapsing) {
        m_collapsed.insert(id);
    } else {
        m_collapsed.remove(id);
    }
    rebuild();
    if (moved) {
        emit scopeSelected(static_cast<int>(NodeType::Node), -1, id);
    }
}

void SidebarModel::selectNext()
{
    const int cur = currentRow();
    const int next = adjacentSelectableRow(cur < 0 ? -1 : cur, 1);
    if (next >= 0) {
        setSelectionFromRow(next);
    }
}

void SidebarModel::selectPrevious()
{
    const int cur = currentRow();
    const int prev = adjacentSelectableRow(cur < 0 ? static_cast<int>(m_rows.size()) : cur, -1);
    if (prev >= 0) {
        setSelectionFromRow(prev);
    }
}

void SidebarModel::collapseCurrent()
{
    const int cur = currentRow();
    if (cur < 0) {
        return;
    }
    const Row& r = m_rows.at(cur);
    if (r.type == NodeType::Node && r.hasChildren && r.expanded) {
        toggleExpand(cur); // collapse; selection stays on this node
        return;
    }
    const int pr = parentRow(cur);
    if (pr >= 0) {
        setSelectionFromRow(pr);
    }
}

void SidebarModel::expandCurrent()
{
    const int cur = currentRow();
    if (cur < 0) {
        return;
    }
    const Row& r = m_rows.at(cur);
    if (r.type != NodeType::Node || !r.hasChildren) {
        return;
    }
    if (!r.expanded) {
        toggleExpand(cur); // expand in place
        return;
    }
    // Already expanded: descend to the first child (the next row, which is one
    // level deeper).
    const int childRow = cur + 1;
    if (childRow < m_rows.size() && m_rows.at(childRow).depth > r.depth) {
        setSelectionFromRow(childRow);
    }
}

void SidebarModel::activateCurrent()
{
    const int cur = currentRow();
    if (cur >= 0) {
        setSelectionFromRow(cur);
    }
}

void SidebarModel::collectExpandableIds(const QString& parentId, QSet<QString>& out) const
{
    if (!m_store) {
        return;
    }
    for (const AgentNode& n : m_store->agentChildren(parentId)) {
        const QList<AgentNode> kids = m_store->agentChildren(n.id);
        if (!kids.isEmpty()) {
            out.insert(n.id);
        }
        collectExpandableIds(n.id, out);
    }
}

bool SidebarModel::anyExpanded() const
{
    QSet<QString> expandable;
    collectExpandableIds(QString(), expandable);
    for (const QString& id : expandable) {
        if (isExpanded(id)) {
            return true;
        }
    }
    return false;
}

void SidebarModel::expandAll()
{
    m_collapsed.clear();
    rebuild();
}

void SidebarModel::collapseAll()
{
    QSet<QString> expandable;
    collectExpandableIds(QString(), expandable);
    m_collapsed = expandable;

    // If the selection is now hidden (a collapsed node's descendant), lift it to
    // its top-level root ancestor so a row stays highlighted.
    if (m_selType == NodeType::Node && m_store) {
        QString cur = m_selAgent;
        QString root = cur;
        for (int guard = 0; !cur.isEmpty() && guard <= 4096; ++guard) {
            root = cur;
            cur = m_store->agentNode(cur).parentId;
        }
        if (root != m_selAgent) {
            m_selAgent = root;
            emit scopeSelected(static_cast<int>(NodeType::Node), -1, root);
        }
    }
    rebuild();
}

void SidebarModel::createRootNode()
{
    if (!m_store) {
        return;
    }
    // A new top-level node. Kind is cosmetic; default to Orchestrator so it
    // reads as a fresh fleet/workspace. changed() -> rebuild() runs first.
    const QString id = m_store->createNode(QString(), AgentNodeKind::Orchestrator);
    m_selType = NodeType::Node;
    m_selTag = -1;
    m_selAgent = id;
    emit scopeSelected(static_cast<int>(NodeType::Node), -1, id);
    emitCurrentChanged();
}

void SidebarModel::createTag()
{
    if (!m_store) {
        return;
    }
    const int id = m_store->createTag(tr("New tag"), QStringLiteral("#8e9296"));
    m_selType = NodeType::Tag;
    m_selTag = id;
    m_selAgent.clear();
    emit scopeSelected(static_cast<int>(NodeType::Tag), id, {});
    emitCurrentChanged();
}
