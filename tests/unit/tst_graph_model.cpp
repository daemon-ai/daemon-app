#include "graph_model.h"

#include <QtTest/QtTest>

using daemongraph::GraphModel;

// The reusable Daemon Kit graph model: generic node/edge ingestion, degree, a normalized force
// layout, and the edges()/nodeById/neighborsOf accessors.
class TestGraphModel : public QObject {
    Q_OBJECT

    static QVariantMap node(const QString& id, const QString& kind = {})
    {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        if (!kind.isEmpty()) {
            m[QStringLiteral("kind")] = kind;
        }
        return m;
    }
    static QVariantMap edge(const QString& s, const QString& t, const QString& type = {})
    {
        QVariantMap m;
        m[QStringLiteral("source")] = s;
        m[QStringLiteral("target")] = t;
        if (!type.isEmpty()) {
            m[QStringLiteral("edgeType")] = type;
        }
        return m;
    }
    static int rowOf(const GraphModel& m, const QString& id)
    {
        for (int i = 0; i < m.rowCount(); ++i) {
            if (m.data(m.index(i), GraphModel::IdRole).toString() == id) {
                return i;
            }
        }
        return -1;
    }

private slots:
    void ingestsNodesAndEdges()
    {
        GraphModel m;
        m.setNodes({ node("a", "agent"), node("b", "session"), node("c", "origin") });
        m.setEdges({ edge("a", "b", "runs"), edge("c", "b", "inbound") });
        QCOMPARE(m.nodeCount(), 3);
        QCOMPARE(m.edgeCount(), 2);
        QCOMPARE(m.rowCount(), 3);
    }

    void degreeReflectsEdges()
    {
        GraphModel m;
        m.setNodes({ node("a"), node("b"), node("c") });
        m.setEdges({ edge("a", "b"), edge("c", "b") });
        QCOMPARE(m.data(m.index(rowOf(m, "b")), GraphModel::DegreeRole).toInt(), 2);
        QCOMPARE(m.data(m.index(rowOf(m, "a")), GraphModel::DegreeRole).toInt(), 1);
    }

    void layoutIsNormalizedInBounds()
    {
        GraphModel m;
        m.setNodes({ node("a"), node("b"), node("c"), node("d") });
        m.setEdges({ edge("a", "b"), edge("b", "c"), edge("c", "d") });
        for (int i = 0; i < m.rowCount(); ++i) {
            const double x = m.data(m.index(i), GraphModel::XRole).toDouble();
            const double y = m.data(m.index(i), GraphModel::YRole).toDouble();
            QVERIFY(x >= 0.0 && x <= 1.0);
            QVERIFY(y >= 0.0 && y <= 1.0);
        }
    }

    void edgesCarryEndpointCoordsAndPassthrough()
    {
        GraphModel m;
        m.setNodes({ node("a"), node("b") });
        m.setEdges({ edge("a", "b", "inbound") });
        const QVariantList es = m.edges();
        QCOMPARE(es.size(), 1);
        const QVariantMap e = es.first().toMap();
        QCOMPARE(e.value(QStringLiteral("edgeType")).toString(), QStringLiteral("inbound"));
        QVERIFY(e.contains(QStringLiteral("x1")) && e.contains(QStringLiteral("y2")));
    }

    void nodeByIdAndNeighbors()
    {
        GraphModel m;
        m.setNodes({ node("a", "agent"), node("b", "session") });
        m.setEdges({ edge("a", "b", "runs") });
        QCOMPARE(m.nodeById(QStringLiteral("a")).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("agent"));
        QVERIFY(m.nodeById(QStringLiteral("nope")).isEmpty());
        const QVariantList nb = m.neighborsOf(QStringLiteral("a"));
        QCOMPARE(nb.size(), 1);
        QCOMPARE(nb.first().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("b"));
    }

    void selectionRoundTrips()
    {
        GraphModel m;
        m.setNodes({ node("a"), node("b") });
        m.setSelectedId(QStringLiteral("b"));
        QCOMPARE(m.selectedId(), QStringLiteral("b"));
        QVERIFY(m.data(m.index(rowOf(m, "b")), GraphModel::SelectedRole).toBool());
        QVERIFY(!m.data(m.index(rowOf(m, "a")), GraphModel::SelectedRole).toBool());
    }
};

QTEST_GUILESS_MAIN(TestGraphModel)

#include "tst_graph_model.moc"
