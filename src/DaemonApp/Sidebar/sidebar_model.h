#pragma once

#include "domain/unit_node.h"
#include "domain/sidebar_node.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class ISessionStore;
}

namespace domain {
struct UnitNode;
}

// Flattened supervision-tree sidebar (VSCode AsyncDataTree style): a single flat
// list of rows where the unit tree is rendered as indented rows. Rows are: All
// Conversations / Archived / a "Fleet" header + the recursively flattened unit
// tree (only descending into expanded units) / a "Tags" header + tag rows.
//
// The flatten is uniformly recursive with NO depth limit and NO per-level
// branching: every unit row carries its `depth`, `hasChildren`, `expanded`,
// cosmetic `kind`, and `state`.
//
// The model OWNS the selection (by identity, not row index) so the highlight
// survives expand/collapse rebuilds, and owns all tree navigation/mutation so the
// structural logic lives in one place (and stays unit-testable in C++). `store` is
// bound from QML to the shared session store (a QObject*).
class SidebarModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)
    // True when at least one unit-with-children is currently expanded; drives the
    // Fleet header's collapse-all/expand-all toggle. Refreshed on rebuild.
    Q_PROPERTY(bool anyExpanded READ anyExpanded NOTIFY treeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        CountRole,
        NodeTypeRole,
        TagIdRole,      // tag id for Tag rows; -1 otherwise
        UnitIdRole,     // unit id for Unit rows; empty otherwise
        IsSeparatorRole,
        SelectableRole,
        ColorRole,      // tag color for the dot; empty otherwise
        DepthRole,      // tree depth (0 = top-level root / non-unit rows)
        HasChildrenRole,
        ExpandedRole,
        KindRole,       // domain::UnitKind (cosmetic) for Unit rows
        StateRole,      // domain::UnitState for Unit rows
        CurrentRole,    // true for the currently-selected row (identity match)
        ProfileRole,    // the unit's profile (ProfileRef == agent identity); empty if none
        SessionIdRole,  // the unit's backing session id (UnitNode.session)
    };

    explicit SidebarModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    [[nodiscard]] bool anyExpanded() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Select a row: records the selection by identity, emits scopeSelected, and
    // repaints the highlight. No-op for separators.
    Q_INVOKABLE void activate(int row);
    // Expand/collapse a unit row (no-op for leaves and non-unit rows). If a
    // collapse would hide the current selection, the selection moves up to the
    // collapsed unit (VSCode behavior).
    Q_INVOKABLE void toggleExpand(int row);

    // Keyboard navigation, all operating on the currently-selected row.
    Q_INVOKABLE void selectNext();       // Down: next selectable row
    Q_INVOKABLE void selectPrevious();   // Up: previous selectable row
    Q_INVOKABLE void collapseCurrent();  // Left: collapse, else go to parent
    Q_INVOKABLE void expandCurrent();    // Right: expand, else go to first child
    Q_INVOKABLE void activateCurrent();  // Enter: re-emit the current scope

    // Whole-tree expand/collapse (Fleet header control).
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    // Mutations via the store; both select the freshly-created row.
    Q_INVOKABLE void createRootUnit();
    Q_INVOKABLE void createTag();

    // Row index of the current selection (-1 if it is hidden/none). Lets QML keep
    // the view scrolled to the selection after keyboard navigation.
    [[nodiscard]] Q_INVOKABLE int currentRow() const;

signals:
    void storeChanged();
    void treeChanged();
    // nodeType is a domain::NodeType; `tagId` is the tag id (Tag scope); `unitId`
    // is the supervision unit id (Unit scope). Unused fields are -1 / empty.
    void scopeSelected(int nodeType, int tagId, const QString& unitId);

private:
    struct Row {
        QString label;
        int count = -1;
        domain::NodeType type = domain::NodeType::AllConversations;
        int tagId = -1;      // tag id
        QString unitId;      // unit id (string at the model boundary)
        bool separator = false;
        bool selectable = true;
        QString color;       // tag dot color
        int depth = 0;
        bool hasChildren = false;
        bool expanded = false;
        int kind = 0;        // domain::UnitKind
        int state = 0;       // domain::UnitState
        QString profile;     // ProfileRef (agent identity) for Unit rows; empty otherwise
        QString session;     // backing session id for Unit rows; empty otherwise
    };

    void rebuild();
    void appendUnitRows(const domain::UnitNode& node, int depth);
    [[nodiscard]] bool isExpanded(const QString& id) const;

    // Selection helpers (identity-based).
    void setSelectionFromRow(int row);     // records identity + emits + repaints
    [[nodiscard]] bool rowIsCurrent(const Row& r) const;
    void emitCurrentChanged();             // dataChanged(CurrentRole) for all rows
    [[nodiscard]] int adjacentSelectableRow(int from, int delta) const;
    [[nodiscard]] int parentRow(int row) const;
    // True when the current selection is `rootId` or a descendant of it.
    [[nodiscard]] bool selectionInSubtree(const QString& rootId) const;
    // Collect ids of every unit that currently has children (any depth).
    void collectExpandableIds(const QString& parentId, QSet<QString>& out) const;

    persistence::ISessionStore* m_store = nullptr;
    QList<Row> m_rows;
    // Units are expanded by default; only explicitly-collapsed ids live here, so
    // the whole fleet tree is visible on first load (and toggling collapses).
    QSet<QString> m_collapsed;

    // The selection, stored by identity so it is stable across rebuilds.
    domain::NodeType m_selType = domain::NodeType::AllConversations;
    int m_selTag = -1;
    QString m_selUnit;
};
