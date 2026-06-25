#include "daemonnet/mock_daemonnet.h"

#include "domain/ids.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"
#include "domain/unit_node.h"

#include <QSet>
#include <QtTest/QtTest>

using daemonnet::MockDaemonNet;
using domain::ListScope;
using domain::NodeType;
using domain::Session;
using domain::SessionId;
using domain::UnitId;
using domain::UnitNode;

// Guards the P2 IDaemonNet typed read seam (roadmap dn-interface): the accessors a future
// DaemonNetSessionStore / daemon adapter delegate to, derived from the one P1 typed seed.
class TestDaemonNet : public QObject {
    Q_OBJECT

    static QStringList sessionIds(const QList<Session>& sessions)
    {
        QStringList ids;
        for (const Session& s : sessions) {
            ids << s.sessionId.toString();
        }
        return ids;
    }

private slots:
    void unitTreeAccessors()
    {
        MockDaemonNet net;
        const QList<UnitNode> roots = net.unitChildren(UnitId());
        QCOMPARE(roots.size(), 2); // the lone Engine root + the Acme orchestrator
        QStringList rootIds;
        for (const UnitNode& u : roots) {
            rootIds << u.id.toString();
        }
        QVERIFY(rootIds.contains(QStringLiteral("n-scratch")));
        QVERIFY(rootIds.contains(QStringLiteral("n-acme")));

        // n-build's direct children are the dispatched workers/orchestrator.
        const QList<UnitNode> buildKids = net.unitChildren(UnitId(QStringLiteral("n-build")));
        QVERIFY(buildKids.size() >= 3);

        QCOMPARE(net.unit(UnitId(QStringLiteral("n-coder"))).id.toString(),
                 QStringLiteral("n-coder"));
        QVERIFY(net.unit(UnitId(QStringLiteral("does-not-exist"))).id.isEmpty());
    }

    void tagsAccessor() { QCOMPARE(MockDaemonNet().tags().size(), 2); }

    void allAndArchivedScopes()
    {
        MockDaemonNet net;
        const QList<Session> all = net.sessionsInScope(ListScope{ NodeType::AllSessions });
        QVERIFY(!all.isEmpty());
        for (const Session& s : all) {
            QVERIFY2(!s.sessionId.isEmpty(), "every session carries its real SessionId");
            QVERIFY(!s.isArchived);
        }

        const QList<Session> archived = net.sessionsInScope(ListScope{ NodeType::Archived });
        QVERIFY(!archived.isEmpty());
        for (const Session& s : archived) {
            QVERIFY(s.isArchived);
        }
        // All and Archived are disjoint partitions.
        const QStringList allIds = sessionIds(all);
        const QSet<QString> allSet(allIds.begin(), allIds.end());
        for (const Session& s : archived) {
            QVERIFY(!allSet.contains(s.sessionId.toString()));
        }
    }

    void unitSubtreeScopeFolds()
    {
        MockDaemonNet net;
        ListScope scope;
        scope.type = NodeType::Unit;
        scope.unitId = UnitId(QStringLiteral("n-build"));
        const QList<Session> sessions = net.sessionsInScope(scope);
        QVERIFY(sessions.size() >= 5); // folds the whole n-build subtree, not just direct children

        const QSet<QString> subtree = { QStringLiteral("n-build"), QStringLiteral("n-coder"),
                                        QStringLiteral("n-review"), QStringLiteral("n-deep"),
                                        QStringLiteral("n-worker") };
        for (const Session& s : sessions) {
            QVERIFY2(subtree.contains(s.unitId.toString()),
                     "unit scope must only return sessions inside the subtree");
        }
    }

    void tagScopeFilters()
    {
        MockDaemonNet net;
        ListScope scope;
        scope.type = NodeType::Tag;
        scope.tagId = 1; // "ideas"
        const QList<Session> sessions = net.sessionsInScope(scope);
        QVERIFY(!sessions.isEmpty());
        for (const Session& s : sessions) {
            QVERIFY(s.tagIds.contains(1));
            QVERIFY(!s.isArchived);
        }
    }

    void lensScopesGroupByEdge()
    {
        MockDaemonNet net;
        ListScope byTransport;
        byTransport.type = NodeType::ByTransport;
        byTransport.lensKey = QStringLiteral("matrix/@bot:hs.org");
        const QStringList overMatrix = sessionIds(net.sessionsInScope(byTransport));
        QVERIFY(overMatrix.contains(QStringLiteral("s-help")));
        QVERIFY(overMatrix.contains(QStringLiteral("s-secops")));

        ListScope byPeer;
        byPeer.type = NodeType::ByPeer;
        byPeer.lensKey = QStringLiteral("@alice:hs.org");
        const QStringList withAlice = sessionIds(net.sessionsInScope(byPeer));
        QVERIFY(withAlice.contains(QStringLiteral("s-help")));
        QVERIFY(withAlice.contains(QStringLiteral("s-secops")));
    }

    void sessionDetailAndContent()
    {
        MockDaemonNet net;
        const SessionId id(QStringLiteral("s-demo-blocks"));
        const Session detail = net.sessionDetail(id);
        QCOMPARE(detail.sessionId.toString(), id.toString());
        QVERIFY(!detail.content.isEmpty());
        QCOMPARE(net.content(id), detail.content);

        // Unknown id yields an invalid session and empty content.
        QVERIFY(net.sessionDetail(SessionId(QStringLiteral("nope"))).sessionId.isEmpty());
        QVERIFY(net.content(SessionId(QStringLiteral("nope"))).isEmpty());
    }
};

QTEST_MAIN(TestDaemonNet)

#include "tst_daemonnet.moc"
