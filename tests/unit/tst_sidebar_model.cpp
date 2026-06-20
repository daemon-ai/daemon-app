#include "persistence/in_memory_conversation_store.h"
#include "sidebar_model.h"

#include <QSignalSpy>
#include <QtTest>

using persistence::InMemoryConversationStore;

// Exercises the flattened agent-tree sidebar model: recursive flattening to
// arbitrary depth, the per-row tree roles, expand/collapse, and scope selection.
class TestSidebarModel : public QObject {
    Q_OBJECT

private:
    static int findRow(const SidebarModel& m, const QString& label)
    {
        for (int i = 0; i < m.rowCount(); ++i) {
            if (m.data(m.index(i, 0), SidebarModel::LabelRole).toString() == label) {
                return i;
            }
        }
        return -1;
    }

    template <typename T>
    static T roleAt(const SidebarModel& m, int row, SidebarModel::Role role)
    {
        return m.data(m.index(row, 0), role).value<T>();
    }

private slots:
    // The whole fleet-of-fleets is flattened with correct depths; nesting is
    // unbounded (worker sits three levels under its root).
    void flattensTreeWithDepths()
    {
        InMemoryConversationStore store;
        SidebarModel model;
        model.setStore(&store);

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        const int build = findRow(model, QStringLiteral("Build Fleet"));
        const int coder = findRow(model, QStringLiteral("Coder"));
        const int worker = findRow(model, QStringLiteral("Worker A"));
        QVERIFY(acme >= 0 && build >= 0 && coder >= 0 && worker >= 0);

        QCOMPARE(roleAt<int>(model, acme, SidebarModel::DepthRole), 0);
        QCOMPARE(roleAt<int>(model, build, SidebarModel::DepthRole), 1);
        QCOMPARE(roleAt<int>(model, coder, SidebarModel::DepthRole), 2);
        QCOMPARE(roleAt<int>(model, worker, SidebarModel::DepthRole), 3);
    }

    // Tree roles are cosmetic-kind + structural flags; orchestrators carry
    // children, leaves do not.
    void exposesTreeRoles()
    {
        InMemoryConversationStore store;
        SidebarModel model;
        model.setStore(&store);

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        const int coder = findRow(model, QStringLiteral("Coder"));

        // kind 2 == Orchestrator, kind 0 == Engine; nodeType 4 == Node.
        QCOMPARE(roleAt<int>(model, acme, SidebarModel::KindRole), 2);
        QCOMPARE(roleAt<int>(model, acme, SidebarModel::NodeTypeRole), 4);
        QVERIFY(roleAt<bool>(model, acme, SidebarModel::HasChildrenRole));
        QCOMPARE(roleAt<QString>(model, acme, SidebarModel::AgentIdRole), QStringLiteral("n-acme"));

        QCOMPARE(roleAt<int>(model, coder, SidebarModel::KindRole), 0);
        QVERIFY(!roleAt<bool>(model, coder, SidebarModel::HasChildrenRole));
    }

    // Collapsing a node hides its whole subtree; expanding restores it.
    void toggleExpandHidesAndRestoresSubtree()
    {
        InMemoryConversationStore store;
        SidebarModel model;
        model.setStore(&store);

        const int before = model.rowCount();
        const int build = findRow(model, QStringLiteral("Build Fleet"));
        QVERIFY(build >= 0);
        QVERIFY(roleAt<bool>(model, build, SidebarModel::ExpandedRole));

        model.toggleExpand(build);
        // Build's subtree (coder, review, deep, worker) is gone.
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Worker A")) < 0);
        QCOMPARE(model.rowCount(), before - 4);
        QVERIFY(!roleAt<bool>(model, findRow(model, QStringLiteral("Build Fleet")),
                              SidebarModel::ExpandedRole));

        model.toggleExpand(findRow(model, QStringLiteral("Build Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Coder")) >= 0);
        QCOMPARE(model.rowCount(), before);
    }

    // Activating a node row selects its subtree scope (nodeType Node, the
    // agent id carried through).
    void activateEmitsNodeScope()
    {
        InMemoryConversationStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        model.activate(acme);

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 4); // NodeType::Node
        QCOMPARE(args.at(2).toString(), QStringLiteral("n-acme"));
    }

    // Separator rows (Fleet / Tags headers) are not selectable and emit nothing.
    void separatorsAreNotSelectable()
    {
        InMemoryConversationStore store;
        SidebarModel model;
        model.setStore(&store);

        const int fleet = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleet >= 0);
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::IsSeparatorRole));

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.activate(fleet);
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(TestSidebarModel)
#include "tst_sidebar_model.moc"
