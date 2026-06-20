#pragma once

#include "domain/sidebar_node.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class IConversationStore;
}

namespace domain {
struct AgentNode;
}

// Flattened agent-tree sidebar (VSCode AsyncDataTree style): a single flat list
// of rows where the supervision tree is rendered as indented rows. Rows are:
// All Conversations / Archived / a "Fleet" header + the recursively flattened
// node tree (only descending into expanded nodes) / a "Tags" header + tag rows.
//
// The flatten is uniformly recursive with NO depth limit and NO per-level
// branching: every node row carries its `depth`, `hasChildren`, `expanded`,
// cosmetic `kind`, and `state`. `store` is bound from QML to the shared chat
// store (a QObject*).
class SidebarModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        CountRole,
        NodeTypeRole,
        NodeIdRole,     // tag id for Tag rows; -1 otherwise
        AgentIdRole,    // agent node id for Node rows; empty otherwise
        IsSeparatorRole,
        SelectableRole,
        ColorRole,      // tag color for the dot; empty otherwise
        DepthRole,      // tree depth (0 = top-level root / non-node rows)
        HasChildrenRole,
        ExpandedRole,
        KindRole,       // domain::AgentNodeKind (cosmetic) for Node rows
        StateRole,      // domain::AgentState for Node rows
    };

    explicit SidebarModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Activate a row; emits scopeSelected for selectable rows.
    Q_INVOKABLE void activate(int row);
    // Expand/collapse a node row (no-op for leaves and non-node rows). Rebuilds
    // the flattened view so children appear/disappear.
    Q_INVOKABLE void toggleExpand(int row);

signals:
    void storeChanged();
    // nodeType is a domain::NodeType; `id` is the tag id (Tag scope); `nodeId`
    // is the agent node id (Node scope). Unused fields are -1 / empty.
    void scopeSelected(int nodeType, int id, const QString& nodeId);

private:
    struct Row {
        QString label;
        int count = -1;
        domain::NodeType type = domain::NodeType::AllConversations;
        int nodeId = -1;     // tag id
        QString agentId;     // agent node id
        bool separator = false;
        bool selectable = true;
        QString color;       // tag dot color
        int depth = 0;
        bool hasChildren = false;
        bool expanded = false;
        int kind = 0;        // domain::AgentNodeKind
        int state = 0;       // domain::AgentState
    };

    void rebuild();
    void appendNodeRows(const domain::AgentNode& node, int depth);
    [[nodiscard]] bool isExpanded(const QString& id) const;

    persistence::IConversationStore* m_store = nullptr;
    QList<Row> m_rows;
    // Nodes are expanded by default; only explicitly-collapsed ids live here, so
    // the whole fleet tree is visible on first load (and toggling collapses).
    QSet<QString> m_collapsed;
};
