// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "integrations_tree_model.h"

#include "transports/ipersons_service.h"
#include "transports/itransport_registry.h"

// RED stub: the API compiles and the tests run, but the tree is empty and no intents route — the
// assertions in tst_integrations_tree fail. GREEN fills in the composition logic.

IntegrationsTreeModel::IntegrationsTreeModel(QObject* parent) : QAbstractListModel(parent) {}

QObject* IntegrationsTreeModel::registry() const {
    return m_registry;
}

void IntegrationsTreeModel::setRegistry(QObject* registry) {
    m_registry = qobject_cast<transports::ITransportRegistry*>(registry);
    emit registryChanged();
    rebuild();
}

QObject* IntegrationsTreeModel::persons() const {
    return m_persons;
}

void IntegrationsTreeModel::setPersons(QObject* persons) {
    m_persons = qobject_cast<transports::IPersonsService*>(persons);
    emit personsChanged();
    rebuild();
}

bool IntegrationsTreeModel::anyExpanded() const {
    return false;
}

int IntegrationsTreeModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant IntegrationsTreeModel::data(const QModelIndex& /*index*/, int /*role*/) const {
    return {};
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
        {EnabledRole, "enabled"},
        {ConnectionRole, "connection"},
        {PresenceRole, "presence"},
        {CountRole, "count"},
        {HasChildrenRole, "hasChildren"},
        {ExpandedRole, "expanded"},
        {SelectableRole, "selectable"},
        {CurrentRole, "current"},
    };
}

void IntegrationsTreeModel::activate(int /*row*/) {}
void IntegrationsTreeModel::toggleExpand(int /*row*/) {}
void IntegrationsTreeModel::expandAll() {}
void IntegrationsTreeModel::collapseAll() {}
void IntegrationsTreeModel::requestAddAccount() {}
void IntegrationsTreeModel::requestEditAccount(const QString& /*transport*/) {}
void IntegrationsTreeModel::requestRemoveAccount(const QString& /*transport*/) {}
void IntegrationsTreeModel::connectAccount(const QString& /*transport*/) {}
void IntegrationsTreeModel::setAccountEnabled(const QString& /*transport*/, bool /*enabled*/) {}

int IntegrationsTreeModel::currentRow() const {
    return -1;
}

void IntegrationsTreeModel::rebuild() {
    beginResetModel();
    m_rows.clear();
    endResetModel();
    emit treeChanged();
}

void IntegrationsTreeModel::appendAccount(const QVariantMap& /*instance*/,
                                          const QVariantMap& /*adapter*/) {}
void IntegrationsTreeModel::appendSpace(const QString& /*transport*/, const QVariantMap& /*space*/,
                                        const QHash<QString, QList<QVariantMap>>& /*byParent*/,
                                        QSet<QString>& /*visited*/, int /*depth*/) {}
void IntegrationsTreeModel::appendConversation(const QString& /*transport*/,
                                               const QVariantMap& /*conv*/, int /*depth*/) {}
void IntegrationsTreeModel::pushSection(const QString& /*transport*/, const QString& /*sectionId*/,
                                        const QString& /*label*/, int /*depth*/) {}

bool IntegrationsTreeModel::isExpanded(const QString& foldKey) const {
    return !m_collapsed.contains(foldKey);
}
void IntegrationsTreeModel::collectFoldKeys(QSet<QString>& /*out*/) const {}
void IntegrationsTreeModel::setSelectionFromRow(int /*row*/) {}
bool IntegrationsTreeModel::rowIsCurrent(const Row& /*r*/) const {
    return false;
}
void IntegrationsTreeModel::emitCurrentChanged() {}

QVariantMap IntegrationsTreeModel::adapterFor(const QString& /*family*/) const {
    return {};
}
