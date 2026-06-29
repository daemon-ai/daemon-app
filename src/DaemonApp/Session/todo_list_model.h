// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QVariantList>

// The composer status-stack todo list. A flat list of { text, done } rows shown
// above the composer during a turn. Extracted from the QML demo seam so the model
// and its populate/clear lifecycle live in shared C++ (owned by
// SessionOrchestrator); both front ends render the same rows. GUI-free: it is
// consumed as a QML model (role names below) and directly from C++ in the TUI.
class TodoListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Item {
        QString text;
        bool done = false;
    };

    enum Role {
        TextRole = Qt::UserRole + 1,
        DoneRole,
    };

    explicit TodoListModel(QObject* parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(m_items.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows. Each entry is a map { text: string, done: bool }.
    void setTodos(const QVariantList& items);
    void clear();

    [[nodiscard]] QString textAt(int index) const;
    [[nodiscard]] bool doneAt(int index) const;

signals:
    void countChanged();

private:
    QList<Item> m_items;
};
