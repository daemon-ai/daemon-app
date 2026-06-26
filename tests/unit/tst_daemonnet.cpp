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

    void participantsAccessor()
    {
        const QList<domain::Participant> parts = MockDaemonNet().participants();
        QCOMPARE(parts.size(), 2);
        // A local Agent + a human User, both "available" (green dot) per the seed.
        QCOMPARE(parts.at(0).name, QStringLiteral("Agent"));
        QVERIFY(parts.at(0).isAgent);
        QCOMPARE(parts.at(1).name, QStringLiteral("User"));
        QVERIFY(!parts.at(1).isAgent);
        for (const domain::Participant& p : parts) {
            QCOMPARE(p.presence, QStringLiteral("available"));
            QVERIFY(!p.color.isEmpty());
        }
    }

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

    // The capability-driven Transports tree: messaging instances expand to ConversationType-grouped
    // conversations -> session leaves; generic instances expand to origin-tagged session leaves.
    void transportsTreeShape()
    {
        MockDaemonNet net;
        const QList<daemonnet::TransportTreeRow> rows = net.transportsTree();
        QVERIFY(!rows.isEmpty());

        QHash<QString, daemonnet::TransportTreeRow> byId;
        QHash<QString, int> childCount;
        for (const auto& r : rows) {
            byId.insert(r.id, r);
            if (!r.parentId.isEmpty()) {
                childCount[r.parentId] += 1;
            }
        }

        // Four transport accounts at depth 0; two messaging (presence), two generic.
        QStringList accounts;
        for (const auto& r : rows) {
            if (r.kind == QStringLiteral("account")) {
                accounts << r.id;
                QCOMPARE(r.depth, 0);
            }
        }
        QCOMPARE(accounts.size(), 4);
        QVERIFY(accounts.contains(QStringLiteral("tx:matrix")));
        QVERIFY(accounts.contains(QStringLiteral("tx:internal")));
        QVERIFY(accounts.contains(QStringLiteral("tx:cron")));
        QVERIFY(accounts.contains(QStringLiteral("tx:http")));

        // Matrix (messaging) groups its conversations under Channels + Direct Messages.
        QCOMPARE(byId.value(QStringLiteral("tx:matrix")).presence, QStringLiteral("available"));
        QCOMPARE(byId.value(QStringLiteral("tx:matrix/ch")).kind, QStringLiteral("convGroup"));
        QCOMPARE(byId.value(QStringLiteral("tx:matrix/dm")).kind, QStringLiteral("convGroup"));
        QCOMPARE(childCount.value(QStringLiteral("tx:matrix/ch")), 2); // #secops + #help
        QCOMPARE(childCount.value(QStringLiteral("tx:matrix/dm")), 2); // @alice + @bob

        // #secops is a Channel conversation whose openable leaf is the s-secops session.
        const auto secops = byId.value(QStringLiteral("tx:matrix/ch/secops"));
        QCOMPARE(secops.kind, QStringLiteral("conversation"));
        QCOMPARE(secops.convType, QStringLiteral("channel"));
        QCOMPARE(secops.sessionId, QStringLiteral("s-secops"));
        // @alice is a Dm conversation cross-linked to the s-help session (agent member n-coder).
        const auto alice = byId.value(QStringLiteral("tx:matrix/dm/alice"));
        QCOMPARE(alice.convType, QStringLiteral("dm"));
        QCOMPARE(alice.sessionId, QStringLiteral("s-help"));
        // #help / @bob are joined but have no active session.
        QVERIFY(byId.value(QStringLiteral("tx:matrix/ch/help")).sessionId.isEmpty());
        QVERIFY(byId.value(QStringLiteral("tx:matrix/dm/bob")).sessionId.isEmpty());

        // internal rooms: a multi-party GroupDm -> s-design.
        const auto design = byId.value(QStringLiteral("tx:internal/design"));
        QCOMPARE(design.convType, QStringLiteral("groupdm"));
        QCOMPARE(design.sessionId, QStringLiteral("s-design"));
        QCOMPARE(design.memberCount, 3);

        // Generic transports: no convGroups - jobs/callers carry their session directly.
        const auto backup = byId.value(QStringLiteral("tx:cron/backup"));
        QCOMPARE(backup.kind, QStringLiteral("job"));
        QCOMPARE(backup.sessionId, QStringLiteral("s-cron-backup"));
        const auto dashboard = byId.value(QStringLiteral("tx:http/dashboard"));
        QCOMPARE(dashboard.kind, QStringLiteral("caller"));
        QCOMPARE(dashboard.sessionId, QStringLiteral("s-http-dashboard"));

        // Every session leaf resolves to a real session (openable transcript).
        for (const auto& r : rows) {
            if (!r.sessionId.isEmpty()) {
                QVERIFY2(!net.sessionDetail(SessionId(r.sessionId)).sessionId.isEmpty(),
                         qPrintable(QStringLiteral("leaf %1 must resolve to a real session").arg(r.id)));
            }
        }
    }
};

QTEST_MAIN(TestDaemonNet)

#include "tst_daemonnet.moc"
