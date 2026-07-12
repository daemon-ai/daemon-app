// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "integrations_tree_model.h"

#include "mirror/generated/entities_gen.h"
#include "mirror/mirror_service.h"
#include "mirror/store.h"
#include "transports/itransport_registry.h"

#include <algorithm>

using transports::ITransportRegistry;

namespace {
constexpr auto kConsumer = "integrations_tree";

// Fold/selection id namespaces (control-char prefixes so a real transport/conversation id can never
// collide across namespaces sharing m_collapsed).
QString acctKey(const QString& transport) {
    return QStringLiteral("\x01"
                          "a:") +
           transport;
}
QString convKey(const QString& transport, const QString& conv) {
    return QStringLiteral("\x01"
                          "c:") +
           transport + QLatin1Char('\x1f') + conv;
}
QString secKey(const QString& transport, const QString& section) {
    return QStringLiteral("\x01"
                          "s:") +
           transport + QLatin1Char('\x1f') + section;
}

bool isChannelKind(const QString& kind) {
    return kind == QStringLiteral("channel") || kind == QStringLiteral("thread");
}
// Mirror vocabulary (map_conversation): "dm" | "group_dm".
bool isDmKind(const QString& kind) {
    return kind == QStringLiteral("dm") || kind == QStringLiteral("group_dm");
}
} // namespace

IntegrationsTreeModel::IntegrationsTreeModel(QObject* parent) : QAbstractListModel(parent) {}

QObject* IntegrationsTreeModel::mirror() const {
    return m_mirrorObject;
}

void IntegrationsTreeModel::setMirror(QObject* mirror) {
    if (m_mirrorObject == mirror) {
        return;
    }
    if (m_mirror != nullptr) {
        m_mirror->disconnect(this);
    }
    m_mirrorObject = mirror;
    m_mirror = nullptr;
    if (auto* svc = qobject_cast<mirror::MirrorService*>(mirror)) {
        m_mirror = &svc->store();
        m_mirror->journal().registerConsumer(QLatin1String(kConsumer));
        m_watermark = m_mirror->journal().headRev();
        m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
        connect(m_mirror, &mirror::Store::committed, this, &IntegrationsTreeModel::onCommitted);
        // Visibility declaration (§5.8): the sidebar tree is a standing surface — observing the
        // coarse collections keeps them fresh via the ingestor's staleness policy.
        svc->ingestor().beginObserving(QStringLiteral("transports"));
        svc->ingestor().beginObserving(QStringLiteral("adapters"));
        svc->ingestor().beginObserving(QStringLiteral("persons"));
        svc->ingestor().beginObserving(QStringLiteral("conversations"));
    }
    emit mirrorChanged();
    rebuild();
}

void IntegrationsTreeModel::onCommitted() {
    // Journal-filtered re-derivation (§8.1): rebuild only when a tree-relevant kind changed.
    const auto deltas = m_mirror->journal().since(m_watermark);
    m_watermark = m_mirror->journal().headRev();
    m_mirror->journal().advanceWatermark(QLatin1String(kConsumer), m_watermark);
    for (const auto& r : deltas) {
        switch (r.kind) {
        case mirror::EntityKind::Adapter:
        case mirror::EntityKind::TransportAccount:
        case mirror::EntityKind::Conversation:
        case mirror::EntityKind::Person:
        case mirror::EntityKind::PersonEndpoint:
            rebuild();
            return;
        default:
            break;
        }
    }
}

QObject* IntegrationsTreeModel::registry() const {
    return m_registry;
}

void IntegrationsTreeModel::setRegistry(QObject* registry) {
    auto* reg = qobject_cast<ITransportRegistry*>(registry);
    if (m_registry == reg) {
        return;
    }
    // AD (1a.3): the registry is a pure VERB sink now — no read signals to (dis)connect.
    m_registry = reg;
    emit registryChanged();
}

QList<QVariantMap> IntegrationsTreeModel::accountRows() const {
    QList<QVariantMap> out;
    if (m_mirror == nullptr) {
        return out;
    }
    for (const mirror::TransportAccount& t : m_mirror->snapshot().transport_accounts) {
        QVariantMap row;
        row.insert(QStringLiteral("transport"), t.transport);
        row.insert(QStringLiteral("family"), t.family);
        // wire v35: the node-persisted label overlays the display name.
        row.insert(QStringLiteral("displayName"), t.label.isEmpty() ? t.display_name : t.label);
        row.insert(QStringLiteral("enabled"), t.enabled);
        row.insert(QStringLiteral("connectionReason"), t.reason);
        out.append(row);
    }
    std::ranges::sort(out, [](const QVariantMap& a, const QVariantMap& b) {
        return a.value(QStringLiteral("transport")).toString() <
               b.value(QStringLiteral("transport")).toString();
    });
    return out;
}

QVariantMap IntegrationsTreeModel::adapterFor(const QString& family) const {
    if (m_mirror == nullptr) {
        return {};
    }
    const mirror::Adapter* a = m_mirror->snapshot().adapters.find(mirror::AdapterKey{family});
    if (a == nullptr) {
        return {};
    }
    QVariantMap row;
    row.insert(QStringLiteral("family"), a->family);
    row.insert(QStringLiteral("displayName"), a->display_name);
    row.insert(QStringLiteral("capabilities"),
               QVariantMap{{QStringLiteral("rooms"), a->cap_rooms},
                           {QStringLiteral("directMessages"), a->cap_direct_messages},
                           {QStringLiteral("fileTransfer"), a->cap_file_transfer}});
    row.insert(QStringLiteral("directory"), a->directory);
    return row;
}

QList<QVariantMap> IntegrationsTreeModel::conversationRows(const QString& transport) const {
    QList<QVariantMap> out;
    if (m_mirror == nullptr) {
        return out;
    }
    for (const mirror::Conversation& c : m_mirror->snapshot().conversations) {
        if (c.transport != transport) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), c.id);
        row.insert(QStringLiteral("kind"), c.kind);
        row.insert(QStringLiteral("title"), c.title);
        row.insert(QStringLiteral("parent"), c.parent);
        out.append(row);
    }
    std::ranges::sort(out, [](const QVariantMap& a, const QVariantMap& b) {
        return a.value(QStringLiteral("id")).toString() < b.value(QStringLiteral("id")).toString();
    });
    return out;
}

QList<QVariantMap> IntegrationsTreeModel::personRows(const QString& transport) const {
    QList<QVariantMap> out;
    if (m_mirror == nullptr) {
        return out;
    }
    // persons × person_endpoints join on this transport: the alias wins as the label, else the
    // endpoint's display name; presence is the endpoint contact-info's primitive.
    for (const mirror::PersonEndpoint& e : m_mirror->snapshot().person_endpoints) {
        if (e.transport != transport) {
            continue;
        }
        QString label = e.display_name;
        if (const mirror::Person* p =
                m_mirror->snapshot().persons.find(mirror::PersonKey{e.person})) {
            if (!p->alias.isEmpty()) {
                label = p->alias;
            }
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), e.person);
        row.insert(QStringLiteral("label"), label);
        row.insert(QStringLiteral("presence"), e.presence_primitive);
        out.append(row);
    }
    std::ranges::sort(out, [](const QVariantMap& a, const QVariantMap& b) {
        return a.value(QStringLiteral("id")).toString() < b.value(QStringLiteral("id")).toString();
    });
    // One row per person (a person with several endpoints on one transport renders once).
    QSet<QString> seen;
    QList<QVariantMap> unique;
    for (const QVariantMap& row : out) {
        const QString id = row.value(QStringLiteral("id")).toString();
        if (!seen.contains(id)) {
            seen.insert(id);
            unique.append(row);
        }
    }
    return unique;
}

bool IntegrationsTreeModel::isExpanded(const QString& foldKey) const {
    return !m_collapsed.contains(foldKey);
}

void IntegrationsTreeModel::rebuild() {
    beginResetModel();
    m_rows.clear();
    for (const QVariantMap& inst : accountRows()) {
        appendAccount(inst, adapterFor(inst.value(QStringLiteral("family")).toString()));
    }
    endResetModel();
    emit treeChanged();
}

void IntegrationsTreeModel::appendAccount(const QVariantMap& instance, const QVariantMap& adapter) {
    const QString transport = instance.value(QStringLiteral("transport")).toString();
    const QString family = instance.value(QStringLiteral("family")).toString();
    const QVariantMap caps = adapter.value(QStringLiteral("capabilities")).toMap();
    const bool dmCap = caps.value(QStringLiteral("directMessages")).toBool();
    const bool directory = adapter.value(QStringLiteral("directory")).toBool();

    // Partition this account's conversations by the node's kind/parent (the protocol-governed
    // hierarchy). A parent that is empty, unknown, or self-referential makes the row a ROOT.
    const QList<QVariantMap> convs = conversationRows(transport);
    QSet<QString> knownIds;
    for (const QVariantMap& c : convs) {
        knownIds.insert(c.value(QStringLiteral("id")).toString());
    }
    const auto isRoot = [&](const QVariantMap& c) {
        const QString id = c.value(QStringLiteral("id")).toString();
        const QString parent = c.value(QStringLiteral("parent")).toString();
        return parent.isEmpty() || parent == id || !knownIds.contains(parent);
    };
    QHash<QString, QList<QVariantMap>> byParent;
    QList<QVariantMap> rootSpaces;
    QList<QVariantMap> rootChannels;
    QList<QVariantMap> dms;
    for (const QVariantMap& c : convs) {
        const QString kind = c.value(QStringLiteral("kind")).toString();
        const bool root = isRoot(c);
        if (!root) {
            byParent[c.value(QStringLiteral("parent")).toString()].append(c);
        }
        if (kind == QStringLiteral("space")) {
            if (root) {
                rootSpaces.append(c);
            }
        } else if (isDmKind(kind)) {
            dms.append(c);
        } else if (isChannelKind(kind) && root) {
            rootChannels.append(c);
        }
    }

    const QList<QVariantMap> people = personRows(transport);
    const bool hasPersons = !people.isEmpty();
    const bool showDms = dmCap && !dms.isEmpty();
    const bool showChannels = !rootChannels.isEmpty();
    const bool hasChildren =
        hasPersons || !rootSpaces.isEmpty() || showChannels || showDms || directory;

    Row acct;
    acct.label = instance.value(QStringLiteral("displayName")).toString();
    acct.depth = 0;
    acct.rowKind = QStringLiteral("account");
    acct.transport = transport;
    acct.family = family;
    acct.enabled = instance.value(QStringLiteral("enabled"), true).toBool();
    acct.connection = instance.value(QStringLiteral("connectionReason")).toString();
    acct.hasChildren = hasChildren;
    acct.foldKey = acctKey(transport);
    acct.selId = QStringLiteral("acct:") + transport;
    acct.expanded = isExpanded(acct.foldKey);
    m_rows.push_back(acct);

    if (!hasChildren || !acct.expanded) {
        return;
    }

    // Persons — the cross-transport identities reachable on this account.
    if (hasPersons) {
        pushSection(transport, QStringLiteral("persons"), tr("Persons"), 1);
        if (isExpanded(secKey(transport, QStringLiteral("persons")))) {
            for (const QVariantMap& person : people) {
                Row r;
                r.label = person.value(QStringLiteral("label")).toString();
                r.depth = 2;
                r.rowKind = QStringLiteral("person");
                r.transport = transport;
                r.personId = person.value(QStringLiteral("id")).toString();
                r.presence = person.value(QStringLiteral("presence")).toString();
                r.selId = QStringLiteral("person:") + r.personId;
                m_rows.push_back(r);
            }
        }
    }

    // Servers / Spaces — each nests its member conversations (recursively).
    for (const QVariantMap& space : rootSpaces) {
        QSet<QString> visited;
        appendSpace(transport, space, byParent, visited, 1);
    }

    // Standalone rooms / channels (incl. unknown-parent rows treated as roots).
    if (showChannels) {
        pushSection(transport, QStringLiteral("channels"), tr("Channels"), 1);
        if (isExpanded(secKey(transport, QStringLiteral("channels")))) {
            for (const QVariantMap& c : rootChannels) {
                appendConversation(transport, c, 2);
            }
        }
    }

    // Direct messages (gated on the adapter's directMessages capability).
    if (showDms) {
        pushSection(transport, QStringLiteral("dms"), tr("Direct Messages"), 1);
        if (isExpanded(secKey(transport, QStringLiteral("dms")))) {
            for (const QVariantMap& c : dms) {
                appendConversation(transport, c, 2);
            }
        }
    }

    // Browse / Join — the directory affordance (an action row, not foldable).
    if (directory) {
        Row r;
        r.label = tr("Browse");
        r.depth = 1;
        r.rowKind = QStringLiteral("action");
        r.section = QStringLiteral("browse");
        r.transport = transport;
        r.selId = QStringLiteral("browse:") + transport;
        m_rows.push_back(r);
    }
}

void IntegrationsTreeModel::appendSpace(const QString& transport, const QVariantMap& space,
                                        const QHash<QString, QList<QVariantMap>>& byParent,
                                        QSet<QString>& visited, int depth) {
    const QString id = space.value(QStringLiteral("id")).toString();
    if (visited.contains(id)) {
        return; // cycle guard
    }
    visited.insert(id);

    const QList<QVariantMap> children = byParent.value(id);
    Row r;
    r.label = space.value(QStringLiteral("title")).toString();
    r.depth = depth;
    r.rowKind = QStringLiteral("space");
    r.convType = QStringLiteral("space");
    r.transport = transport;
    r.conversation = id;
    r.count = children.isEmpty() ? -1 : static_cast<int>(children.size());
    r.hasChildren = !children.isEmpty();
    r.foldKey = convKey(transport, id);
    r.selId = QStringLiteral("conv:") + transport + QLatin1Char('/') + id;
    r.expanded = isExpanded(r.foldKey);
    m_rows.push_back(r);

    if (!r.hasChildren || !r.expanded) {
        return;
    }
    for (const QVariantMap& child : children) {
        if (child.value(QStringLiteral("kind")).toString() == QStringLiteral("space")) {
            appendSpace(transport, child, byParent, visited, depth + 1);
        } else {
            appendConversation(transport, child, depth + 1);
        }
    }
}

void IntegrationsTreeModel::appendConversation(const QString& transport, const QVariantMap& conv,
                                               int depth) {
    Row r;
    r.label = conv.value(QStringLiteral("title")).toString();
    r.depth = depth;
    r.rowKind = QStringLiteral("conversation");
    r.convType = conv.value(QStringLiteral("kind")).toString();
    r.transport = transport;
    r.conversation = conv.value(QStringLiteral("id")).toString();
    r.selId = QStringLiteral("conv:") + transport + QLatin1Char('/') + r.conversation;
    m_rows.push_back(r);
}

void IntegrationsTreeModel::pushSection(const QString& transport, const QString& sectionId,
                                        const QString& label, int depth) {
    Row r;
    r.label = label;
    r.depth = depth;
    r.rowKind = QStringLiteral("section");
    r.section = sectionId;
    r.transport = transport;
    r.selectable = false;
    r.hasChildren = true; // so the disclosure plumbing folds the section
    r.foldKey = secKey(transport, sectionId);
    r.expanded = isExpanded(r.foldKey);
    m_rows.push_back(r);
}

int IntegrationsTreeModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

bool IntegrationsTreeModel::rowIsCurrent(const Row& r) const {
    return !r.selId.isEmpty() && r.selId == m_selKey;
}

QVariant IntegrationsTreeModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case LabelRole:
        return r.label;
    case DepthRole:
        return r.depth;
    case RowKindRole:
        return r.rowKind;
    case ConvTypeRole:
        return r.convType;
    case SectionRole:
        return r.section;
    case TransportRole:
        return r.transport;
    case ConversationRole:
        return r.conversation;
    case FamilyRole:
        return r.family;
    case PersonIdRole:
        return r.personId;
    case EnabledRole:
        return r.enabled;
    case ConnectionRole:
        return r.connection;
    case PresenceRole:
        return r.presence;
    case CountRole:
        return r.count;
    case HasChildrenRole:
        return r.hasChildren;
    case ExpandedRole:
        return r.expanded;
    case SelectableRole:
        return r.selectable;
    case CurrentRole:
        return rowIsCurrent(r);
    default:
        return {};
    }
}

QHash<int, QByteArray> IntegrationsTreeModel::roleNames() const {
    return {
        {LabelRole, "label"},
        {DepthRole, "depth"},
        {RowKindRole, "rowKind"},
        {ConvTypeRole, "convType"},
        {SectionRole, "section"},
        {TransportRole, "transport"},
        {ConversationRole, "conversation"},
        {FamilyRole, "family"},
        {PersonIdRole, "personId"},
        {EnabledRole, "accountEnabled"},
        {ConnectionRole, "connection"},
        {PresenceRole, "presence"},
        {CountRole, "count"},
        {HasChildrenRole, "hasChildren"},
        {ExpandedRole, "expanded"},
        {SelectableRole, "selectable"},
        {CurrentRole, "current"},
    };
}

void IntegrationsTreeModel::emitCurrentChanged() {
    if (!m_rows.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_rows.size()) - 1), {CurrentRole});
    }
}

void IntegrationsTreeModel::setSelectionFromRow(int row) {
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const Row& r = m_rows.at(row);
    if (!r.selectable) {
        return;
    }
    m_selKey = r.selId;
    if (r.rowKind == QStringLiteral("conversation")) {
        emit conversationActivated(r.transport, r.conversation);
    } else if (r.rowKind == QStringLiteral("account")) {
        emit accountActivated(r.transport);
    } else if (r.rowKind == QStringLiteral("person")) {
        emit personActivated(r.personId);
    } else if (r.rowKind == QStringLiteral("action") && r.section == QStringLiteral("browse")) {
        emit browseRequested(r.transport);
    }
    // A space row is a selectable container: highlight only (its rooms carry the activation).
    emitCurrentChanged();
}

void IntegrationsTreeModel::activate(int row) {
    setSelectionFromRow(row);
}

int IntegrationsTreeModel::currentRow() const {
    for (int i = 0; i < m_rows.size(); ++i) {
        if (rowIsCurrent(m_rows.at(i))) {
            return i;
        }
    }
    return -1;
}

void IntegrationsTreeModel::toggleExpand(int row) {
    if (row < 0 || row >= m_rows.size()) {
        return;
    }
    const QString foldKey = m_rows.at(row).foldKey;
    if (foldKey.isEmpty()) {
        return; // a leaf (conversation / person / action) does not fold
    }
    if (isExpanded(foldKey)) {
        m_collapsed.insert(foldKey);
    } else {
        m_collapsed.remove(foldKey);
    }
    rebuild();
}

void IntegrationsTreeModel::collectFoldKeys(QSet<QString>& out) const {
    for (const QVariantMap& inst : accountRows()) {
        const QString transport = inst.value(QStringLiteral("transport")).toString();
        const QString family = inst.value(QStringLiteral("family")).toString();
        const QVariantMap adapter = adapterFor(family);
        const QVariantMap caps = adapter.value(QStringLiteral("capabilities")).toMap();
        const bool dmCap = caps.value(QStringLiteral("directMessages")).toBool();
        out.insert(acctKey(transport));
        if (!personRows(transport).isEmpty()) {
            out.insert(secKey(transport, QStringLiteral("persons")));
        }
        bool anyChannel = false;
        bool anyDm = false;
        QSet<QString> knownIds;
        const QList<QVariantMap> convs = conversationRows(transport);
        for (const QVariantMap& c : convs) {
            knownIds.insert(c.value(QStringLiteral("id")).toString());
        }
        for (const QVariantMap& c : convs) {
            const QString kind = c.value(QStringLiteral("kind")).toString();
            const QString id = c.value(QStringLiteral("id")).toString();
            const QString parent = c.value(QStringLiteral("parent")).toString();
            const bool root = parent.isEmpty() || parent == id || !knownIds.contains(parent);
            if (kind == QStringLiteral("space") && root) {
                out.insert(convKey(transport, id));
            } else if (isDmKind(kind)) {
                anyDm = true;
            } else if (isChannelKind(kind) && root) {
                anyChannel = true;
            }
        }
        if (anyChannel) {
            out.insert(secKey(transport, QStringLiteral("channels")));
        }
        if (dmCap && anyDm) {
            out.insert(secKey(transport, QStringLiteral("dms")));
        }
    }
}

bool IntegrationsTreeModel::anyExpanded() const {
    QSet<QString> keys;
    collectFoldKeys(keys);
    return std::ranges::any_of(keys, [this](const QString& k) { return isExpanded(k); });
}

void IntegrationsTreeModel::expandAll() {
    QSet<QString> keys;
    collectFoldKeys(keys);
    for (const QString& k : keys) {
        m_collapsed.remove(k);
    }
    rebuild();
}

void IntegrationsTreeModel::collapseAll() {
    QSet<QString> keys;
    collectFoldKeys(keys);
    for (const QString& k : keys) {
        m_collapsed.insert(k);
    }
    rebuild();
}

void IntegrationsTreeModel::requestAddAccount() {
    emit addAccountRequested();
}

void IntegrationsTreeModel::requestEditAccount(const QString& transport) {
    emit editAccountRequested(transport);
}

void IntegrationsTreeModel::requestRemoveAccount(const QString& transport) {
    emit removeAccountRequested(transport);
}

void IntegrationsTreeModel::connectAccount(const QString& transport) {
    if (m_registry != nullptr) {
        m_registry->connectAccount(transport);
    }
}

void IntegrationsTreeModel::setAccountEnabled(const QString& transport, bool enabled) {
    if (m_registry != nullptr) {
        m_registry->setEnabled(transport, enabled);
    }
}
