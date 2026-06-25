#include "sidebar_model.h"

#include "domain/unit_node.h"
#include "domain/ids.h"
#include "persistence/isession_store.h"

using domain::ListScope;
using domain::NodeType;
using domain::UnitId;
using domain::UnitKind;
using domain::UnitNode;

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
    auto* sessionStore = qobject_cast<persistence::ISessionStore*>(store);
    if (m_store == sessionStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = sessionStore;
    if (m_store) {
        connect(m_store, &persistence::ISessionStore::changed, this, &SidebarModel::rebuild);
    }
    emit storeChanged();
    rebuild();
}

bool SidebarModel::isExpanded(const QString& id) const
{
    return !m_collapsed.contains(id);
}

void SidebarModel::appendUnitRows(const UnitNode& node, int depth)
{
    const QList<UnitNode> children = m_store->unitChildren(node.id);
    const bool expanded = isExpanded(node.id.toString());

    Row r;
    r.label = node.name;
    r.count = m_store->sessionCount({ NodeType::Unit, -1, node.id, {} });
    r.type = NodeType::Unit;
    r.unitId = node.id.toString();
    r.selectable = true;
    r.depth = depth;
    r.hasChildren = !children.isEmpty();
    r.expanded = expanded;
    r.kind = static_cast<int>(node.kind);
    r.state = static_cast<int>(node.state);
    r.profile = node.profile.toString();
    r.session = node.session.toString();
    m_rows.push_back(r);

    // Uniformly recursive: descend into any expanded unit, to any depth. No
    // branching on kind, no level cap.
    if (r.hasChildren && expanded) {
        for (const UnitNode& child : children) {
            appendUnitRows(child, depth + 1);
        }
    }
}

void SidebarModel::rebuild()
{
    beginResetModel();
    m_rows.clear();
    if (m_store) {
        m_rows.push_back({ tr("All Sessions"),
                           m_store->sessionCount({ NodeType::AllSessions, -1, {}, {} }),
                           NodeType::AllSessions, -1, {}, false, true, {}, 0, false, false, 0,
                           0, {}, {} });
        m_rows.push_back({ tr("Archived"),
                           m_store->sessionCount({ NodeType::Archived, -1, {}, {} }),
                           NodeType::Archived, -1, {}, false, true, {}, 0, false, false, 0, 0, {},
                           {} });

        m_rows.push_back({ tr("Fleet"), -1, NodeType::FleetSeparator, -1, {}, true, false, {},
                           0, false, false, 0, 0, {}, {} });
        // Top-level roots have an empty parent; each may be a lone unit or the head
        // of an arbitrarily deep fleet.
        for (const UnitNode& root : m_store->unitChildren(UnitId())) {
            appendUnitRows(root, 0);
        }

        m_rows.push_back({ tr("Tags"), -1, NodeType::TagSeparator, -1, {}, true, false, {},
                           0, false, false, 0, 0, {}, {} });
        for (const domain::Tag& t : m_store->tags()) {
            m_rows.push_back({ t.name, m_store->sessionCount({ NodeType::Tag, t.id, {}, {} }),
                               NodeType::Tag, t.id, {}, false, true, t.color,
                               0, false, false, 0, 0, {}, {} });
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
    case NodeType::Unit:
        return m_selType == NodeType::Unit && r.unitId == m_selUnit;
    case NodeType::Tag:
        return m_selType == NodeType::Tag && r.tagId == m_selTag;
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
    case TagIdRole:
        return r.tagId;
    case UnitIdRole:
        return r.unitId;
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
    case ProfileRole:
        return r.profile;
    case SessionIdRole:
        return r.session;
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
        { TagIdRole, "tagId" },
        { UnitIdRole, "unitId" },
        { IsSeparatorRole, "isSeparator" },
        { SelectableRole, "selectable" },
        { ColorRole, "color" },
        { DepthRole, "depth" },
        { HasChildrenRole, "hasChildren" },
        { ExpandedRole, "expanded" },
        { KindRole, "kind" },
        { StateRole, "state" },
        { CurrentRole, "current" },
        { ProfileRole, "profile" },
        { SessionIdRole, "sessionId" },
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
    m_selTag = r.tagId;
    m_selUnit = r.unitId;
    emit scopeSelected(static_cast<int>(r.type), r.tagId, r.unitId);
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
    if (r.type != NodeType::Unit) {
        return -1;
    }
    const QString parentId = m_store->unit(UnitId(r.unitId)).parentId.toString();
    if (parentId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_rows.size(); ++i) {
        const Row& cand = m_rows.at(i);
        if (cand.type == NodeType::Unit && cand.unitId == parentId) {
            return i;
        }
    }
    return -1;
}

bool SidebarModel::selectionInSubtree(const QString& rootId) const
{
    if (m_selType != NodeType::Unit || !m_store) {
        return false;
    }
    QString cur = m_selUnit;
    for (int guard = 0; !cur.isEmpty(); ++guard) {
        if (cur == rootId) {
            return true;
        }
        // Bounded walk; the store's tree is finite and acyclic.
        if (guard > 4096) {
            break;
        }
        cur = m_store->unit(UnitId(cur)).parentId.toString();
    }
    return false;
}

void SidebarModel::toggleExpand(int row)
{
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    if (r.type != NodeType::Unit || !r.hasChildren) {
        return;
    }
    const QString id = r.unitId;
    const bool collapsing = isExpanded(id);

    // If we are about to hide the current selection, move it up to this unit so a
    // highlighted row stays visible (and the middle pane follows).
    bool moved = false;
    if (collapsing && selectionInSubtree(id)) {
        m_selType = NodeType::Unit;
        m_selTag = -1;
        m_selUnit = id;
        moved = true;
    }

    if (collapsing) {
        m_collapsed.insert(id);
    } else {
        m_collapsed.remove(id);
    }
    rebuild();
    if (moved) {
        emit scopeSelected(static_cast<int>(NodeType::Unit), -1, id);
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
    if (r.type == NodeType::Unit && r.hasChildren && r.expanded) {
        toggleExpand(cur); // collapse; selection stays on this unit
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
    if (r.type != NodeType::Unit || !r.hasChildren) {
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
    for (const UnitNode& n : m_store->unitChildren(UnitId(parentId))) {
        const QList<UnitNode> kids = m_store->unitChildren(n.id);
        if (!kids.isEmpty()) {
            out.insert(n.id.toString());
        }
        collectExpandableIds(n.id.toString(), out);
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

    // If the selection is now hidden (a collapsed unit's descendant), lift it to
    // its top-level root ancestor so a row stays highlighted.
    if (m_selType == NodeType::Unit && m_store) {
        QString cur = m_selUnit;
        QString root = cur;
        for (int guard = 0; !cur.isEmpty() && guard <= 4096; ++guard) {
            root = cur;
            cur = m_store->unit(UnitId(cur)).parentId.toString();
        }
        if (root != m_selUnit) {
            m_selUnit = root;
            emit scopeSelected(static_cast<int>(NodeType::Unit), -1, root);
        }
    }
    rebuild();
}

void SidebarModel::createRootUnit()
{
    if (!m_store) {
        return;
    }
    // A new top-level unit. Kind is cosmetic; default to Orchestrator so it reads
    // as a fresh fleet/workspace. changed() -> rebuild() runs first.
    const QString id = m_store->createUnit(UnitId(), UnitKind::Orchestrator).toString();
    m_selType = NodeType::Unit;
    m_selTag = -1;
    m_selUnit = id;
    emit scopeSelected(static_cast<int>(NodeType::Unit), -1, id);
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
    m_selUnit.clear();
    emit scopeSelected(static_cast<int>(NodeType::Tag), id, {});
    emitCurrentChanged();
}
