#include "domain/agent_node.h"
#include "domain/sidebar_node.h"
#include "persistence/in_memory_conversation_store.h"

#include <QSignalSpy>
#include <QtTest>

using domain::AgentNode;
using domain::ListScope;
using domain::NodeType;
using persistence::InMemoryConversationStore;

// Exercises the uniformly-recursive agent tree in the in-memory store: roots,
// arbitrary depth, subtree-folded counts/scope matching, and creation. These
// guard the invariant that nothing branches on a fixed depth or node kind.
class TestStore : public QObject {
    Q_OBJECT

private:
    static ListScope nodeScope(const QString& id)
    {
        return { NodeType::Node, -1, id };
    }

private slots:
    // Roots are the parentless nodes; one of them is a lone agent (a tree of
    // one) with no children at all.
    void rootsIncludeLoneAgentAndFleet()
    {
        InMemoryConversationStore store;
        const QList<AgentNode> roots = store.agentChildren(QString());
        QStringList ids;
        for (const AgentNode& n : roots) {
            ids << n.id;
        }
        QVERIFY2(ids.contains("n-scratch"), "expected lone-agent root");
        QVERIFY2(ids.contains("n-acme"), "expected fleet root");
        QVERIFY2(store.agentChildren("n-scratch").isEmpty(), "lone root must have no children");
        QVERIFY(!store.agentChildren("n-acme").isEmpty());
    }

    // Orchestrators nest inside orchestrators: acme -> build -> deep -> worker.
    // The parent chain is walked the same way at every level.
    void treeNestsToArbitraryDepth()
    {
        InMemoryConversationStore store;
        QCOMPARE(store.agentNode("n-worker").parentId, QStringLiteral("n-deep"));
        QCOMPARE(store.agentNode("n-deep").parentId, QStringLiteral("n-build"));
        QCOMPARE(store.agentNode("n-build").parentId, QStringLiteral("n-acme"));
        QCOMPARE(store.agentNode("n-acme").parentId, QString());
        // The deep orchestrator really does carry a child.
        QVERIFY(!store.agentChildren("n-deep").isEmpty());
    }

    // A node scope folds over the WHOLE subtree (node + all descendants), and a
    // leaf folds to just its own conversations. Archived items never count.
    void nodeScopeFoldsSubtree()
    {
        InMemoryConversationStore store;
        // Whole platform subtree (acme/build/coder/worker own non-archived convs;
        // n-coder also carries the seeded "Agent blocks demo" transcript).
        QCOMPARE(store.conversationCount(nodeScope("n-acme")), 7);
        QCOMPARE(store.conversationCount(nodeScope("n-build")), 6);
        QCOMPARE(store.conversationCount(nodeScope("n-deep")), 1);
        // Leaves fold to their own (n-coder owns the agent-blocks and the
        // message-roles demo transcripts on top of its two working notes).
        QCOMPARE(store.conversationCount(nodeScope("n-coder")), 4);
        QCOMPARE(store.conversationCount(nodeScope("n-scratch")), 1);
        // The actual conversations come back for the fold too.
        QCOMPARE(store.conversations(nodeScope("n-deep")).size(), 1);
    }

    void allAndArchivedScopes()
    {
        InMemoryConversationStore store;
        QCOMPARE(store.conversationCount({ NodeType::AllConversations, -1, {} }), 8);
        QCOMPARE(store.conversationCount({ NodeType::Archived, -1, {} }), 1);
    }

    void tagScopeIgnoresArchived()
    {
        InMemoryConversationStore store;
        // "ideas" (1) is on the scratch conversation, the seeded agent-blocks demo,
        // the message-roles demo, and an archived review note; the archived one is
        // excluded.
        QCOMPARE(store.conversationCount({ NodeType::Tag, 1, {} }), 3);
        // "todo" (2) is on the build dispatch log and the coder endpoint.
        QCOMPARE(store.conversationCount({ NodeType::Tag, 2, {} }), 2);
    }

    // Creating a conversation under a deep node lifts the folded counts of that
    // node, its ancestors, and the global All scope, via the same recursion.
    void createUnderDeepNodeFoldsUp()
    {
        InMemoryConversationStore store;
        QSignalSpy spy(&store, &persistence::IConversationStore::changed);
        const int before = store.conversationCount(nodeScope("n-acme"));

        const int id = store.createConversation(QStringLiteral("n-worker"));
        QVERIFY(id > 0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(store.conversationCount(nodeScope("n-worker")), 2);
        QCOMPARE(store.conversationCount(nodeScope("n-deep")), 2);
        QCOMPARE(store.conversationCount(nodeScope("n-acme")), before + 1);
        QCOMPARE(store.conversationCount({ NodeType::AllConversations, -1, {} }), 9);
    }

    void unknownNodeYieldsNothing()
    {
        InMemoryConversationStore store;
        QVERIFY(!store.agentNode("nope").isValid());
        QCOMPARE(store.conversationCount(nodeScope("nope")), 0);
    }

    // title(id) returns the stored canonical title (the string the list shows),
    // independent of the conversation's content; an unknown id yields empty.
    void titleReturnsStoredTitle()
    {
        InMemoryConversationStore store;
        const int id = store.createConversation(QStringLiteral("n-worker"));
        QCOMPARE(store.title(id), QStringLiteral("New conversation"));

        store.setContent(id, QStringLiteral("Some unrelated first content line.\nmore"));
        // Content changes must not bleed into the title.
        QCOMPARE(store.title(id), QStringLiteral("New conversation"));
        QVERIFY(store.title(id) != store.content(id));

        QVERIFY(store.title(-1).isEmpty());
    }

    // createNode with an empty parent adds a new root; with a parent it becomes
    // a child reachable via the same agentChildren primitive.
    void createNodeAsRootAndChild()
    {
        InMemoryConversationStore store;
        QSignalSpy spy(&store, &persistence::IConversationStore::changed);

        const int rootsBefore = store.agentChildren(QString()).size();
        const QString root = store.createNode(QString(), domain::AgentNodeKind::Orchestrator);
        QVERIFY(!root.isEmpty());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(store.agentChildren(QString()).size(), rootsBefore + 1);
        QVERIFY(store.agentNode(root).isValid());
        QCOMPARE(store.agentNode(root).parentId, QString());

        const QString child = store.createNode(root, domain::AgentNodeKind::Engine);
        QCOMPARE(store.agentNode(child).parentId, root);
        QCOMPARE(store.agentChildren(root).size(), 1);
        // Distinct ids each time.
        QVERIFY(child != root);
    }

    // createTag adds a selectable tag with a fresh, unique id.
    void createTagAppendsWithUniqueId()
    {
        InMemoryConversationStore store;
        QSignalSpy spy(&store, &persistence::IConversationStore::changed);

        const int before = store.tags().size();
        const int a = store.createTag(QStringLiteral("alpha"), QStringLiteral("#111111"));
        const int b = store.createTag(QStringLiteral("beta"), QStringLiteral("#222222"));
        QCOMPARE(spy.count(), 2);
        QCOMPARE(store.tags().size(), before + 2);
        QVERIFY(a != b);
        // Existing seeded ids (1,2) are not reused.
        QVERIFY(a > 2 && b > 2);
    }
};

QTEST_MAIN(TestStore)
#include "tst_store.moc"
