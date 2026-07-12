// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "integrations_tree_model.h"

#include "transports/ipersons_service.h"
#include "transports/itransport_registry.h"

#include <algorithm>

using transports::IPersonsService;
using transports::ITransportRegistry;

namespace {
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
bool isDmKind(const QString& kind) {
    return kind == QStringLiteral("dm") || kind == QStringLiteral("groupdm");
}
} // namespace

IntegrationsTreeModel::IntegrationsTreeModel(QObject* parent) : QAbstractListModel(parent) {}

QObject* IntegrationsTreeModel::registry() const {
    return m_registry;
}

void IntegrationsTreeModel::setRegistry(QObject* registry) {
    auto* reg = qobject_cast<ITransportRegistry*>(registry);
    if (m_registry == reg) {
        return;
    }
    if (m_registry != nullptr) {
        // ITransportRegistry declares a `disconnect(QString)` verb that shadows
        // QObject::disconnect; use the explicit static overload to drop our connections.
        QObject::disconnect(m_registry, nullptr, this, nullptr);
    }
    m_registry = reg;
    if (m_registry != nullptr) {
        connect(m_registry, &ITransportRegistry::instancesChanged, this,
                &IntegrationsTreeModel::rebuild);
        connect(m_registry, &ITransportRegistry::adaptersChanged, this,
                &IntegrationsTreeModel::rebuild);
        connect(m_registry, &ITransportRegistry::conversationsChanged, this,
                [this](const QString&) { rebuild(); });
    }
    emit registryChanged();
    rebuild();
}

QObject* IntegrationsTreeModel::persons() const {
    return m_persons;
}

void IntegrationsTreeModel::setPersons(QObject* persons) {
    auto* svc = qobject_cast<IPersonsService*>(persons);
    if (m_persons == svc) {
        return;
    }
    if (m_persons != nullptr) {
        m_persons->disconnect(this);
    }
    m_persons = svc;
    if (m_persons != nullptr) {
        connect(m_persons, &IPersonsService::personsChanged, this,
                [this](const QVariantList&) { rebuild(); });
    }
    emit personsChanged();
    rebuild();
}

QVariantMap IntegrationsTreeModel::adapterFor(const QString& family) const {
    if (m_registry == nullptr) {
        return {};
    }
    const QVariantList adapters = m_registry->availableAdapters();
    for (const QVariant& a : adapters) {
        const QVariantMap row = a.toMap();
        if (row.value(QStringLiteral("family")).toString() == family) {
            return row;
        }
    }
    return {};
}

bool IntegrationsTreeModel::isExpanded(const QString& foldKey) const {
    return !m_collapsed.contains(foldKey);
}

// The persons (whole rows) with at least one endpoint on `transport`.
static QList<QVariantMap> personsOnTransport(IPersonsService* persons, const QString& transport) {
    QList<QVariantMap> out;
    if (persons == nullptr) {
        return out;
    }
    for (const QVariant& p : persons->persons()) {
        const QVariantMap person = p.toMap();
        const QVariantList endpoints = person.value(QStringLiteral("endpoints")).toList();
        for (const QVariant& e : endpoints) {
            if (e.toMap().value(QStringLiteral("transport")).toString() == transport) {
                out.append(person);
                break;
            }
        }
    }
    return out;
}

void IntegrationsTreeModel::rebuild() {
    beginResetModel();
    m_rows.clear();
    if (m_registry != nullptr) {
        const QVariantList instances = m_registry->instances();
        for (const QVariant& i : instances) {
            const QVariantMap inst = i.toMap();
            appendAccount(inst, adapterFor(inst.value(QStringLiteral("family")).toString()));
        }
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
    const QVariantList convVariants = m_registry->conversations(transport);
    QList<QVariantMap> convs;
    QSet<QString> knownIds;
    for (const QVariant& c : convVariants) {
        const QVariantMap m = c.toMap();
        convs.append(m);
        knownIds.insert(m.value(QStringLiteral("id")).toString());
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

    const QList<QVariantMap> people = personsOnTransport(m_persons, transport);
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
                const QString alias = person.value(QStringLiteral("alias")).toString();
                QString label = alias;
                QString presence;
                if (label.isEmpty()) {
                    // Fall back to the display name of the endpoint on this transport.
                    for (const QVariant& e : person.value(QStringLiteral("endpoints")).toList()) {
                        const QVariantMap ep = e.toMap();
                        if (ep.value(QStringLiteral("transport")).toString() == transport) {
                            label = ep.value(QStringLiteral("displayName")).toString();
                            presence = ep.value(QStringLiteral("presence")).toString();
                            break;
                        }
                    }
                } else {
                    for (const QVariant& e : person.value(QStringLiteral("endpoints")).toList()) {
                        const QVariantMap ep = e.toMap();
                        if (ep.value(QStringLiteral("transport")).toString() == transport) {
                            presence = ep.value(QStringLiteral("presence")).toString();
                            break;
                        }
                    }
                }
                r.label = label;
                r.depth = 2;
                r.rowKind = QStringLiteral("person");
                r.transport = transport;
                r.personId = person.value(QStringLiteral("id")).toString();
                r.presence = presence;
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
    if (m_registry == nullptr) {
        return;
    }
    for (const QVariant& i : m_registry->instances()) {
        const QVariantMap inst = i.toMap();
        const QString transport = inst.value(QStringLiteral("transport")).toString();
        const QString family = inst.value(QStringLiteral("family")).toString();
        const QVariantMap adapter = adapterFor(family);
        const QVariantMap caps = adapter.value(QStringLiteral("capabilities")).toMap();
        const bool dmCap = caps.value(QStringLiteral("directMessages")).toBool();
        out.insert(acctKey(transport));
        if (!personsOnTransport(m_persons, transport).isEmpty()) {
            out.insert(secKey(transport, QStringLiteral("persons")));
        }
        bool anyChannel = false;
        bool anyDm = false;
        QSet<QString> knownIds;
        const QVariantList convs = m_registry->conversations(transport);
        for (const QVariant& c : convs) {
            knownIds.insert(c.toMap().value(QStringLiteral("id")).toString());
        }
        for (const QVariant& c : convs) {
            const QVariantMap m = c.toMap();
            const QString kind = m.value(QStringLiteral("kind")).toString();
            const QString id = m.value(QStringLiteral("id")).toString();
            const QString parent = m.value(QStringLiteral("parent")).toString();
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
