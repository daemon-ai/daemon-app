#pragma once

#include "memory/memory_dtos.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace memory {
class IMemoryService;
}

namespace memoryui {

// Flat, filterable/sortable list of memory rows over the IMemoryService seam.
// Backs both the Memories browser (op "list") and Search results (op "search");
// shared verbatim by the GUI ListView and the TUI painted list view.
class MemoryListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString sort READ sort WRITE setSort NOTIFY sortChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        ContentRole,
        SourceRole,
        TimestampRole,
        SessionRole,
        ScopeRole,
        TierRole,
        TierLevelRole,
        ImportanceRole,
        VeracityRole,
        TrustTierRole,
        RecallCountRole,
        StatusRole,
        DegradationRole,
        EffectiveWeightRole,
        ContaminatedRole,
        ScoreRole,
    };
    Q_ENUM(Role)

    explicit MemoryListModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] QString sort() const { return m_sort; }
    void setSort(const QString& sort);
    [[nodiscard]] int count() const { return static_cast<int>(m_rows.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Set a filter field (tier/source/veracity/status/text); empty value clears it.
    Q_INVOKABLE void setFilter(const QString& key, const QString& value);
    Q_INVOKABLE void clearFilters();
    // Re-run the list query (list mode).
    Q_INVOKABLE void refresh();
    // Run a search query (search mode); empty text falls back to refresh().
    Q_INVOKABLE void search(const QString& text);
    // Full field map for the detail drawer.
    Q_INVOKABLE QVariantMap entryAt(int row) const;
    Q_INVOKABLE QString idAt(int row) const;

signals:
    void serviceChanged();
    void sortChanged();
    void countChanged();

private:
    void onPageReady(const QString& op, const QList<memory::MemoryEntry>& items,
                     const QString& nextCursor);
    void onScopeChanged();

    memory::IMemoryService* m_service = nullptr;
    QList<memory::MemoryEntry> m_rows;
    QVariantMap m_filter;
    QString m_sort = QStringLiteral("recent");
};

} // namespace memoryui
