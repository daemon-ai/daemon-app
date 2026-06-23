#include "domain/conversation.h"
#include "domain/sidebar_node.h"
#include "persistence/sqlite_conversation_store.h"

#include <QTemporaryDir>
#include <QtTest>

using domain::Conversation;
using domain::ListScope;
using domain::NodeType;
using persistence::SqliteConversationStore;

namespace {
ListScope allScope()
{
    return { NodeType::AllConversations, -1, QString() };
}
ListScope archivedScope()
{
    return { NodeType::Archived, -1, QString() };
}
} // namespace

// Exercises the durable SQLite conversation store: a fresh database seeds the
// demo data, and every mutation survives closing and reopening the same file
// (the offline cache the audit calls for).
class TestSqliteStore : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString dbPath() const { return m_dir.path() + QStringLiteral("/conversations.db"); }

private slots:
    void freshDatabaseSeeds()
    {
        SqliteConversationStore store(dbPath());
        // The demo seed includes several non-archived conversations and a root tree.
        QVERIFY(store.conversationCount(allScope()) > 0);
        QVERIFY(!store.agentChildren(QString()).isEmpty());
    }

    void mutationsSurviveReopen()
    {
        int newId = -1;
        int baselineCount = 0;
        {
            SqliteConversationStore store(dbPath());
            baselineCount = store.conversationCount(allScope());

            newId = store.createConversation(QStringLiteral("n-scratch"));
            store.renameConversation(newId, QStringLiteral("Persisted thread"));
            store.setContent(newId, QStringLiteral("# durable\n\nhello"));
            store.setPinned(newId, true);
            QCOMPARE(store.conversationCount(allScope()), baselineCount + 1);
        }

        // Reopen the SAME file in a new store instance: state must be restored.
        {
            SqliteConversationStore store(dbPath());
            QCOMPARE(store.conversationCount(allScope()), baselineCount + 1);
            QCOMPARE(store.title(newId), QStringLiteral("Persisted thread"));
            QCOMPARE(store.content(newId), QStringLiteral("# durable\n\nhello"));
            QVERIFY(store.isPinned(newId));
        }
    }

    void archiveAndDeleteSurviveReopen()
    {
        int keepId = -1;
        int dropId = -1;
        {
            SqliteConversationStore store(dbPath());
            keepId = store.createConversation(QStringLiteral("n-scratch"));
            dropId = store.createConversation(QStringLiteral("n-scratch"));
            store.setArchived(keepId, true);
            store.deleteConversation(dropId);
        }
        {
            SqliteConversationStore store(dbPath());
            // Archived conversation is gone from All but present in Archived.
            const QList<Conversation> archived = store.conversations(archivedScope());
            bool found = false;
            for (const Conversation& c : archived) {
                if (c.id == keepId) {
                    found = true;
                }
            }
            QVERIFY2(found, "archived conversation should persist in the Archived scope");
            // Deleted conversation has no title (unknown) after reopen.
            QVERIFY(store.title(dropId).isEmpty());
        }
    }

    void newIdsDoNotCollideAfterReopen()
    {
        int firstNew = -1;
        {
            SqliteConversationStore store(dbPath());
            firstNew = store.createConversation(QString());
        }
        {
            SqliteConversationStore store(dbPath());
            const int secondNew = store.createConversation(QString());
            QVERIFY2(secondNew != firstNew, "id counter must persist so ids never collide");
        }
    }
};

QTEST_MAIN(TestSqliteStore)
#include "tst_conversation_store_sqlite.moc"
