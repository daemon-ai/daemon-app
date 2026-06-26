#include "memory_stats_model.h"

#include "memory/imemory_service.h"

#include <algorithm>

namespace memoryui {

MemoryStatsModel::MemoryStatsModel(QObject* parent) : QObject(parent) {
    memory::registerMemoryMetatypes();
}

QObject* MemoryStatsModel::service() const {
    return m_service;
}

void MemoryStatsModel::setService(QObject* service) {
    auto* svc = qobject_cast<memory::IMemoryService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    if (m_service) {
        connect(m_service, &memory::IMemoryService::statsReady, this,
                &MemoryStatsModel::onStatsReady);
        connect(m_service, &memory::IMemoryService::scopeChanged, this,
                &MemoryStatsModel::onScopeChanged);
        refresh();
    }
}

void MemoryStatsModel::refresh() {
    if (m_service)
        m_service->requestStats();
}

QVariantList MemoryStatsModel::toList(const QList<memory::Bucket>& buckets) {
    int maxCount = 1;
    for (const memory::Bucket& b : buckets)
        maxCount = std::max(maxCount, b.count);
    QVariantList out;
    out.reserve(buckets.size());
    for (const memory::Bucket& b : buckets) {
        out.append(QVariantMap{
            {QStringLiteral("key"), b.key},
            {QStringLiteral("count"), b.count},
            {QStringLiteral("fraction"), static_cast<double>(b.count) / maxCount},
        });
    }
    return out;
}

void MemoryStatsModel::onStatsReady(const memory::MemoryStats& stats) {
    m_stats = stats;
    emit changed();
}

void MemoryStatsModel::onScopeChanged() {
    refresh();
}

} // namespace memoryui
