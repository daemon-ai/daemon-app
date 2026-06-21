#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

// The slash / @ completion item list, backing the composer's CompletionPopover
// (GUI) and the TUI completion overlay. Ported from CompletionProvider.qml: the
// canned client-side data + the case-insensitive substring `search`, plus a model
// so both front ends render the same rows. GUI-free.
//
// Each item is { label, hint, group, value, action }. `action`: "insert" inserts
// `value` as text; any other string is a command the composer forwards to the host
// via commandInvoked(action) (with "clear" handled inline by the controller).
class CompletionModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Item {
        QString label;
        QString hint;
        QString group;
        QString value;
        QString action;
    };

    enum Role {
        LabelRole = Qt::UserRole + 1,
        HintRole,
        GroupRole,
        ValueRole,
        ActionRole,
    };

    explicit CompletionModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(m_items.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QList<Item>& items);
    [[nodiscard]] Item at(int index) const;

    // Filter the slash/mention pool by a case-insensitive substring of `query`.
    // `kind` is "mention" (the @ pool) or "slash" (everything else).
    [[nodiscard]] static QList<Item> search(const QString& kind, const QString& query);

signals:
    void countChanged();

private:
    QList<Item> m_items;
};
