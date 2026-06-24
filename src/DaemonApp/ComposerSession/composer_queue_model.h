#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>

// The prompt queue backing the composer's QueuePanel. A flat list of
// { text, refs } entries; `refs` is the space-joined @file:/@image: string
// captured when the entry was queued. GUI-free (no QtQuick): it is consumed both
// as a QML model (role names below) and directly from C++ in the TUI.
class ComposerQueueModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Entry {
        QString text;
        QString refs;
    };

    enum Role {
        TextRole = Qt::UserRole + 1,
        RefsRole,
    };

    explicit ComposerQueueModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(m_entries.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const QString& text, const QString& refs);
    void setEntry(int index, const QString& text, const QString& refs);
    void insertAt(int index, const QString& text, const QString& refs);
    void removeAt(int index);
    void clear();

    [[nodiscard]] QString textAt(int index) const;
    [[nodiscard]] QString refsAt(int index) const;

    // Bulk swap for per-session stash/restore.
    [[nodiscard]] QList<Entry> entries() const { return m_entries; }
    void setEntries(const QList<Entry>& entries);

signals:
    void countChanged();

private:
    QList<Entry> m_entries;
};
