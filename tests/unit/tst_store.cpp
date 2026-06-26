#include "domain/ids.h"
#include "domain/sidebar_node.h"
#include "domain/unit_node.h"
#include "persistence/in_memory_session_store.h"

#include <QSignalSpy>
#include <QtTest>

using domain::ListScope;
using domain::NodeType;
using domain::SessionId;
using domain::UnitId;
using domain::UnitNode;
using persistence::InMemorySessionStore;

namespace {
UnitId U(const char* s) {
    return UnitId(QString::fromLatin1(s));
}
} // namespace

// Exercises the uniformly-recursive unit tree in the in-memory store: roots,
// arbitrary depth, subtree-folded counts/scope matching, and creation. These
// guard the invariant that nothing branches on a fixed depth or unit kind.
class TestStore : public QObject {
    Q_OBJECT

private:
    static ListScope unitScope(const char* id) { return {NodeType::Unit, -1, U(id)}; }

private slots:
    // Roots are the parentless units; one of them is a lone agent (a tree of one)
    // with no children at all.
    void rootsIncludeLoneAgentAndFleet() {
        InMemorySessionStore store;
        const QList<UnitNode> roots = store.unitChildren(UnitId());
        QStringList ids;
        for (const UnitNode& n : roots) {
            ids << n.id.toString();
        }
        QVERIFY2(ids.contains("n-scratch"), "expected lone-agent root");
        QVERIFY2(ids.contains("n-acme"), "expected fleet root");
        QVERIFY2(store.unitChildren(U("n-scratch")).isEmpty(), "lone root must have no children");
        QVERIFY(!store.unitChildren(U("n-acme")).isEmpty());
    }

    // Orchestrators nest inside orchestrators: acme -> build -> deep -> worker.
    // The parent chain is walked the same way at every level.
    void treeNestsToArbitraryDepth() {
        InMemorySessionStore store;
        QCOMPARE(store.unit(U("n-worker")).parentId.toString(), QStringLiteral("n-deep"));
        QCOMPARE(store.unit(U("n-deep")).parentId.toString(), QStringLiteral("n-build"));
        QCOMPARE(store.unit(U("n-build")).parentId.toString(), QStringLiteral("n-acme"));
        QCOMPARE(store.unit(U("n-acme")).parentId.toString(), QString());
        // The deep orchestrator really does carry a child.
        QVERIFY(!store.unitChildren(U("n-deep")).isEmpty());
    }

    // A unit scope folds over the WHOLE subtree (unit + all descendants), and a
    // leaf folds to just its own sessions. Archived items never count.
    void unitScopeFoldsSubtree() {
        InMemorySessionStore store;
        QCOMPARE(store.sessionCount(unitScope("n-acme")), 7);
        QCOMPARE(store.sessionCount(unitScope("n-build")), 6);
        QCOMPARE(store.sessionCount(unitScope("n-deep")), 1);
        QCOMPARE(store.sessionCount(unitScope("n-coder")), 4);
        QCOMPARE(store.sessionCount(unitScope("n-scratch")), 1);
        QCOMPARE(store.sessions(unitScope("n-deep")).size(), 1);
    }

    void allAndArchivedScopes() {
        InMemorySessionStore store;
        QCOMPARE(store.sessionCount({NodeType::AllSessions, -1, {}}), 8);
        QCOMPARE(store.sessionCount({NodeType::Archived, -1, {}}), 1);
    }

    void tagScopeIgnoresArchived() {
        InMemorySessionStore store;
        QCOMPARE(store.sessionCount({NodeType::Tag, 1, {}}), 3);
        QCOMPARE(store.sessionCount({NodeType::Tag, 2, {}}), 2);
    }

    // Creating a session under a deep unit lifts the folded counts of that unit,
    // its ancestors, and the global All scope, via the same recursion.
    void createUnderDeepUnitFoldsUp() {
        InMemorySessionStore store;
        QSignalSpy spy(&store, &persistence::ISessionStore::changed);
        const int before = store.sessionCount(unitScope("n-acme"));

        const SessionId id = store.newSession(U("n-worker"));
        QVERIFY(!id.isEmpty());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(store.sessionCount(unitScope("n-worker")), 2);
        QCOMPARE(store.sessionCount(unitScope("n-deep")), 2);
        QCOMPARE(store.sessionCount(unitScope("n-acme")), before + 1);
        QCOMPARE(store.sessionCount({NodeType::AllSessions, -1, {}}), 9);
    }

    void unknownUnitYieldsNothing() {
        InMemorySessionStore store;
        QVERIFY(!store.unit(U("nope")).isValid());
        QCOMPARE(store.sessionCount(unitScope("nope")), 0);
    }

    void titleReturnsStoredTitle() {
        InMemorySessionStore store;
        const SessionId id = store.newSession(U("n-worker"));
        QCOMPARE(store.title(id), QStringLiteral("New session"));

        store.setContent(id, QStringLiteral("Some unrelated first content line.\nmore"));
        QCOMPARE(store.title(id), QStringLiteral("New session"));
        QVERIFY(store.title(id) != store.content(id));

        QVERIFY(store.title(SessionId()).isEmpty());
    }

    // createUnit with an empty parent adds a new root; with a parent it becomes a
    // child reachable via the same unitChildren primitive.
    void createUnitAsRootAndChild() {
        InMemorySessionStore store;
        QSignalSpy spy(&store, &persistence::ISessionStore::changed);

        const int rootsBefore = store.unitChildren(UnitId()).size();
        const UnitId root = store.createUnit(UnitId(), domain::UnitKind::Orchestrator);
        QVERIFY(!root.isEmpty());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(store.unitChildren(UnitId()).size(), rootsBefore + 1);
        QVERIFY(store.unit(root).isValid());
        QCOMPARE(store.unit(root).parentId.toString(), QString());

        const UnitId child = store.createUnit(root, domain::UnitKind::Engine);
        QCOMPARE(store.unit(child).parentId, root);
        QCOMPARE(store.unitChildren(root).size(), 1);
        QVERIFY(child != root);
    }

    void renameSetsTitle() {
        InMemorySessionStore store;
        const SessionId id = store.newSession(U("n-worker"));
        QSignalSpy spy(&store, &persistence::ISessionStore::changed);

        store.renameSession(id, QStringLiteral("Release plan"));
        QCOMPARE(store.title(id), QStringLiteral("Release plan"));
        QVERIFY(spy.count() >= 1);
    }

    void deleteRemovesSession() {
        InMemorySessionStore store;
        const SessionId id = store.newSession(U("n-worker"));
        QCOMPARE(store.sessionCount(unitScope("n-worker")), 2);

        QSignalSpy spy(&store, &persistence::ISessionStore::changed);
        store.deleteSession(id);
        QVERIFY(spy.count() >= 1);
        QCOMPARE(store.sessionCount(unitScope("n-worker")), 1);
        QVERIFY(store.title(id).isEmpty()); // gone
    }

    void pinFloatsToTopOfScope() {
        InMemorySessionStore store;
        const SessionId a = store.newSession(U("n-worker"));
        const SessionId b = store.newSession(U("n-worker"));
        QVERIFY(!store.isPinned(b));

        store.setPinned(b, true);
        QVERIFY(store.isPinned(b));

        const auto convs = store.sessions(unitScope("n-worker"));
        QStringList ids;
        for (const auto& c : convs) {
            ids << c.sessionId.toString();
        }
        QVERIFY(ids.indexOf(b.toString()) < ids.indexOf(a.toString()));

        store.setPinned(b, false);
        QVERIFY(!store.isPinned(b));
    }

    void createTagAppendsWithUniqueId() {
        InMemorySessionStore store;
        QSignalSpy spy(&store, &persistence::ISessionStore::changed);

        const int before = store.tags().size();
        const int a = store.createTag(QStringLiteral("alpha"), QStringLiteral("#111111"));
        const int b = store.createTag(QStringLiteral("beta"), QStringLiteral("#222222"));
        QCOMPARE(spy.count(), 2);
        QCOMPARE(store.tags().size(), before + 2);
        QVERIFY(a != b);
        QVERIFY(a > 2 && b > 2);
    }

    // The SessionId-keyed API is the only session key into the store: newSession mints a real
    // opaque id, and content/title/pin/archive/delete all round-trip through it.
    void sessionIdKeyedCrud() {
        InMemorySessionStore store;
        const SessionId sid = store.newSession(U("n-worker"));
        QVERIFY(!sid.isEmpty());
        QVERIFY2(!sid.toString().startsWith(QLatin1String("local-")),
                 "newSession mints a real opaque id, not the legacy local- form");

        // The new session appears in the All scope keyed by its real SessionId.
        bool present = false;
        for (const auto& c : store.sessions({NodeType::AllSessions, -1, {}})) {
            present = present || c.sessionId == sid;
        }
        QVERIFY(present);

        store.setContent(sid, QStringLiteral("# canonical"));
        QCOMPARE(store.content(sid), QStringLiteral("# canonical"));

        store.renameSession(sid, QStringLiteral("Renamed by id"));
        QCOMPARE(store.title(sid), QStringLiteral("Renamed by id"));

        store.setPinned(sid, true);
        QVERIFY(store.isPinned(sid));
        store.setPinned(sid, false);
        QVERIFY(!store.isPinned(sid));

        store.setArchived(sid, true);
        QCOMPARE(store.sessionCount({NodeType::Archived, -1, {}}), 2); // seed has 1 archived + this

        store.deleteSession(sid);
        QVERIFY(store.title(sid).isEmpty());
    }

    // Typed ids round-trip through their string form (daemon string_id parity).
    void typedIdsRoundTrip() {
        QCOMPARE(UnitId(QStringLiteral("u-1")).toString(), QStringLiteral("u-1"));
        QVERIFY(UnitId() != UnitId(QStringLiteral("u-1")));
        QVERIFY(UnitId(QStringLiteral("x")) == UnitId(QStringLiteral("x")));
        QVERIFY(domain::ProfileRef().isEmpty());
        QCOMPARE(domain::SessionId(QStringLiteral("s-9")).toString(), QStringLiteral("s-9"));
    }
};

QTEST_MAIN(TestStore)
#include "tst_store.moc"
