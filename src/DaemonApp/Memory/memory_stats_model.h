#pragma once

#include "memory/memory_dtos.h"

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>

namespace memory {
class IMemoryService;
}

namespace memoryui {

// Aggregate counts + breakdowns for the Overview tab. Breakdown lists are exposed
// as [{ key, count, fraction }] so both the GUI bars and the TUI text gauges read
// the same normalized fraction (relative to the largest bucket).
class MemoryStatsModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(int working READ working NOTIFY changed)
    Q_PROPERTY(int episodic READ episodic NOTIFY changed)
    Q_PROPERTY(int scratchpad READ scratchpad NOTIFY changed)
    Q_PROPERTY(int total READ total NOTIFY changed)
    Q_PROPERTY(int triples READ triples NOTIFY changed)
    Q_PROPERTY(int facts READ facts NOTIFY changed)
    Q_PROPERTY(int conflicts READ conflicts NOTIFY changed)
    Q_PROPERTY(QVariantList bySource READ bySource NOTIFY changed)
    Q_PROPERTY(QVariantList byScope READ byScope NOTIFY changed)
    Q_PROPERTY(QVariantList byVeracity READ byVeracity NOTIFY changed)
    Q_PROPERTY(QVariantList byDegradation READ byDegradation NOTIFY changed)
    Q_PROPERTY(QVariantList bySession READ bySession NOTIFY changed)

public:
    explicit MemoryStatsModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);

    [[nodiscard]] int working() const { return m_stats.working; }
    [[nodiscard]] int episodic() const { return m_stats.episodic; }
    [[nodiscard]] int scratchpad() const { return m_stats.scratchpad; }
    [[nodiscard]] int total() const {
        return m_stats.working + m_stats.episodic + m_stats.scratchpad;
    }
    [[nodiscard]] int triples() const { return m_stats.triples; }
    [[nodiscard]] int facts() const { return m_stats.facts; }
    [[nodiscard]] int conflicts() const { return m_stats.conflicts; }
    [[nodiscard]] QVariantList bySource() const { return toList(m_stats.bySource); }
    [[nodiscard]] QVariantList byScope() const { return toList(m_stats.byScope); }
    [[nodiscard]] QVariantList byVeracity() const { return toList(m_stats.byVeracity); }
    [[nodiscard]] QVariantList byDegradation() const { return toList(m_stats.byDegradation); }
    [[nodiscard]] QVariantList bySession() const { return toList(m_stats.bySession); }

    Q_INVOKABLE void refresh();

signals:
    void serviceChanged();
    void changed();

private:
    [[nodiscard]] static QVariantList toList(const QList<memory::Bucket>& buckets);
    void onStatsReady(const memory::MemoryStats& stats);
    void onScopeChanged();

    memory::IMemoryService* m_service = nullptr;
    memory::MemoryStats m_stats;
};

} // namespace memoryui
