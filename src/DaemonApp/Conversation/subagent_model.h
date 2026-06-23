#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantList>

// Live delegation/subagent rows for the composer status stack (Hermes' live
// subagent rows). Fed off the same turn event stream as the status bar: each
// "subagent" event upserts a row keyed by id, carrying a title, a status
// ("running"|"done"|"error") and a short live detail line. Owned by
// ConversationOrchestrator; rendered as a QML model (GUI) and read directly in
// the TUI. Pre-shapes the daemon's subagent.* contract.
class SubagentModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int runningCount READ runningCount NOTIFY countChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY countChanged)

public:
    struct Item {
        QString id;
        QString title;
        QString status;
        QString detail;
    };

    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        StatusRole,
        DetailRole,
    };

    explicit SubagentModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(m_items.size()); }
    [[nodiscard]] int runningCount() const;
    [[nodiscard]] int failedCount() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Upsert any "subagent" events found in the batch (other types are ignored),
    // keyed by id; emits countChanged when the set or any running-state changes.
    void applyEvents(const QVariantList& events);
    void clear();

    [[nodiscard]] QString titleAt(int index) const;
    [[nodiscard]] QString statusAt(int index) const;
    [[nodiscard]] QString detailAt(int index) const;

signals:
    void countChanged();

private:
    QList<Item> m_items;
    QHash<QString, int> m_indexById;
};
