#include "memory_timeline_model.h"

#include "memory/imemory_service.h"

namespace memoryui {

MemoryTimelineModel::MemoryTimelineModel(QObject* parent)
    : QAbstractListModel(parent)
{
    memory::registerMemoryMetatypes();
}

QObject* MemoryTimelineModel::service() const { return m_service; }

void MemoryTimelineModel::setService(QObject* service)
{
    auto* svc = qobject_cast<memory::IMemoryService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    if (m_service) {
        connect(m_service, &memory::IMemoryService::timelineReady, this,
                &MemoryTimelineModel::onTimelineReady);
        connect(m_service, &memory::IMemoryService::scopeChanged, this,
                &MemoryTimelineModel::onScopeChanged);
        refresh();
    }
}

void MemoryTimelineModel::setGroup(const QString& group)
{
    if (m_group == group)
        return;
    m_group = group;
    emit groupChanged();
    refresh();
}

int MemoryTimelineModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant MemoryTimelineModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row& r = m_rows.at(index.row());
    switch (role) {
    case IsHeaderRole:
        return r.header;
    case GroupKeyRole:
        return r.groupKey;
    case KindRole:
        return r.ev.kind;
    case MemoryIdRole:
        return r.ev.memoryId;
    case AtRole:
        return r.ev.at;
    case Qt::DisplayRole:
    case SummaryRole:
        return r.header ? r.groupKey : r.ev.summary;
    default:
        return {};
    }
}

QHash<int, QByteArray> MemoryTimelineModel::roleNames() const
{
    return {
        { IsHeaderRole, "isHeader" }, { GroupKeyRole, "groupKey" }, { KindRole, "eventKind" },
        { MemoryIdRole, "memId" },    { AtRole, "at" },             { SummaryRole, "summary" },
    };
}

void MemoryTimelineModel::refresh()
{
    if (m_service)
        m_service->requestTimeline(m_group, 200);
}

void MemoryTimelineModel::onTimelineReady(const QList<memory::MemoryTimelineGroup>& groups)
{
    beginResetModel();
    m_rows.clear();
    for (const memory::MemoryTimelineGroup& g : groups) {
        m_rows.append(Row{ true, g.key, {} });
        for (const memory::MemoryEvent& ev : g.events)
            m_rows.append(Row{ false, g.key, ev });
    }
    endResetModel();
}

void MemoryTimelineModel::onScopeChanged() { refresh(); }

} // namespace memoryui
