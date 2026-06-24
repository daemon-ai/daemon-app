#pragma once

#include "memory/memory_dtos.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace memory {
class IMemoryService;
}

namespace memoryui {

// Flattened event timeline (group headers interleaved with event rows) over the
// IMemoryService seam. Shared by the GUI timeline ListView (using the isHeader
// role for section styling) and the TUI grouped painted view.
class MemoryTimelineModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY groupChanged)

public:
    enum Role {
        IsHeaderRole = Qt::UserRole + 1,
        GroupKeyRole,
        KindRole,
        MemoryIdRole,
        AtRole,
        SummaryRole,
    };
    Q_ENUM(Role)

    explicit MemoryTimelineModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] QString group() const { return m_group; }
    void setGroup(const QString& group);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();

signals:
    void serviceChanged();
    void groupChanged();

private:
    struct Row {
        bool header = false;
        QString groupKey;
        memory::MemoryEvent ev;
    };

    void onTimelineReady(const QList<memory::MemoryTimelineGroup>& groups);
    void onScopeChanged();

    memory::IMemoryService* m_service = nullptr;
    QList<Row> m_rows;
    QString m_group = QStringLiteral("day");
};

} // namespace memoryui
