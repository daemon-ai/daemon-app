#include "persistence/in_memory_session_store.h"
#include "sidebar_model.h"

#include <QSignalSpy>
#include <QtTest>

using persistence::InMemorySessionStore;

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
        InMemorySessionStore store;
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
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        const int coder = findRow(model, QStringLiteral("Coder"));

        // kind 2 == Orchestrator, kind 0 == Engine; nodeType 4 == Node.
        QCOMPARE(roleAt<int>(model, acme, SidebarModel::KindRole), 2);
        QCOMPARE(roleAt<int>(model, acme, SidebarModel::NodeTypeRole), 4);
        QVERIFY(roleAt<bool>(model, acme, SidebarModel::HasChildrenRole));
        QCOMPARE(roleAt<QString>(model, acme, SidebarModel::UnitIdRole), QStringLiteral("n-acme"));

        QCOMPARE(roleAt<int>(model, coder, SidebarModel::KindRole), 0);
        QVERIFY(!roleAt<bool>(model, coder, SidebarModel::HasChildrenRole));
    }

    // Collapsing a node hides its whole subtree; expanding restores it.
    void toggleExpandHidesAndRestoresSubtree()
    {
        InMemorySessionStore store;
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
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        model.activate(acme);

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 4); // NodeType::Unit
        QCOMPARE(args.at(2).toString(), QStringLiteral("n-acme"));
    }

    // Separator rows (Fleet / Tags headers) are not selectable and emit nothing.
    void separatorsAreNotSelectable()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int fleet = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleet >= 0);
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::IsSeparatorRole));

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.activate(fleet);
        QCOMPARE(spy.count(), 0);
    }

    // The highlight is identity-based: it follows the node across a model reset
    // rather than sticking to a row index. Collapsing a node that does NOT
    // contain the selection rebuilds the list but must leave Coder highlighted.
    void currentRoleTracksIdentityAcrossRebuild()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int coder = findRow(model, QStringLiteral("Coder"));
        model.activate(coder);
        QVERIFY(roleAt<bool>(model, coder, SidebarModel::CurrentRole));

        // Deep Fleet is a sibling branch under Build, not an ancestor of Coder,
        // so collapsing it triggers a full rebuild without hiding Coder.
        model.toggleExpand(findRow(model, QStringLiteral("Deep Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Worker A")) < 0); // the rebuild happened
        const int coderAfter = findRow(model, QStringLiteral("Coder"));
        QVERIFY(coderAfter >= 0);
        QVERIFY(roleAt<bool>(model, coderAfter, SidebarModel::CurrentRole));
        QCOMPARE(model.currentRow(), coderAfter);
    }

    // Collapsing an ancestor of the selection lifts the selection up to the
    // collapsed node (VSCode behavior) and re-emits its scope.
    void collapsingAncestorMovesSelectionUp()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int worker = findRow(model, QStringLiteral("Worker A"));
        model.activate(worker);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        const int build = findRow(model, QStringLiteral("Build Fleet"));
        model.toggleExpand(build); // hides Worker A's branch

        QVERIFY(findRow(model, QStringLiteral("Worker A")) < 0);
        const int buildAfter = findRow(model, QStringLiteral("Build Fleet"));
        QVERIFY(roleAt<bool>(model, buildAfter, SidebarModel::CurrentRole));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(2).toString(), QStringLiteral("n-build"));
    }

    // Up/Down move the selection between adjacent selectable rows, skipping
    // separators; Enter re-emits the current scope.
    void keyboardNavigationMovesSelection()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        model.activate(findRow(model, QStringLiteral("All Conversations")));
        QSignalSpy spy(&model, &SidebarModel::scopeSelected);

        model.selectNext(); // -> Archived
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("Archived")));
        QCOMPARE(spy.takeFirst().at(0).toInt(), 1); // NodeType::Archived

        model.selectPrevious(); // -> All Conversations
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("All Conversations")));

        spy.clear();
        model.activateCurrent(); // Enter re-emits without moving
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 0); // NodeType::AllConversations
    }

    // Right expands a collapsed node then descends; Left collapses then climbs.
    void arrowKeysExpandCollapseAndTraverse()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int build = findRow(model, QStringLiteral("Build Fleet"));
        model.activate(build);

        model.collapseCurrent(); // collapse Build (selection stays)
        QVERIFY(!roleAt<bool>(model, findRow(model, QStringLiteral("Build Fleet")),
                              SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);

        model.expandCurrent(); // re-expand Build
        QVERIFY(roleAt<bool>(model, findRow(model, QStringLiteral("Build Fleet")),
                             SidebarModel::ExpandedRole));

        model.expandCurrent(); // already expanded -> step to first child
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("Coder")));

        model.collapseCurrent(); // leaf -> climb to parent (Build)
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("Build Fleet")));
    }

    // Collapse-all hides every subtree (roots remain); expand-all restores them.
    void expandAllAndCollapseAllToggleWholeTree()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QVERIFY(model.anyExpanded());

        model.collapseAll();
        QVERIFY(!model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Build Fleet")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Acme Platform")) >= 0); // root stays

        model.expandAll();
        QVERIFY(model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Worker A")) >= 0);
    }

    // collapseAll while a deep node is selected lifts the highlight to its root.
    void collapseAllLiftsSelectionToRoot()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        model.activate(findRow(model, QStringLiteral("Worker A")));
        model.collapseAll();

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        QVERIFY(roleAt<bool>(model, acme, SidebarModel::CurrentRole));
    }

    // Creating a root node adds a top-level row and selects it.
    void createRootNodeAddsAndSelects()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.createRootUnit();

        const int created = findRow(model, QStringLiteral("New fleet"));
        QVERIFY(created >= 0);
        QCOMPARE(roleAt<int>(model, created, SidebarModel::DepthRole), 0);
        QVERIFY(roleAt<bool>(model, created, SidebarModel::CurrentRole));
        QCOMPARE(spy.takeLast().at(0).toInt(), 4); // NodeType::Unit
    }

    // Creating a tag adds a tag row and selects it.
    void createTagAddsAndSelects()
    {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.createTag();

        const int created = findRow(model, QStringLiteral("New tag"));
        QVERIFY(created >= 0);
        QCOMPARE(roleAt<int>(model, created, SidebarModel::NodeTypeRole), 5); // Tag
        QVERIFY(roleAt<bool>(model, created, SidebarModel::CurrentRole));
        QCOMPARE(spy.takeLast().at(0).toInt(), 5);
    }
};

QTEST_MAIN(TestSidebarModel)
#include "tst_sidebar_model.moc"
