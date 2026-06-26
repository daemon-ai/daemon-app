#include "memory_graph_model.h"

#include "memory/imemory_service.h"

#include <algorithm>
#include <QtGlobal>
#include <QtMath>

namespace memoryui {

MemoryGraphModel::MemoryGraphModel(QObject* parent) : QAbstractListModel(parent) {
    memory::registerMemoryMetatypes();
}

QObject* MemoryGraphModel::service() const {
    return m_service;
}

void MemoryGraphModel::setService(QObject* service) {
    auto* svc = qobject_cast<memory::IMemoryService*>(service);
    if (svc == m_service)
        return;
    if (m_service)
        m_service->disconnect(this);
    m_service = svc;
    emit serviceChanged();
    if (m_service) {
        connect(m_service, &memory::IMemoryService::graphReady, this,
                &MemoryGraphModel::onGraphReady);
        connect(m_service, &memory::IMemoryService::scopeChanged, this,
                &MemoryGraphModel::onScopeChanged);
        refresh();
    }
}

void MemoryGraphModel::setKind(const QString& kind) {
    if (m_kind == kind)
        return;
    m_kind = kind;
    m_seed.clear();
    emit kindChanged();
    emit seedChanged();
    refresh();
}

void MemoryGraphModel::setSeed(const QString& seed) {
    if (m_seed == seed)
        return;
    m_seed = seed;
    emit seedChanged();
    refresh();
}

void MemoryGraphModel::setDepth(int depth) {
    depth = qBound(1, depth, 5);
    if (m_depth == depth)
        return;
    m_depth = depth;
    emit depthChanged();
    refresh();
}

void MemoryGraphModel::setSelectedId(const QString& id) {
    if (m_selectedId == id)
        return;
    m_selectedId = id;
    emit selectionChanged();
    if (!m_graph.nodes.isEmpty())
        emit dataChanged(index(0), index(static_cast<int>(m_graph.nodes.size()) - 1),
                         {SelectedRole});
}

int MemoryGraphModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_graph.nodes.size());
}

QVariant MemoryGraphModel::data(const QModelIndex& index, int role) const {
    if (index.row() < 0 || index.row() >= m_graph.nodes.size())
        return {};
    const memory::MemoryNode& n = m_graph.nodes.at(index.row());
    switch (role) {
    case IdRole:
        return n.id;
    case KindRole:
        return n.kind;
    case Qt::DisplayRole:
    case LabelRole:
        return n.label;
    case WeightRole:
        return n.weight;
    case XRole:
        return m_pos.value(n.id, QPointF(0.5, 0.5)).x();
    case YRole:
        return m_pos.value(n.id, QPointF(0.5, 0.5)).y();
    case SelectedRole:
        return n.id == m_selectedId;
    case DegreeRole:
        return m_degree.value(n.id, 0);
    default:
        return {};
    }
}

QHash<int, QByteArray> MemoryGraphModel::roleNames() const {
    return {
        {IdRole, "nodeId"},         {KindRole, "nodeKind"}, {LabelRole, "label"},
        {WeightRole, "weight"},     {XRole, "nx"},          {YRole, "ny"},
        {SelectedRole, "selected"}, {DegreeRole, "degree"},
    };
}

QVariantList MemoryGraphModel::edges() const {
    QVariantList out;
    out.reserve(m_graph.edges.size());
    for (const memory::MemoryEdge& e : m_graph.edges) {
        const QPointF a = m_pos.value(e.source, QPointF(0.5, 0.5));
        const QPointF b = m_pos.value(e.target, QPointF(0.5, 0.5));
        out.append(QVariantMap{
            {QStringLiteral("source"), e.source},
            {QStringLiteral("target"), e.target},
            {QStringLiteral("edgeType"), e.edgeType},
            {QStringLiteral("label"), e.label},
            {QStringLiteral("weight"), e.weight},
            {QStringLiteral("x1"), a.x()},
            {QStringLiteral("y1"), a.y()},
            {QStringLiteral("x2"), b.x()},
            {QStringLiteral("y2"), b.y()},
        });
    }
    return out;
}

QVariantList MemoryGraphModel::neighborsOf(const QString& id) const {
    QHash<QString, memory::MemoryNode> byId;
    for (const memory::MemoryNode& n : m_graph.nodes)
        byId.insert(n.id, n);
    QVariantList out;
    for (const memory::MemoryEdge& e : m_graph.edges) {
        QString other;
        if (e.source == id)
            other = e.target;
        else if (e.target == id)
            other = e.source;
        else
            continue;
        const memory::MemoryNode n = byId.value(other);
        out.append(QVariantMap{
            {QStringLiteral("id"), other},
            {QStringLiteral("label"), n.label.isEmpty() ? other : n.label},
            {QStringLiteral("kind"), n.kind},
            {QStringLiteral("edgeType"), e.edgeType},
            {QStringLiteral("edgeLabel"), e.label},
        });
    }
    return out;
}

QVariantMap MemoryGraphModel::nodeById(const QString& id) const {
    for (const memory::MemoryNode& n : m_graph.nodes) {
        if (n.id == id) {
            QVariantMap m{
                {QStringLiteral("id"), n.id},
                {QStringLiteral("kind"), n.kind},
                {QStringLiteral("label"), n.label},
                {QStringLiteral("weight"), n.weight},
                {QStringLiteral("degree"), m_degree.value(n.id, 0)},
            };
            for (auto it = n.meta.constBegin(); it != n.meta.constEnd(); ++it)
                m.insert(it.key(), it.value());
            return m;
        }
    }
    return {};
}

void MemoryGraphModel::refresh() {
    if (m_service)
        m_service->requestGraph(m_kind, m_seed, m_depth, 200);
}

void MemoryGraphModel::expand(const QString& id) {
    m_seed = id;
    m_depth = qBound(1, m_depth + 1, 5);
    emit seedChanged();
    emit depthChanged();
    refresh();
}

void MemoryGraphModel::resetFocus() {
    if (m_seed.isEmpty() && m_depth == 1)
        return;
    m_seed.clear();
    m_depth = 1;
    emit seedChanged();
    emit depthChanged();
    refresh();
}

void MemoryGraphModel::onGraphReady(const memory::MemoryGraph& graph) {
    beginResetModel();
    m_graph = graph;
    m_degree.clear();
    for (const memory::MemoryEdge& e : m_graph.edges) {
        m_degree[e.source] += 1;
        m_degree[e.target] += 1;
    }
    layout();
    endResetModel();
    emit graphChanged();
}

void MemoryGraphModel::onScopeChanged() {
    refresh();
}

// Fruchterman-Reingold layout in a unit square; positions normalized to [0,1].
void MemoryGraphModel::layout() {
    const int n = static_cast<int>(m_graph.nodes.size());
    m_pos.clear();
    if (n == 0)
        return;
    if (n == 1) {
        m_pos.insert(m_graph.nodes.first().id, QPointF(0.5, 0.5));
        return;
    }

    QList<QString> ids;
    ids.reserve(n);
    QHash<QString, int> idx;
    QList<QPointF> p;
    p.reserve(n);
    for (int i = 0; i < n; ++i) {
        const QString& id = m_graph.nodes.at(i).id;
        ids.append(id);
        idx.insert(id, i);
        // Seed on a circle to avoid degenerate all-equal start.
        const double a = 2.0 * M_PI * i / n;
        p.append(QPointF(0.5 + (0.35 * std::cos(a)), 0.5 + (0.35 * std::sin(a))));
    }

    struct E {
        int a;
        int b;
    };
    QList<E> es;
    es.reserve(m_graph.edges.size());
    for (const memory::MemoryEdge& e : m_graph.edges) {
        if (idx.contains(e.source) && idx.contains(e.target))
            es.append(E{idx.value(e.source), idx.value(e.target)});
    }

    const double area = 1.0;
    const double k = std::sqrt(area / n) * 0.85;
    double temp = 0.10;
    const int iterations = 200;

    QList<QPointF> disp(n);
    for (int it = 0; it < iterations; ++it) {
        for (int i = 0; i < n; ++i)
            disp[i] = QPointF(0.0, 0.0);

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
            dist = std::max(dist, 1e-4);
            const double force = (dist * dist) / k;
            const QPointF dir = delta / dist;
            disp[e.a] -= dir * force;
            disp[e.b] += dir * force;
        }
        // Apply with temperature cap, keep inside [0.05, 0.95].
        for (int i = 0; i < n; ++i) {
            double dlen = std::hypot(disp[i].x(), disp[i].y());
            if (dlen < 1e-6)
                continue;
            const QPointF step = disp[i] / dlen * std::min(dlen, temp);
            p[i] += step;
            p[i].setX(qBound(0.05, p[i].x(), 0.95));
            p[i].setY(qBound(0.05, p[i].y(), 0.95));
        }
        temp *= 0.97;
    }

    for (int i = 0; i < n; ++i)
        m_pos.insert(ids[i], p[i]);
}

} // namespace memoryui
