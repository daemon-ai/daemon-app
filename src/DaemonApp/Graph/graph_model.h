// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QPointF>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>
#include <QVariantMap>

namespace daemongraph {

// A generic 2-D graph model for the Daemon Kit graph view. Arbitrary nodes + edges are set from
// QML/C++ (no service/domain coupling - every surface, e.g. the memory graph or the routing
// topology, feeds it its own nodes/edges), a force-directed (Fruchterman-Reingold) layout is
// computed in C++ (normalized [0,1] so both front ends could agree on positions), and the nodes are
// exposed as a flat list model (x/y/kind/label/degree/selected + the whole node map under
// `nodeData` for kind-specific delegate fields) alongside edge/neighbour invokables.
class GraphModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId NOTIFY selectionChanged)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY graphChanged)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY graphChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        KindRole,
        LabelRole,
        XRole,
        YRole,
        DegreeRole,
        SelectedRole,
        DataRole, // the whole node QVariantMap (passthrough fields)
    };
    Q_ENUM(Role)

    explicit GraphModel(QObject* parent = nullptr);

    [[nodiscard]] QString selectedId() const { return m_selectedId; }
    void setSelectedId(const QString& id);
    [[nodiscard]] int nodeCount() const { return static_cast<int>(m_nodes.size()); }
    [[nodiscard]] int edgeCount() const { return static_cast<int>(m_edges.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Set the node set: each map needs `id` (+ optional `kind`/`label` and arbitrary passthrough).
    Q_INVOKABLE void setNodes(const QVariantList& nodes);
    // Set the edge set: each map needs `source`/`target` (+ passthrough, e.g. edgeType/provenance/
    // sinkKind, read by the view's edge styler).
    Q_INVOKABLE void setEdges(const QVariantList& edges);
    // Edges with endpoint coords in [0,1]: [{ ...passthrough, source, target, x1, y1, x2, y2 }].
    [[nodiscard]] Q_INVOKABLE QVariantList edges() const;
    // The node map for `id` (empty if unknown).
    [[nodiscard]] Q_INVOKABLE QVariantMap nodeById(const QString& id) const;
    // Neighbour rows for `id`: [{ id, label, kind, edgeType }].
    [[nodiscard]] Q_INVOKABLE QVariantList neighborsOf(const QString& id) const;

signals:
    void selectionChanged();
    void graphChanged();

private:
    void rebuild();  // re-index + degree + relayout + reset
    void relayout(); // Fruchterman-Reingold over the current node/edge sets

    QList<QVariantMap> m_nodes;
    QList<QVariantMap> m_edges;
    QHash<QString, int> m_index;   // node id -> row
    QHash<QString, QPointF> m_pos; // node id -> normalized position
    QHash<QString, int> m_degree;  // node id -> degree
    QString m_selectedId;
};

} // namespace daemongraph
