#include <QtTest/QtTest>

#include "memory/mock_memory_service.h"
#include "memory_graph_model.h"
#include "memory_list_model.h"
#include "memory_stats_model.h"
#include "memory_timeline_model.h"
#include "tab_model.h"

// Exercises the seeded memory seam + the shared view-models that back BOTH the GUI
// and the TUI memory views. The mock delivers results asynchronously (queued
// emit), so the assertions spin the event loop via QTRY_*.
class TestMemory : public QObject {
    Q_OBJECT

private slots:
    void listPopulatesAndFilters();
    void listSortsByImportance();
    void searchMatchesContent();
    void memoryIsOwnedByAgent();
    void sessionFiltersWithinAgent();
    void requestSessionsListsAgentSessions();
    void statsAggregate();
    void graphHasNodesAndNeighbours();
    void graphKnowledgeKind();
    void timelineGroupsEvents();
    void tabsAreKeyedPerAgent();
};

void TestMemory::listPopulatesAndFilters()
{
    memory::MockMemoryService svc;
    memoryui::MemoryListModel model;
    model.setService(&svc);

    QTRY_VERIFY(model.rowCount() > 0);

    // The filtered result arrives asynchronously and replaces the rows, so wait on
    // the precise predicate (every row is working) rather than just rowCount > 0.
    const auto allWorking = [&model] {
        for (int i = 0; i < model.rowCount(); ++i) {
            if (model.entryAt(i).value(QStringLiteral("tier")).toString()
                != QStringLiteral("working"))
                return false;
        }
        return true;
    };
    model.setFilter(QStringLiteral("tier"), QStringLiteral("working"));
    QTRY_VERIFY(model.rowCount() > 0 && allWorking());

    // Clearing the filter restores the (larger) unfiltered set.
    const int filtered = model.rowCount();
    model.clearFilters();
    QTRY_VERIFY(model.rowCount() >= filtered);
}

void TestMemory::listSortsByImportance()
{
    memory::MockMemoryService svc;
    memoryui::MemoryListModel model;
    model.setService(&svc);
    model.setSort(QStringLiteral("importance"));
    QTRY_VERIFY(model.rowCount() > 1);

    double prev = 1e9;
    for (int i = 0; i < model.rowCount(); ++i) {
        const double imp = model.entryAt(i).value(QStringLiteral("importance")).toDouble();
        QVERIFY(imp <= prev + 1e-9);
        prev = imp;
    }
}

void TestMemory::searchMatchesContent()
{
    memory::MockMemoryService svc;
    memoryui::MemoryListModel model;
    model.setService(&svc);
    QTRY_VERIFY(model.rowCount() > 0);

    const auto allMatchRust = [&model] {
        for (int i = 0; i < model.rowCount(); ++i) {
            if (!model.entryAt(i)
                     .value(QStringLiteral("content"))
                     .toString()
                     .contains(QStringLiteral("Rust"), Qt::CaseInsensitive))
                return false;
        }
        return true;
    };
    model.search(QStringLiteral("Rust"));
    QTRY_VERIFY(model.rowCount() > 0 && allMatchRust());
}

void TestMemory::memoryIsOwnedByAgent()
{
    // Memory is owned by the agent (one bank per profile): two agents' banks are
    // disjoint, and binding to a profile shows that agent's whole bank.
    memory::MockMemoryService svc;
    memoryui::MemoryListModel model;
    model.setService(&svc);

    const auto hasId = [&model](const QString& id) {
        for (int i = 0; i < model.rowCount(); ++i) {
            if (model.idAt(i) == id)
                return true;
        }
        return false;
    };
    const auto collect = [&model] {
        QSet<QString> ids;
        for (int i = 0; i < model.rowCount(); ++i)
            ids.insert(model.idAt(i));
        return ids;
    };

    // m01 lives only in prof-1's bank; m05 only in prof-2's. Wait on those markers
    // so we know the async re-scope has actually landed before snapshotting.
    svc.setScope(QStringLiteral("prof-1"), QString(), true);
    QTRY_VERIFY(hasId(QStringLiteral("m01")) && !hasId(QStringLiteral("m05")));
    const QSet<QString> a = collect();

    svc.setScope(QStringLiteral("prof-2"), QString(), true);
    QTRY_VERIFY(hasId(QStringLiteral("m05")) && !hasId(QStringLiteral("m01")));
    const QSet<QString> b = collect();

    QVERIFY(!a.isEmpty());
    QVERIFY(!b.isEmpty());
    QVERIFY(!a.intersects(b)); // distinct agents => disjoint memory
}

void TestMemory::sessionFiltersWithinAgent()
{
    // A session is an optional lens WITHIN one agent's bank, not the binding.
    memory::MockMemoryService svc;
    memoryui::MemoryListModel model;
    model.setService(&svc);

    svc.setScope(QStringLiteral("prof-3"), QString(), true); // whole bank
    QTRY_VERIFY(model.rowCount() > 0);
    const int whole = model.rowCount();

    const auto allInSession = [&model](const QString& session) {
        for (int i = 0; i < model.rowCount(); ++i) {
            if (model.entryAt(i).value(QStringLiteral("sessionId")).toString() != session)
                return false;
        }
        return true;
    };
    svc.setScope(QStringLiteral("prof-3"), QStringLiteral("rs-survey"), false);
    QTRY_VERIFY(model.rowCount() > 0 && allInSession(QStringLiteral("rs-survey")));
    QVERIFY(model.rowCount() <= whole);
}

void TestMemory::requestSessionsListsAgentSessions()
{
    memory::MockMemoryService svc;
    QSignalSpy spy(&svc, &memory::IMemoryService::sessionsReady);
    svc.requestSessions(QStringLiteral("prof-1"));
    QTRY_VERIFY(spy.count() > 0);
    const QVariantList sessions = spy.takeFirst().at(1).toList();
    QStringList ids;
    for (const QVariant& v : sessions)
        ids << v.toMap().value(QStringLiteral("id")).toString();
    QVERIFY(ids.contains(QStringLiteral("ga-plan")));
    QVERIFY(ids.contains(QStringLiteral("ga-notes")));
}

void TestMemory::statsAggregate()
{
    memory::MockMemoryService svc;
    memoryui::MemoryStatsModel stats;
    stats.setService(&svc);

    QTRY_VERIFY(stats.total() > 0);
    QCOMPARE(stats.total(), stats.working() + stats.episodic() + stats.scratchpad());
    QVERIFY(!stats.bySource().isEmpty());
    QVERIFY(stats.facts() > 0);
}

void TestMemory::graphHasNodesAndNeighbours()
{
    memory::MockMemoryService svc;
    memoryui::MemoryGraphModel graph;
    graph.setService(&svc); // defaults to the association graph
    QTRY_VERIFY(graph.nodeCount() > 0);
    QVERIFY(graph.edgeCount() > 0);

    // Find a node with at least one incident edge and verify neighbours resolve.
    const QVariantList edges = graph.edges();
    QVERIFY(!edges.isEmpty());
    const QString seed = edges.first().toMap().value(QStringLiteral("source")).toString();
    QVERIFY(!graph.neighborsOf(seed).isEmpty());
}

void TestMemory::graphKnowledgeKind()
{
    memory::MockMemoryService svc;
    memoryui::MemoryGraphModel graph;
    graph.setService(&svc);
    QTRY_VERIFY(graph.nodeCount() > 0);

    graph.setKind(QStringLiteral("knowledge"));
    QTRY_VERIFY(graph.nodeCount() > 0);
    QVERIFY(graph.edgeCount() > 0);
}

void TestMemory::timelineGroupsEvents()
{
    memory::MockMemoryService svc;
    memoryui::MemoryTimelineModel timeline;
    timeline.setService(&svc);
    QTRY_VERIFY(timeline.rowCount() > 0);

    // The flattened model must contain at least one group header row.
    bool sawHeader = false;
    for (int i = 0; i < timeline.rowCount(); ++i) {
        if (timeline.data(timeline.index(i), memoryui::MemoryTimelineModel::IsHeaderRole).toBool()) {
            sawHeader = true;
            break;
        }
    }
    QVERIFY(sawHeader);
}

void TestMemory::tabsAreKeyedPerAgent()
{
    // Memory/Profile tabs are multi-instance keyed by (kind, profile): the same
    // agent re-activates its tab; a different agent opens a new one; and the two
    // kinds are independent keyspaces.
    TabModel tabs;
    const int memA = tabs.openAgentTab(TabModel::Memory, QStringLiteral("prof-1"),
                                       QStringLiteral("Memory - A"));
    const int afterFirst = tabs.count();
    const int memASame = tabs.openAgentTab(TabModel::Memory, QStringLiteral("prof-1"),
                                           QStringLiteral("Memory - A"));
    QCOMPARE(memASame, memA);          // same agent -> same tab
    QCOMPARE(tabs.count(), afterFirst);

    const int memB = tabs.openAgentTab(TabModel::Memory, QStringLiteral("prof-2"),
                                       QStringLiteral("Memory - B"));
    QVERIFY(memB != memA);             // different agent -> new tab
    QCOMPARE(tabs.count(), afterFirst + 1);

    const int profA = tabs.openAgentTab(TabModel::Profile, QStringLiteral("prof-1"),
                                        QStringLiteral("Profile - A"));
    QVERIFY(profA != memA);            // Profile is an independent keyspace
    QCOMPARE(tabs.agentRefAt(tabs.indexOfTabId(profA)), QStringLiteral("prof-1"));
}

QTEST_MAIN(TestMemory)
#include "tst_memory.moc"
