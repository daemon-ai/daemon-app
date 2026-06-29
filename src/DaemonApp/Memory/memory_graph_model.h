// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "memory/memory_dtos.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>
#include <QVariantMap>

namespace memory {
class IMemoryService;
}

namespace memoryui {

// Holds the current memory graph snapshot and a computed 2D layout. Exposed as a
// list model of nodes (x/y in normalized [0,1] space) plus edge/neighbour
// invokables. The GUI MemoryGraphView renders nodes (Repeater) and edges (Canvas)
// from this; the TUI graph view uses neighborsOf() for an adjacency drill-down.
// Layout is force-directed (Fruchterman-Reingold) computed in C++ so both front
// ends agree on positions.
class MemoryGraphModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* service READ service WRITE setService NOTIFY serviceChanged)
    Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY kindChanged)
    Q_PROPERTY(QString seed READ seed WRITE setSeed NOTIFY seedChanged)
    Q_PROPERTY(int depth READ depth WRITE setDepth NOTIFY depthChanged)
    Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId NOTIFY selectionChanged)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY graphChanged)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY graphChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        KindRole,
        LabelRole,
        WeightRole,
        XRole,
        YRole,
        SelectedRole,
        DegreeRole,
    };
    Q_ENUM(Role)

    explicit MemoryGraphModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* service() const;
    void setService(QObject* service);
    [[nodiscard]] QString kind() const { return m_kind; }
    void setKind(const QString& kind);
    [[nodiscard]] QString seed() const { return m_seed; }
    void setSeed(const QString& seed);
    [[nodiscard]] int depth() const { return m_depth; }
    void setDepth(int depth);
    [[nodiscard]] QString selectedId() const { return m_selectedId; }
    void setSelectedId(const QString& id);
    [[nodiscard]] int nodeCount() const { return static_cast<int>(m_graph.nodes.size()); }
    [[nodiscard]] int edgeCount() const { return static_cast<int>(m_graph.edges.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // [{ source, target, edgeType, label, weight, x1, y1, x2, y2 }] in [0,1] space.
    Q_INVOKABLE QVariantList edges() const;
    // Neighbour rows for a node id: [{ id, label, kind, edgeType, label/predicate }].
    Q_INVOKABLE QVariantList neighborsOf(const QString& id) const;
    Q_INVOKABLE QVariantMap nodeById(const QString& id) const;
    // Re-request the current (kind, seed, depth) graph.
    Q_INVOKABLE void refresh();
    // Focus + expand from a node (BFS one hop deeper).
    Q_INVOKABLE void expand(const QString& id);
    // Clear the seed/focus and show the whole graph for the current kind.
    Q_INVOKABLE void resetFocus();

signals:
    void serviceChanged();
    void kindChanged();
    void seedChanged();
    void depthChanged();
    void selectionChanged();
    void graphChanged();

private:
    void onGraphReady(const memory::MemoryGraph& graph);
    void onScopeChanged();
    void layout();

    memory::IMemoryService* m_service = nullptr;
    memory::MemoryGraph m_graph;
    QHash<QString, QPointF> m_pos;
    QHash<QString, int> m_degree;
    QString m_kind = QStringLiteral("association");
    QString m_seed;
    int m_depth = 1;
    QString m_selectedId;
};

} // namespace memoryui
