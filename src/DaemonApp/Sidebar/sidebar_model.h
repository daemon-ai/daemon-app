#pragma once

#include "domain/sidebar_node.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class IConversationStore;
}

// Flat sidebar list with separator rows: All Conversations / Archived /
// "Folders" + folder rows / "Tags" + tag rows, each with a conversation count.
// `store` is bound from QML to the shared chat store (a QObject*).
class SidebarModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        CountRole,
        NodeTypeRole,
        NodeIdRole,
        IsSeparatorRole,
        SelectableRole,
        ColorRole, // tag color for the dot; empty otherwise
    };

    explicit SidebarModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Activate a row; emits scopeSelected for selectable rows.
    Q_INVOKABLE void activate(int row);

signals:
    void storeChanged();
    void scopeSelected(int nodeType, int nodeId);

private:
    struct Row {
        QString label;
        int count = -1;
        domain::NodeType type = domain::NodeType::AllConversations;
        int nodeId = -1;
        bool separator = false;
        bool selectable = true;
        QString color; // tag dot color; empty for non-tag rows
    };

    void rebuild();

    persistence::IConversationStore* m_store = nullptr;
    QList<Row> m_rows;
};
