#include "graph_model.h"

#include <algorithm>
#include <cmath>
#include <QtGlobal>
#include <QtMath>

namespace daemongraph {

namespace {
const QString kId = QStringLiteral("id");
const QString kKind = QStringLiteral("kind");
const QString kLabel = QStringLiteral("label");
const QString kSource = QStringLiteral("source");
const QString kTarget = QStringLiteral("target");
} // namespace

GraphModel::GraphModel(QObject* parent) : QAbstractListModel(parent) {}

void GraphModel::setSelectedId(const QString& id) {
    if (m_selectedId == id) {
        return;
    }
    m_selectedId = id;
    emit selectionChanged();
    if (!m_nodes.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_nodes.size()) - 1), {SelectedRole});
    }
}

int GraphModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_nodes.size());
}

QVariant GraphModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_nodes.size()) {
        return {};
    }
    const QVariantMap& n = m_nodes.at(index.row());
    const QString id = n.value(kId).toString();
    switch (role) {
    case IdRole:
        return id;
    case KindRole:
        return n.value(kKind);
    case Qt::DisplayRole:
    case LabelRole:
        return n.value(kLabel, id);
    case XRole:
        return m_pos.value(id, QPointF(0.5, 0.5)).x();
    case YRole:
        return m_pos.value(id, QPointF(0.5, 0.5)).y();
    case DegreeRole:
        return m_degree.value(id, 0);
    case SelectedRole:
        return id == m_selectedId;
    case DataRole:
        return n;
    default:
        return {};
    }
}

QHash<int, QByteArray> GraphModel::roleNames() const {
    return {
        {IdRole, "nodeId"},
        {KindRole, "nodeKind"},
        {LabelRole, "label"},
        {XRole, "nx"},
        {YRole, "ny"},
        {DegreeRole, "degree"},
        {SelectedRole, "selected"},
        {DataRole, "nodeData"},
    };
}

void GraphModel::setNodes(const QVariantList& nodes) {
    m_nodes.clear();
    m_nodes.reserve(nodes.size());
    for (const QVariant& v : nodes) {
        m_nodes.append(v.toMap());
    }
    rebuild();
}

void GraphModel::setEdges(const QVariantList& edges) {
    m_edges.clear();
    m_edges.reserve(edges.size());
    for (const QVariant& v : edges) {
        m_edges.append(v.toMap());
    }
    rebuild();
}

void GraphModel::rebuild() {
    beginResetModel();
    m_index.clear();
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_index.insert(m_nodes.at(i).value(kId).toString(), i);
    }
    m_degree.clear();
    for (const QVariantMap& e : m_edges) {
        m_degree[e.value(kSource).toString()] += 1;
        m_degree[e.value(kTarget).toString()] += 1;
    }
    relayout();
    endResetModel();
    emit graphChanged();
}

QVariantList GraphModel::edges() const {
    QVariantList out;
    out.reserve(static_cast<int>(m_edges.size()));
    for (const QVariantMap& e : m_edges) {
        const QPointF a = m_pos.value(e.value(kSource).toString(), QPointF(0.5, 0.5));
        const QPointF b = m_pos.value(e.value(kTarget).toString(), QPointF(0.5, 0.5));
        QVariantMap row = e;
        row[QStringLiteral("x1")] = a.x();
        row[QStringLiteral("y1")] = a.y();
        row[QStringLiteral("x2")] = b.x();
        row[QStringLiteral("y2")] = b.y();
        out.append(row);
    }
    return out;
}

QVariantMap GraphModel::nodeById(const QString& id) const {
    const int row = m_index.value(id, -1);
    return row >= 0 ? m_nodes.at(row) : QVariantMap{};
}

QVariantList GraphModel::neighborsOf(const QString& id) const {
    QVariantList out;
    for (const QVariantMap& e : m_edges) {
        const QString src = e.value(kSource).toString();
        const QString dst = e.value(kTarget).toString();
        QString other;
        if (src == id) {
            other = dst;
        } else if (dst == id) {
            other = src;
        } else {
            continue;
        }
        const QVariantMap n = nodeById(other);
        out.append(QVariantMap{
            {QStringLiteral("id"), other},
            {QStringLiteral("label"), n.value(kLabel, other)},
            {QStringLiteral("kind"), n.value(kKind)},
            {QStringLiteral("edgeType"), e.value(QStringLiteral("edgeType"))},
        });
    }
    return out;
}

// Fruchterman-Reingold layout in a unit square; positions normalized to [0,1].
void GraphModel::relayout() {
    const int n = static_cast<int>(m_nodes.size());
    m_pos.clear();
    if (n == 0) {
        return;
    }
    if (n == 1) {
        m_pos.insert(m_nodes.first().value(kId).toString(), QPointF(0.5, 0.5));
        return;
    }

    QList<QString> ids;
    ids.reserve(n);
    QList<QPointF> p;
    p.reserve(n);
    for (int i = 0; i < n; ++i) {
        ids.append(m_nodes.at(i).value(kId).toString());
        const double a = 2.0 * M_PI * i / n;
        p.append(QPointF(0.5 + 0.35 * std::cos(a), 0.5 + 0.35 * std::sin(a)));
    }

    struct E {
        int a;
        int b;
    };
    QList<E> es;
    es.reserve(static_cast<int>(m_edges.size()));
    for (const QVariantMap& e : m_edges) {
        const int a = m_index.value(e.value(kSource).toString(), -1);
        const int b = m_index.value(e.value(kTarget).toString(), -1);
        if (a >= 0 && b >= 0) {
            es.append(E{a, b});
        }
    }

    const double k = std::sqrt(1.0 / n) * 0.85;
    double temp = 0.10;
    constexpr int kIterations = 200;
    QList<QPointF> disp(n);
    for (int it = 0; it < kIterations; ++it) {
        for (int i = 0; i < n; ++i) {
            disp[i] = QPointF(0.0, 0.0);
        }
        // Repulsion (all pairs).
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                QPointF delta = p[i] - p[j];
                double dist = std::hypot(delta.x(), delta.y());
                if (dist < 1e-4) {
                    delta = QPointF((i % 2 ? 1e-3 : -1e-3), 1e-3);
                    dist = 1e-3;
                }
                const double force = (k * k) / dist;
                const QPointF dir = delta / dist;
                disp[i] += dir * force;
                disp[j] -= dir * force;
            }
        }
        // Attraction (along edges).
        for (const E& e : es) {
            QPointF delta = p[e.a] - p[e.b];
            double dist = std::hypot(delta.x(), delta.y());
            if (dist < 1e-4) {
                dist = 1e-4;
            }
            const double force = (dist * dist) / k;
            const QPointF dir = delta / dist;
            disp[e.a] -= dir * force;
            disp[e.b] += dir * force;
        }
        // Apply with temperature cap, keep inside [0.05, 0.95].
        for (int i = 0; i < n; ++i) {
            const double dlen = std::hypot(disp[i].x(), disp[i].y());
            if (dlen < 1e-6) {
                continue;
            }
            const QPointF step = disp[i] / dlen * std::min(dlen, temp);
            p[i] += step;
            p[i].setX(qBound(0.05, p[i].x(), 0.95));
            p[i].setY(qBound(0.05, p[i].y(), 0.95));
        }
        temp *= 0.97;
    }

    for (int i = 0; i < n; ++i) {
        m_pos.insert(ids[i], p[i]);
    }
}

} // namespace daemongraph
