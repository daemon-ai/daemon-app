// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "memory_list_model.h"

#include "memory/imemory_service.h"

namespace memoryui {

MemoryListModel::MemoryListModel(QObject* parent) : QAbstractListModel(parent) {
    memory::registerMemoryMetatypes();
}

QObject* MemoryListModel::service() const {
    return m_service;
}

void MemoryListModel::setService(QObject* service) {
    auto* svc = qobject_cast<memory::IMemoryService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    if (m_service) {
        connect(m_service, &memory::IMemoryService::pageReady, this, &MemoryListModel::onPageReady);
        connect(m_service, &memory::IMemoryService::scopeChanged, this,
                &MemoryListModel::onScopeChanged);
        refresh();
    }
}

void MemoryListModel::setSort(const QString& sort) {
    if (m_sort == sort)
        return;
    m_sort = sort;
    emit sortChanged();
    refresh();
}

int MemoryListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant MemoryListModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const memory::MemoryEntry& e = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case ContentRole:
        return e.content;
    case IdRole:
        return e.id;
    case SourceRole:
        return e.source;
    case TimestampRole:
        return e.timestamp;
    case SessionRole:
        return e.sessionId;
    case ScopeRole:
        return e.scope;
    case TierRole:
        return e.tier;
    case TierLevelRole:
        return e.tierLevel;
    case ImportanceRole:
        return e.importance;
    case VeracityRole:
        return e.veracity;
    case TrustTierRole:
        return e.trustTier;
    case RecallCountRole:
        return e.recallCount;
    case StatusRole:
        return e.status;
    case DegradationRole:
        return e.degradationLabel;
    case EffectiveWeightRole:
        return e.effectiveWeight;
    case ContaminatedRole:
        return e.contaminated;
    case ScoreRole:
        return e.score;
    default:
        return {};
    }
}

QHash<int, QByteArray> MemoryListModel::roleNames() const {
    return {
        {IdRole, "memId"},
        {ContentRole, "content"},
        {SourceRole, "source"},
        {TimestampRole, "timestamp"},
        {SessionRole, "sessionId"},
        {ScopeRole, "scope"},
        {TierRole, "tier"},
        {TierLevelRole, "tierLevel"},
        {ImportanceRole, "importance"},
        {VeracityRole, "veracity"},
        {TrustTierRole, "trustTier"},
        {RecallCountRole, "recallCount"},
        {StatusRole, "status"},
        {DegradationRole, "degradation"},
        {EffectiveWeightRole, "effectiveWeight"},
        {ContaminatedRole, "contaminated"},
        {ScoreRole, "score"},
    };
}

void MemoryListModel::setFilter(const QString& key, const QString& value) {
    if (value.isEmpty())
        m_filter.remove(key);
    else
        m_filter.insert(key, value);
    refresh();
}

void MemoryListModel::clearFilters() {
    if (m_filter.isEmpty())
        return;
    m_filter.clear();
    refresh();
}

void MemoryListModel::refresh() {
    if (!m_service)
        return;
    QVariantMap query = m_filter;
    query.insert(QStringLiteral("sort"), m_sort);
    query.insert(QStringLiteral("limit"), 200);
    m_service->requestList(query);
}

void MemoryListModel::search(const QString& text) {
    if (!m_service)
        return;
    if (text.trimmed().isEmpty()) {
        refresh();
        return;
    }
    m_service->requestSearch(text, 200);
}

QVariantMap MemoryListModel::entryAt(int row) const {
    if (row < 0 || row >= m_rows.size())
        return {};
    const memory::MemoryEntry& e = m_rows.at(row);
    return QVariantMap{
        {QStringLiteral("id"), e.id},
        {QStringLiteral("content"), e.content},
        {QStringLiteral("source"), e.source},
        {QStringLiteral("timestamp"), e.timestamp},
        {QStringLiteral("sessionId"), e.sessionId},
        {QStringLiteral("scope"), e.scope},
        {QStringLiteral("tier"), e.tier},
        {QStringLiteral("tierLevel"), e.tierLevel},
        {QStringLiteral("importance"), e.importance},
        {QStringLiteral("veracity"), e.veracity},
        {QStringLiteral("trustTier"), e.trustTier},
        {QStringLiteral("recallCount"), e.recallCount},
        {QStringLiteral("lastRecalled"), e.lastRecalled},
        {QStringLiteral("validUntil"), e.validUntil},
        {QStringLiteral("supersededBy"), e.supersededBy},
        {QStringLiteral("status"), e.status},
        {QStringLiteral("degradation"), e.degradationLabel},
        {QStringLiteral("trustWeight"), e.trustWeight},
        {QStringLiteral("degradationWeight"), e.degradationWeight},
        {QStringLiteral("effectiveWeight"), e.effectiveWeight},
        {QStringLiteral("contaminated"), e.contaminated},
        {QStringLiteral("score"), e.score},
    };
}

QString MemoryListModel::idAt(int row) const {
    return (row >= 0 && row < m_rows.size()) ? m_rows.at(row).id : QString();
}

void MemoryListModel::onPageReady(const QString& op, const QList<memory::MemoryEntry>& items,
                                  const QString& nextCursor) {
    Q_UNUSED(op)
    Q_UNUSED(nextCursor)
    beginResetModel();
    m_rows = items;
    endResetModel();
    emit countChanged();
}

void MemoryListModel::onScopeChanged() {
    refresh();
}

} // namespace memoryui
