// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "sidebar_model.h"

#include "daemonnet/idaemonnet.h"
#include "domain/ids.h"
#include "domain/session.h"
#include "domain/unit_node.h"
#include "persistence/isession_store.h"
#include "profiles/iprofile_store.h"
#include "uimodels/variant_list_model.h"

#include <algorithm>
#include <QDateTime>
#include <QFile>
#include <QHash>

using daemonnet::TransportTreeRow;
using domain::ListScope;
using domain::NodeType;
using domain::UnitId;
using domain::UnitNode;

namespace {
// Fleet membership expand-state keys (a disjoint id namespace from unit/transport ids, so they
// share m_collapsed safely). Prefixed with a control char no real id carries.
QString nodeRootKey() {
    return QStringLiteral("\x01node-root");
}
QString agentExpandKey(const QString& profileId) {
    return QStringLiteral("\x01agent:") + profileId;
}
} // namespace

SidebarModel::SidebarModel(QObject* parent) : QAbstractListModel(parent) {}

QObject* SidebarModel::store() const {
    return m_store;
}

void SidebarModel::setStore(QObject* store) {
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

QObject* SidebarModel::daemonNet() const {
    return m_net;
}

void SidebarModel::setDaemonNet(QObject* net) {
    auto* dn = qobject_cast<daemonnet::IDaemonNet*>(net);
    if (m_net == dn) {
        return;
    }
    if (m_net) {
        m_net->disconnect(this);
    }
    m_net = dn;
    if (m_net) {
        connect(m_net, &daemonnet::IDaemonNet::changed, this, &SidebarModel::rebuild);
    }
    emit daemonNetChanged();
    rebuild();
}

QObject* SidebarModel::profiles() const {
    return m_profiles;
}

void SidebarModel::setProfiles(QObject* profiles) {
    auto* store = qobject_cast<profiles::IProfileStore*>(profiles);
    if (m_profiles == store) {
        return;
    }
    if (m_profiles) {
        m_profiles->disconnect(this);
    }
    m_profiles = store;
    if (m_profiles) {
        // The profile list changed (create/update/delete/select/re-list): the FLEET agents change,
        // so rebuild. This is the single-client post-mutation re-list (A2) — no node event needed.
        connect(m_profiles, &profiles::IProfileStore::changed, this, &SidebarModel::rebuild);
    }
    emit profilesChanged();
    rebuild();
}

QString SidebarModel::nodeLabel() const {
    return m_nodeLabel;
}

void SidebarModel::setNodeLabel(const QString& label) {
    if (m_nodeLabel == label) {
        return;
    }
    m_nodeLabel = label;
    emit nodeLabelChanged();
    rebuild();
}

QList<QVariantMap> SidebarModel::agentRows() const {
    if (m_profiles == nullptr) {
        return {};
    }
    // Agents == profiles (§0). The profile store publishes them as a VariantListModel
    // (rows: id, name, model, provider, isDefault); project those rows directly.
    auto* model = qobject_cast<uimodels::VariantListModel*>(m_profiles->profiles());
    return model != nullptr ? model->rows() : QList<QVariantMap>{};
}

bool SidebarModel::isExpanded(const QString& id) const {
    return !m_collapsed.contains(id);
}

bool SidebarModel::isSectionExpanded(const QString& sectionKey) const {
    return !m_sectionCollapsed.contains(sectionKey);
}

void SidebarModel::pushSectionHeader(const QString& label, NodeType type,
                                     const QString& sectionKey) {
    // A collapsible section group: not selectable (it is a header), but carries
    // hasChildren + expanded so the GUI twistie and the TUI disclosure plumbing
    // (clickAt / tree_list_view) fold the whole section under it.
    Row r;
    r.label = label;
    r.type = type;
    r.separator = true;
    r.selectable = false;
    r.hasChildren = true;
    r.expanded = isSectionExpanded(sectionKey);
    r.sectionKey = sectionKey;
    m_rows.push_back(r);
}

void SidebarModel::appendFleetMembership() {
    // The client-side node/connection ROOT (§0): represents this client's ffi/socket link to the
    // (local) node — NOT an in-tree unit. Remote/daisy-chained nodes would be additional roots
    // later; nothing here precludes that (we just render one root today).
    const QString rootLabel = m_nodeLabel.isEmpty() ? tr("Local node") : m_nodeLabel;
    const QList<QVariantMap> agents = agentRows();

    Row root;
    root.label = rootLabel;
    root.type = NodeType::FleetNode;
    root.selectable = true;
    root.depth = 0;
    root.expandKey = nodeRootKey();
    root.expanded = isExpanded(root.expandKey);
    root.hasChildren = !agents.isEmpty();
    root.count = static_cast<int>(agents.size());
    m_rows.push_back(root);

    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(
                QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"FLEET-ROOT\","
                               "\"location\":\"sidebar_model.cpp:appendFleetMembership\","
                               "\"message\":\"node root row built\",\"data\":{\"label\":\"%1\","
                               "\"agents\":%2},\"timestamp\":%3}\n")
                    .arg(rootLabel)
                    .arg(agents.size())
                    .arg(QDateTime::currentMSecsSinceEpoch())
                    .toUtf8());
    }
    // #endregion

    if (!root.expanded) {
        return; // node root collapsed: agents hidden
    }

    int agentCount = 0;
    for (const QVariantMap& a : agents) {
        const QString id = a.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) {
            continue;
        }
        const QString name = a.value(QStringLiteral("name")).toString();
        // An agent's sessions come from the store filtered by SessionScope::ByProfile (client-side
        // over the cached roster; the authoritative wire query is issued on selection, A3).
        const QList<domain::Session> sess =
            m_store != nullptr ? m_store->sessions({NodeType::Agent, -1, UnitId(), id})
                               : QList<domain::Session>{};
        Row agent;
        agent.label = name.isEmpty() ? id : name;
        agent.type = NodeType::Agent;
        agent.selectable = true;
        agent.depth = 1;
        agent.profile = id;
        agent.expandKey = agentExpandKey(id);
        agent.expanded = isExpanded(agent.expandKey);
        agent.hasChildren = !sess.isEmpty();
        agent.count = static_cast<int>(sess.size());
        m_rows.push_back(agent);
        ++agentCount;

        if (!agent.expanded) {
            continue;
        }
        for (const domain::Session& s : sess) {
            Row leaf;
            leaf.label = s.title.isEmpty() ? tr("(untitled)") : s.title;
            leaf.type = NodeType::AgentSession;
            leaf.selectable = true;
            leaf.depth = 2;
            leaf.profile = id;
            leaf.session = s.sessionId.toString();
            m_rows.push_back(leaf);
        }
    }

    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"AGENT-LIST\","
                                     "\"location\":\"sidebar_model.cpp:appendFleetMembership\","
                                     "\"message\":\"agent rows built from ProfileList\",\"data\":{"
                                     "\"agents\":%1},\"timestamp\":%2}\n")
                          .arg(agentCount)
                          .arg(QDateTime::currentMSecsSinceEpoch())
                          .toUtf8());
    }
    // #endregion
}

void SidebarModel::collectFleetExpandKeys(QSet<QString>& out) const {
    const QList<QVariantMap> agents = agentRows();
    if (agents.isEmpty()) {
        return;
    }
    out.insert(nodeRootKey());
    for (const QVariantMap& a : agents) {
        const QString id = a.value(QStringLiteral("id")).toString();
        if (id.isEmpty()) {
            continue;
        }
        // Only agents that actually have sessions are foldable.
        const int n =
            m_store != nullptr ? m_store->sessionCount({NodeType::Agent, -1, UnitId(), id}) : 0;
        if (n > 0) {
            out.insert(agentExpandKey(id));
        }
    }
}

void SidebarModel::rebuild() {
    beginResetModel();
    m_rows.clear();
    if (m_store) {
        m_rows.push_back({tr("All Sessions"),
                          m_store->sessionCount({NodeType::AllSessions, -1, {}, {}}),
                          NodeType::AllSessions,
                          -1,
                          {},
                          false,
                          true,
                          {},
                          0,
                          false,
                          false,
                          0,
                          0,
                          {},
                          {},
                          {},
                          {},
                          {},
                          {},
                          {},
                          {},
                          -1,
                          {}});
        m_rows.push_back({tr("Archived"),
                          m_store->sessionCount({NodeType::Archived, -1, {}, {}}),
                          NodeType::Archived,
                          -1,
                          {},
                          false,
                          true,
                          {},
                          0,
                          false,
                          false,
                          0,
                          0,
                          {},
                          {},
                          {},
                          {},
                          {},
                          {},
                          {},
                          {},
                          -1,
                          {}});

        // Tags section sits above Fleet (cross-cutting labels before the org chart).
        pushSectionHeader(tr("Tags"), NodeType::TagSeparator, QStringLiteral("tags"));
        if (isSectionExpanded(QStringLiteral("tags"))) {
            for (const domain::Tag& t : m_store->tags()) {
                m_rows.push_back({t.name,
                                  m_store->sessionCount({NodeType::Tag, t.id, {}, {}}),
                                  NodeType::Tag,
                                  t.id,
                                  {},
                                  false,
                                  true,
                                  t.color,
                                  0,
                                  false,
                                  false,
                                  0,
                                  0,
                                  {},
                                  {},
                                  {},
                                  {},
                                  {},
                                  {},
                                  {},
                                  {},
                                  -1,
                                  {}});
            }
        }

        pushSectionHeader(tr("Fleet"), NodeType::FleetSeparator, QStringLiteral("fleet"));
        if (isSectionExpanded(QStringLiteral("fleet"))) {
            // §0 membership view: node(client connection root) -> agents(profiles) -> sessions
            // (ByProfile). NOT the node's in-partition tree()/unitChildren() (that feeds the
            // in-transcript SubagentModel drill-down, left untouched).
            appendFleetMembership();
        }
    }
    // The co-equal events-IO axis: an "Integrations" header + the capability-driven
    // transport tree (account -> taxonomy -> session leaf), sourced from DaemonNet.
    appendTransportRows();
    endResetModel();
    emit treeChanged();
}

void SidebarModel::appendTransportRows() {
    if (!m_net) {
        return;
    }
    const QList<TransportTreeRow> tree = m_net->transportsTree();
    if (tree.isEmpty()) {
        return;
    }

    // The "Integrations" section header (the user-facing name for the events-IO
    // transport-adapter tree). Collapsible like Fleet/Tags.
    pushSectionHeader(tr("Integrations"), NodeType::TransportSeparator,
                      QStringLiteral("integrations"));
    if (!isSectionExpanded(QStringLiteral("integrations"))) {
        return; // section folded: header only, no body
    }

    // Parent-chain map so a collapsed account/group hides its whole subtree.
    QHash<QString, QString> parentOf;
    for (const TransportTreeRow& t : tree) {
        parentOf.insert(t.id, t.parentId);
    }
    const auto hiddenByCollapse = [&](const QString& id) {
        QString cur = parentOf.value(id);
        for (int guard = 0; !cur.isEmpty() && guard <= 4096; ++guard) {
            if (!isExpanded(cur)) {
                return true;
            }
            cur = parentOf.value(cur);
        }
        return false;
    };

    for (const TransportTreeRow& t : tree) {
        if (hiddenByCollapse(t.id)) {
            continue;
        }
        Row r;
        r.label = t.label;
        r.type = NodeType::Transport;
        r.selectable = true;
        r.depth = t.depth;
        r.hasChildren = t.hasChildren;
        r.expanded = isExpanded(t.id);
        r.count = t.memberCount > 0 ? t.memberCount : -1;
        r.session = t.sessionId;
        r.txNode = t.id;
        r.txKind = t.kind;
        r.convType = t.convType;
        r.sublabel = t.sublabel;
        r.presence = t.presence;
        r.scopeKey = t.scopeKey;
        // When a row has no session leaf, selecting it scopes the list: an account
        // groups all sessions over its transport (ByTransport); a 1:1 Dm peer groups
        // by that peer (ByPeer). Channels / convGroups carry no list scope.
        if (t.kind == QStringLiteral("account")) {
            r.scopeType = static_cast<int>(NodeType::ByTransport);
        } else if (t.convType == QStringLiteral("dm")) {
            r.scopeType = static_cast<int>(NodeType::ByPeer);
        }
        m_rows.push_back(r);
    }
}

int SidebarModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

bool SidebarModel::rowIsCurrent(const Row& r) const {
    if (r.separator || !r.selectable) {
        return false;
    }
    switch (r.type) {
    case NodeType::Unit:
        return m_selType == NodeType::Unit && r.unitId == m_selUnit;
    case NodeType::Tag:
        return m_selType == NodeType::Tag && r.tagId == m_selTag;
    case NodeType::Transport:
        return m_selType == NodeType::Transport && r.txNode == m_selTxNode;
    case NodeType::FleetNode:
        return m_selType == NodeType::FleetNode;
    case NodeType::Agent:
        return m_selType == NodeType::Agent && r.profile == m_selAgent;
    case NodeType::AgentSession:
        return m_selType == NodeType::AgentSession && r.session == m_selSession;
    default:
        return m_selType == r.type;
    }
}

QVariant SidebarModel::data(const QModelIndex& index, int role) const {
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
    case TxKindRole:
        return r.txKind;
    case ConvTypeRole:
        return r.convType;
    case SubLabelRole:
        return r.sublabel;
    case PresenceRole:
        return r.presence;
    default:
        return {};
    }
}

QHash<int, QByteArray> SidebarModel::roleNames() const {
    return {
        {LabelRole, "label"},           {CountRole, "count"},
        {NodeTypeRole, "nodeType"},     {TagIdRole, "tagId"},
        {UnitIdRole, "unitId"},         {IsSeparatorRole, "isSeparator"},
        {SelectableRole, "selectable"}, {ColorRole, "color"},
        {DepthRole, "depth"},           {HasChildrenRole, "hasChildren"},
        {ExpandedRole, "expanded"},     {KindRole, "kind"},
        {StateRole, "state"},           {CurrentRole, "current"},
        {ProfileRole, "profile"},       {SessionIdRole, "sessionId"},
        {TxKindRole, "txKind"},         {ConvTypeRole, "convType"},
        {SubLabelRole, "subLabel"},     {PresenceRole, "presence"},
    };
}

void SidebarModel::emitCurrentChanged() {
    if (!m_rows.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_rows.size()) - 1), {CurrentRole});
    }
}

void SidebarModel::setSelectionFromRow(int row) {
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
    m_selTxNode = r.txNode;
    m_selAgent = (r.type == NodeType::Agent) ? r.profile : QString();
    m_selSession = (r.type == NodeType::AgentSession) ? r.session : QString();
    if (r.type == NodeType::Transport) {
        // A session leaf opens its transcript directly; a scope-bearing group
        // (account / Dm peer) regroups the list; everything else just highlights.
        if (!r.session.isEmpty()) {
            emit sessionActivated(r.session);
        } else if (r.scopeType >= 0 && !r.scopeKey.isEmpty()) {
            emit scopeSelected(r.scopeType, -1, r.scopeKey);
        }
    } else if (r.type == NodeType::AgentSession) {
        // A session leaf under an agent opens its transcript (like an Integrations session leaf).
        if (!r.session.isEmpty()) {
            emit sessionActivated(r.session);
        }
    } else if (r.type == NodeType::Agent) {
        // Selecting an agent scopes the session list to SessionScope::ByProfile(profileId): the
        // per-agent view (A3). `profile` rides the string slot as the lens/profile key.
        emit scopeSelected(static_cast<int>(NodeType::Agent), -1, r.profile);
    } else if (r.type == NodeType::FleetNode) {
        // The node root scopes to all of this node's sessions (its whole membership).
        emit scopeSelected(static_cast<int>(NodeType::AllSessions), -1, QString());
    } else {
        emit scopeSelected(static_cast<int>(r.type), r.tagId, r.unitId);
    }
    emitCurrentChanged();
}

void SidebarModel::activate(int row) {
    setSelectionFromRow(row);
}

int SidebarModel::currentRow() const {
    for (int i = 0; i < m_rows.size(); ++i) {
        if (rowIsCurrent(m_rows.at(i))) {
            return i;
        }
    }
    return -1;
}

int SidebarModel::adjacentSelectableRow(int from, int delta) const {
    for (int i = from + delta; i >= 0 && i < m_rows.size(); i += delta) {
        const Row& r = m_rows.at(i);
        if (r.selectable && !r.separator) {
            return i;
        }
    }
    return -1;
}

int SidebarModel::parentRow(int row) const {
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

bool SidebarModel::selectionInSubtree(const QString& rootId) const {
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

void SidebarModel::toggleExpand(int row) {
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    // Section header rows (Fleet/Tags/Integrations) fold their whole section.
    if (r.separator && !r.sectionKey.isEmpty()) {
        if (isSectionExpanded(r.sectionKey)) {
            m_sectionCollapsed.insert(r.sectionKey);
        } else {
            m_sectionCollapsed.remove(r.sectionKey);
        }
        rebuild();
        return;
    }
    // Fleet membership rows (node root / agent) fold by their expand key (a disjoint id namespace).
    if ((r.type == NodeType::FleetNode || r.type == NodeType::Agent) && r.hasChildren &&
        !r.expandKey.isEmpty()) {
        if (isExpanded(r.expandKey)) {
            m_collapsed.insert(r.expandKey);
        } else {
            m_collapsed.remove(r.expandKey);
        }
        rebuild();
        return;
    }
    if ((r.type != NodeType::Unit && r.type != NodeType::Transport) || !r.hasChildren) {
        return;
    }
    // Transport group rows toggle by their tree-node id (a disjoint id namespace
    // from unit ids, so they share m_collapsed safely).
    if (r.type == NodeType::Transport) {
        const QString id = r.txNode;
        if (isExpanded(id)) {
            m_collapsed.insert(id);
        } else {
            m_collapsed.remove(id);
        }
        rebuild();
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

void SidebarModel::selectNext() {
    const int cur = currentRow();
    const int next = adjacentSelectableRow(cur < 0 ? -1 : cur, 1);
    if (next >= 0) {
        setSelectionFromRow(next);
    }
}

void SidebarModel::selectPrevious() {
    const int cur = currentRow();
    const int prev = adjacentSelectableRow(cur < 0 ? static_cast<int>(m_rows.size()) : cur, -1);
    if (prev >= 0) {
        setSelectionFromRow(prev);
    }
}

void SidebarModel::collapseCurrent() {
    const int cur = currentRow();
    if (cur < 0) {
        return;
    }
    const Row& r = m_rows.at(cur);
    if ((r.type == NodeType::Unit || r.type == NodeType::Transport ||
         r.type == NodeType::FleetNode || r.type == NodeType::Agent) &&
        r.hasChildren && r.expanded) {
        toggleExpand(cur); // collapse; selection stays on this row
        return;
    }
    const int pr = parentRow(cur);
    if (pr >= 0) {
        setSelectionFromRow(pr);
    }
}

void SidebarModel::expandCurrent() {
    const int cur = currentRow();
    if (cur < 0) {
        return;
    }
    const Row& r = m_rows.at(cur);
    if ((r.type != NodeType::Unit && r.type != NodeType::Transport &&
         r.type != NodeType::FleetNode && r.type != NodeType::Agent) ||
        !r.hasChildren) {
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

void SidebarModel::activateCurrent() {
    const int cur = currentRow();
    if (cur >= 0) {
        setSelectionFromRow(cur);
    }
}

bool SidebarModel::anyExpanded() const {
    // The Fleet header's expand-all/collapse-all now folds the membership rows (node root +
    // agents), not the node's unit tree.
    QSet<QString> expandable;
    collectFleetExpandKeys(expandable);
    return std::ranges::any_of(expandable, [this](const QString& id) { return isExpanded(id); });
}

void SidebarModel::collectTransportExpandableIds(QSet<QString>& out) const {
    if (!m_net) {
        return;
    }
    // The Integrations-tree nodes that can fold (accounts + conversation groups).
    for (const TransportTreeRow& t : m_net->transportsTree()) {
        if (t.hasChildren) {
            out.insert(t.id);
        }
    }
}

bool SidebarModel::anyTransportExpanded() const {
    QSet<QString> expandable;
    collectTransportExpandableIds(expandable);
    return std::ranges::any_of(expandable, [this](const QString& id) { return isExpanded(id); });
}

void SidebarModel::expandAllTransports() {
    QSet<QString> expandable;
    collectTransportExpandableIds(expandable);
    for (const QString& id : expandable) {
        m_collapsed.remove(id);
    }
    rebuild();
}

void SidebarModel::collapseAllTransports() {
    QSet<QString> expandable;
    collectTransportExpandableIds(expandable);
    for (const QString& id : expandable) {
        m_collapsed.insert(id);
    }
    rebuild();
}

void SidebarModel::expandAll() {
    // Fleet membership-scoped: un-collapse the node root + agent keys, leaving any collapsed
    // transport nodes (a disjoint id namespace sharing m_collapsed) untouched.
    QSet<QString> expandable;
    collectFleetExpandKeys(expandable);
    for (const QString& id : expandable) {
        m_collapsed.remove(id);
    }
    rebuild();
}

void SidebarModel::collapseAll() {
    QSet<QString> expandable;
    collectFleetExpandKeys(expandable);
    // Union (not replace) so transport-node collapse state survives.
    for (const QString& id : expandable) {
        m_collapsed.insert(id);
    }
    rebuild();
}

void SidebarModel::createRootUnit() {
    // A2: the Fleet "+" mints a new AGENT (a profile), not a bare chat. The shell opens a small
    // agent form (provider/model/optional key) and calls ProfileRepository::createProfile(spec);
    // on success the post-mutation profile re-list makes the agent appear under the node root (no
    // node event needed — single client). A separate affordance opens a session on an agent.
    emit createAgentRequested();
}

void SidebarModel::createTag() {
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
