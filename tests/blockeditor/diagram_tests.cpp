#include "diagram/engine/diagram_engine.h"
#include "diagram/geometry/path_mesh.h"
#include "diagram/layout/layered_layout.h"
#include "diagram/parser/detect.h"
#include "diagram/parser/flowchart_parser.h"

#include <QtTest/QtTest>

using namespace be::diagram;

namespace {

const DiagramEdge* findEdge(const DiagramModel& m, const QString& from, const QString& to) {
    for (const DiagramEdge& e : m.edges) {
        if (e.fromId == from && e.toId == to) {
            return &e;
        }
    }
    return nullptr;
}

} // namespace

class DiagramTests : public QObject {
    Q_OBJECT

private slots:
    void detectsFamily();
    void parsesNodesAndShapes();
    void parsesEdgeKindsAndLabels();
    void parsesSubgraphsAndClassDef();
    void layoutAssignsRanks();
    void longEdgeGetsBends();
    void brandesKopfAlignsChain();
    void clusterContainsAndIsContiguous();
    void curveBasisIsMonotonic();
    void edgeLabelReservesSpace();
    void engineProducesGeometry();
    void engineReportsErrors();
};

void DiagramTests::detectsFamily() {
    QCOMPARE(detectFamily(QStringLiteral("flowchart TD\nA-->B")), QStringLiteral("flowchart"));
    QCOMPARE(detectFamily(QStringLiteral("graph LR\nA-->B")), QStringLiteral("flowchart"));
    QCOMPARE(detectFamily(QStringLiteral("sequenceDiagram\nA->>B: hi")), QString());
    QCOMPARE(detectFamily(QStringLiteral("   %% comment\nflowchart TD")),
             QStringLiteral("flowchart"));
}

void DiagramTests::parsesNodesAndShapes() {
    const DiagramModel m = parseFlowchart(QStringLiteral("flowchart TD\n"
                                                         "A[Rect]\n"
                                                         "B(Round)\n"
                                                         "C{Diamond}\n"
                                                         "D((Circle))\n"
                                                         "E{{Hex}}\n"));
    QVERIFY(m.valid);
    QCOMPARE(m.dir, Direction::TB);
    QCOMPARE(m.nodes.size(), 5);
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("A"))].shape, NodeShape::Rect);
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("B"))].shape, NodeShape::RoundRect);
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("C"))].shape, NodeShape::Diamond);
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("D"))].shape, NodeShape::Circle);
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("E"))].shape, NodeShape::Hexagon);
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("A"))].label, QStringLiteral("Rect"));
}

void DiagramTests::parsesEdgeKindsAndLabels() {
    const DiagramModel m = parseFlowchart(QStringLiteral("flowchart LR\n"
                                                         "A --> B\n"
                                                         "B -.-> C\n"
                                                         "C ==> D\n"
                                                         "A -->|yes| D\n"
                                                         "D -- maybe --> A\n"));
    QVERIFY(m.valid);
    QCOMPARE(m.dir, Direction::LR);
    QCOMPARE(m.edges.size(), 5);
    QCOMPARE(m.edges[0].kind, EdgeKind::Normal);
    QCOMPARE(m.edges[0].head, ArrowHead::Arrow);
    QCOMPARE(m.edges[1].kind, EdgeKind::Dotted);
    QCOMPARE(m.edges[2].kind, EdgeKind::Thick);
    QCOMPARE(m.edges[3].label, QStringLiteral("yes"));
    QCOMPARE(m.edges[4].label, QStringLiteral("maybe"));
}

void DiagramTests::parsesSubgraphsAndClassDef() {
    const DiagramModel m = parseFlowchart(QStringLiteral("flowchart TD\n"
                                                         "classDef hot fill:#f00,stroke:#900\n"
                                                         "subgraph G [Group]\n"
                                                         "A --> B\n"
                                                         "end\n"
                                                         "class A hot\n"
                                                         "C:::hot --> A\n"));
    QVERIFY(m.valid);
    QCOMPARE(m.clusters.size(), 1);
    QCOMPARE(m.clusters[0].title, QStringLiteral("Group"));
    QVERIFY(m.clusters[0].memberIds.contains(QStringLiteral("A")));
    QVERIFY(m.classDefs.contains(QStringLiteral("hot")));
    QCOMPARE(m.classDefs[QStringLiteral("hot")].value(QStringLiteral("fill")),
             QStringLiteral("#f00"));
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("A"))].classRef, QStringLiteral("hot"));
    QCOMPARE(m.nodes[m.indexOf(QStringLiteral("C"))].classRef, QStringLiteral("hot"));
}

void DiagramTests::layoutAssignsRanks() {
    DiagramEngine engine;
    const DiagramModel m =
        engine.buildModel(QStringLiteral("flowchart TD\nA-->B\nB-->C\nA-->C\n"), Style{}, 600.0);
    QVERIFY(m.valid);
    const int a = m.indexOf(QStringLiteral("A"));
    const int b = m.indexOf(QStringLiteral("B"));
    const int c = m.indexOf(QStringLiteral("C"));
    QCOMPARE(m.nodes[a].rank, 0);
    QCOMPARE(m.nodes[b].rank, 1);
    QCOMPARE(m.nodes[c].rank, 2);
    // Sizes and centers assigned.
    QVERIFY(m.nodes[a].width > 0.0);
    QVERIFY(m.nodes[c].y > m.nodes[a].y); // TB: later ranks are lower
}

void DiagramTests::longEdgeGetsBends() {
    DiagramEngine engine;
    // A-->C skips a rank, so the layout must route it through dummy bends.
    const DiagramModel m =
        engine.buildModel(QStringLiteral("flowchart TD\nA-->B\nB-->C\nA-->C\n"), Style{}, 600.0);
    QVERIFY(m.valid);
    const DiagramEdge* ac = findEdge(m, QStringLiteral("A"), QStringLiteral("C"));
    QVERIFY(ac);
    QVERIFY2(ac->points.size() > 2, "long edge should have interior bend points");
    const DiagramEdge* ab = findEdge(m, QStringLiteral("A"), QStringLiteral("B"));
    QVERIFY(ab);
    QVERIFY(ab->points.size() >= 2);
}

void DiagramTests::brandesKopfAlignsChain() {
    DiagramEngine engine;
    const DiagramModel m =
        engine.buildModel(QStringLiteral("flowchart TD\nA-->B\nB-->C\nC-->D\n"), Style{}, 600.0);
    QVERIFY(m.valid);
    const qreal xa = m.nodes[m.indexOf(QStringLiteral("A"))].x;
    for (const char* id : {"B", "C", "D"}) {
        const qreal x = m.nodes[m.indexOf(QString::fromLatin1(id))].x;
        QVERIFY2(std::abs(x - xa) < 1.0, "a straight chain should be vertically aligned");
    }
}

void DiagramTests::clusterContainsAndIsContiguous() {
    DiagramEngine engine;
    const DiagramModel m = engine.buildModel(QStringLiteral("flowchart TD\n"
                                                            "subgraph G [Grp]\n"
                                                            "g1\n"
                                                            "g2\n"
                                                            "end\n"
                                                            "g1 --> x\n"
                                                            "g2 --> x\n"
                                                            "n --> x\n"),
                                             Style{}, 600.0);
    QVERIFY(m.valid);
    QCOMPARE(m.clusters.size(), 1);
    const DiagramCluster& g = m.clusters[0];
    QVERIFY(g.bounds.isValid());

    // Cluster bounds contain its members.
    for (const char* id : {"g1", "g2"}) {
        const DiagramNode& node = m.nodes[m.indexOf(QString::fromLatin1(id))];
        const QRectF r(node.x - node.width / 2.0, node.y - node.height / 2.0, node.width,
                       node.height);
        QVERIFY2(g.bounds.contains(r), "cluster bounds must contain its member");
    }

    // g1/g2 share rank 0 with the foreign node n; the two members must remain
    // adjacent (contiguous) rather than have n placed between them.
    const DiagramNode& g1 = m.nodes[m.indexOf(QStringLiteral("g1"))];
    const DiagramNode& g2 = m.nodes[m.indexOf(QStringLiteral("g2"))];
    const DiagramNode& n = m.nodes[m.indexOf(QStringLiteral("n"))];
    QCOMPARE(g1.rank, g2.rank);
    QCOMPARE(n.rank, g1.rank);
    QCOMPARE(std::abs(g1.order - g2.order), 1);
    QVERIFY2(n.order < std::min(g1.order, g2.order) || n.order > std::max(g1.order, g2.order),
             "foreign node must not split the cluster");
}

void DiagramTests::curveBasisIsMonotonic() {
    const QVector<QPointF> knots{{0, 0}, {10, 0}, {20, 0}, {30, 0}};
    const QVector<QPointF> dense = sampleCurveBasis(knots);
    QVERIFY(dense.size() > knots.size());
    QCOMPARE(dense.first(), knots.first());
    QCOMPARE(dense.last(), knots.last());
    for (int i = 1; i < dense.size(); ++i) {
        QVERIFY2(dense[i].x() >= dense[i - 1].x() - 1e-6, "x should advance monotonically");
        QVERIFY2(std::abs(dense[i].y()) < 1e-6, "collinear knots stay on the line");
    }
}

void DiagramTests::edgeLabelReservesSpace() {
    DiagramEngine engine;
    // Two edges meet at C; one carries a wide label whose dummy occupies the
    // intermediate rank and must push the siblings apart.
    const DiagramModel plain =
        engine.buildModel(QStringLiteral("flowchart TD\nA-->C\nB-->C\n"), Style{}, 600.0);
    const DiagramModel labeled = engine.buildModel(
        QStringLiteral("flowchart TD\nA-->|a very wide edge label|C\nB-->C\n"), Style{}, 600.0);
    QVERIFY(plain.valid && labeled.valid);

    const auto width = [](const DiagramModel& m) {
        qreal lo = 1e9;
        qreal hi = -1e9;
        for (const DiagramNode& node : m.nodes) {
            lo = std::min(lo, node.x - node.width / 2.0);
            hi = std::max(hi, node.x + node.width / 2.0);
        }
        return hi - lo;
    };
    QVERIFY2(width(labeled) > width(plain) + 1.0, "wide edge label should widen the layout");
}

void DiagramTests::engineProducesGeometry() {
    DiagramEngine engine;
    const auto snap = engine.buildSnapshot(
        QStringLiteral("flowchart TD\nA[Start] --> B{Choice}\nB -->|yes| C[Done]\n"), Style{},
        600.0, 1);
    QVERIFY(snap);
    QVERIFY(!snap->hasError);
    QVERIFY(!snap->nodeFills.vertices.isEmpty());
    QVERIFY(!snap->nodeFills.indices.isEmpty());
    QVERIFY(!snap->nodeBorders.indices.isEmpty());
    QVERIFY(!snap->edges.indices.isEmpty());
    QVERIFY(!snap->arrowheads.indices.isEmpty());
    QVERIFY(snap->labels.size() >= 3);
    QVERIFY(snap->hits.size() >= 3);
    QVERIFY(snap->diagramBounds.isValid());
    QCOMPARE(snap->contentRevision, quint64(1));
}

void DiagramTests::engineReportsErrors() {
    DiagramEngine engine;
    const auto empty = engine.buildSnapshot(QStringLiteral("not a diagram"), Style{}, 600.0, 2);
    QVERIFY(empty);
    QVERIFY(empty->hasError);

    const auto noNodes = engine.buildSnapshot(QStringLiteral("flowchart TD\n"), Style{}, 600.0, 3);
    QVERIFY(noNodes);
    QVERIFY(noNodes->hasError);
}

QTEST_MAIN(DiagramTests)
#include "diagram_tests.moc"
