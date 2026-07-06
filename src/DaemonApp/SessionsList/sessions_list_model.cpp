// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "sessions_list_model.h"

#include "domain/ids.h"
#include "domain/unit_node.h"
#include "persistence/isession_store.h"

using domain::ListScope;
using domain::NodeType;
using domain::Session;
using domain::UnitId;
using domain::UnitNode;

namespace {

QString snippetOf(const Session& c) {
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

SessionsListModel::SessionsListModel(QObject* parent) : QAbstractListModel(parent) {}

QObject* SessionsListModel::store() const {
    return m_store;
}

void SessionsListModel::setStore(QObject* store) {
    auto* sessionStore = qobject_cast<persistence::ISessionStore*>(store);
    if (m_store == sessionStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = sessionStore;
    if (m_store) {
        connect(m_store, &persistence::ISessionStore::changed, this, &SessionsListModel::reload);
    }
    emit storeChanged();
    reload();
}

void SessionsListModel::setSearch(const QString& search) {
    if (m_search == search) {
        return;
    }
    m_search = search;
    emit searchChanged();
    applyFilter();
}

void SessionsListModel::setScope(int nodeType, int tagId, const QString& unitId) {
    // For the DaemonNet lens scopes (ByTransport/ByPeer) the string slot carries the
    // lens key (transport-instance / peer id) rather than a unit id; everything else
    // treats it as the unit id.
    const auto type = static_cast<NodeType>(nodeType);
    // The lens/agent scopes carry a key (transport-instance / peer / profile id) in the string slot
    // rather than a unit id; everything else treats it as the unit id.
    const bool isLens =
        (type == NodeType::ByTransport || type == NodeType::ByPeer || type == NodeType::Agent);
    m_scope = {type, tagId, isLens ? UnitId() : UnitId(unitId), isLens ? unitId : QString()};
    // A new scope is a fresh list of sessions; drop the old selection so a stale
    // id doesn't linger as a phantom highlight.
    if (!m_currentId.isEmpty()) {
        m_currentId.clear();
        emit selectionChanged(QString());
    }
    // Agent scope (Fleet membership): ask the store for the authoritative per-agent view
    // (SessionScope::ByProfile). The store merges the node's rows into the cache and emits
    // changed(), which re-drives reload() — so the list shows the node-authoritative sessions.
    if (type == NodeType::Agent && m_store != nullptr && !unitId.isEmpty()) {
        m_store->refreshSessionsForProfile(unitId);
    }
    // Archived scope (F6): the TopLevel roster excludes archived rows, so entering the scope
    // fetches the authoritative SessionScope::Archived listing the same way (additive merge,
    // changed() re-drives reload()).
    if (type == NodeType::Archived && m_store != nullptr) {
        m_store->refreshArchivedSessions();
    }
    // ByTransport scope (B4): membership is node-resolved (SessionScope::ByTransport); fetch on
    // entry so the account-scoped list projects the node's answer instead of rendering empty.
    if (type == NodeType::ByTransport && m_store != nullptr && !unitId.isEmpty()) {
        m_store->refreshSessionsForTransport(unitId);
    }
    reload();
    emit scopeChanged();
}

void SessionsListModel::reload() {
    rebuildLookups();
    m_all = m_store ? m_store->sessions(m_scope) : QList<Session>{};
    m_scopeTitle = computeScopeTitle();
    emit scopeChanged();
    applyFilter();
}

void SessionsListModel::rebuildLookups() {
    m_unitInfo.clear();
    m_tagInfo.clear();
    if (!m_store) {
        return;
    }
    // Walk the whole unit tree via the single recursive primitive so each
    // session's owning-unit name/kind can be resolved (any depth).
    persistence::ISessionStore* store = m_store;
    auto collect = [store, this](const UnitId& parentId, auto&& self) -> void {
        for (const UnitNode& n : store->unitChildren(parentId)) {
            m_unitInfo.insert(n.id.toString(), {n.name, static_cast<int>(n.kind)});
            self(n.id, self);
        }
    };
    collect(UnitId(), collect);

    for (const domain::Tag& t : m_store->tags()) {
        m_tagInfo.insert(t.id, {t.name, t.color});
    }
}

void SessionsListModel::applyFilter() {
    beginResetModel();
    m_filtered.clear();
    for (const Session& c : m_all) {
        if (m_search.isEmpty() || c.title.contains(m_search, Qt::CaseInsensitive) ||
            c.content.contains(m_search, Qt::CaseInsensitive)) {
            m_filtered.push_back(c);
        }
    }
    endResetModel();
    emit countChanged();
}

QString SessionsListModel::computeScopeTitle() const {
    switch (m_scope.type) {
    case NodeType::AllSessions:
        return tr("All Sessions");
    case NodeType::Archived:
        return tr("Archived");
    case NodeType::Unit:
        if (m_store) {
            const UnitNode n = m_store->unit(m_scope.unitId);
            if (n.isValid()) {
                return n.name;
            }
        }
        return tr("Agent");
    case NodeType::Tag:
        if (m_store) {
            for (const domain::Tag& t : m_store->tags()) {
                if (t.id == m_scope.tagId) {
                    return t.name;
                }
            }
        }
        return tr("Tag");
    case NodeType::Agent:
        // Fleet membership: an agent == its profile; title the list by the profile id.
        return m_scope.lensKey.isEmpty() ? tr("Agent") : m_scope.lensKey;
    case NodeType::ByTransport:
    case NodeType::ByPeer:
        // The lens scopes (roadmap P2) are not surfaced by the sidebar yet; title by their key.
        return m_scope.lensKey.isEmpty() ? tr("Sessions") : m_scope.lensKey;
    case NodeType::FleetSeparator:
    case NodeType::TagSeparator:
    case NodeType::TransportSeparator:
    case NodeType::Transport:
    case NodeType::FleetNode:
    case NodeType::AgentSession:
        break;
    }
    return tr("Sessions");
}

int SessionsListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_filtered.size());
}

QVariant SessionsListModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_filtered.size()) {
        return {};
    }
    const Session& c = m_filtered.at(index.row());
    switch (role) {
    case IdRole:
        return c.sessionId.toString();
    case TitleRole:
        // Same fallback as the Fleet tree leaf: a fresh SessionCreate-minted session has no
        // title until the first message seeds one node-side — never show a blank row.
        return c.title.isEmpty() ? tr("(untitled)") : c.title;
    case SnippetRole:
        return snippetOf(c);
    case ModifiedRole:
        return c.modified;
    case UnitNameRole:
        return m_unitInfo.value(c.unitId.toString()).first;
    case UnitKindRole:
        return m_unitInfo.value(c.unitId.toString()).second;
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
        return c.sessionId.toString() == m_currentId;
    case PinnedRole:
        return c.isPinned;
    default:
        return {};
    }
}

QHash<int, QByteArray> SessionsListModel::roleNames() const {
    return {
        {IdRole, "sessionId"},      {TitleRole, "title"},         {SnippetRole, "snippet"},
        {ModifiedRole, "modified"}, {UnitNameRole, "unitName"},   {UnitKindRole, "unitKind"},
        {TagNamesRole, "tagNames"}, {TagColorsRole, "tagColors"}, {CurrentRole, "current"},
        {PinnedRole, "pinned"},
    };
}

QString SessionsListModel::idAt(int row) const {
    if (row < 0 || row >= m_filtered.size()) {
        return {};
    }
    return m_filtered.at(row).sessionId.toString();
}

void SessionsListModel::emitCurrentChanged() {
    if (!m_filtered.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_filtered.size()) - 1), {CurrentRole});
    }
}

void SessionsListModel::setCurrentId(const QString& id) {
    if (m_currentId == id) {
        return;
    }
    m_currentId = id;
    emit selectionChanged(id);
    emitCurrentChanged();
}

void SessionsListModel::activate(int row) {
    const QString id = idAt(row);
    if (!id.isEmpty()) {
        setCurrentId(id);
    }
}

void SessionsListModel::selectNext() {
    const int cur = currentRow();
    const int next = cur < 0 ? 0 : cur + 1;
    if (next < m_filtered.size()) {
        setCurrentId(m_filtered.at(next).sessionId.toString());
    }
}

void SessionsListModel::selectPrevious() {
    const int cur = currentRow();
    if (cur > 0) {
        setCurrentId(m_filtered.at(cur - 1).sessionId.toString());
    }
}

int SessionsListModel::currentRow() const {
    if (m_currentId.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < m_filtered.size(); ++i) {
        if (m_filtered.at(i).sessionId.toString() == m_currentId) {
            return i;
        }
    }
    return -1;
}
