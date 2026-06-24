#include "domain/session.h"
#include "domain/ids.h"
#include "domain/sidebar_node.h"
#include "persistence/sqlite_session_store.h"

#include <QTemporaryDir>
#include <QtTest>

using domain::ListScope;
using domain::NodeType;
using domain::Session;
using domain::UnitId;
using persistence::SqliteSessionStore;

namespace {
ListScope allScope()
{
    return { NodeType::AllConversations, -1, UnitId() };
}
ListScope archivedScope()
{
    return { NodeType::Archived, -1, UnitId() };
}
UnitId U(const char* s) { return UnitId(QString::fromLatin1(s)); }
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
        SqliteSessionStore store(dbPath());
        // The demo seed includes several non-archived conversations and a root tree.
        QVERIFY(store.sessionCount(allScope()) > 0);
        QVERIFY(!store.unitChildren(UnitId()).isEmpty());
    }

    void mutationsSurviveReopen()
    {
        int newId = -1;
        int baselineCount = 0;
        {
            SqliteSessionStore store(dbPath());
            baselineCount = store.sessionCount(allScope());

            newId = store.createSession(U("n-scratch"));
            store.renameSession(newId, QStringLiteral("Persisted thread"));
            store.setContent(newId, QStringLiteral("# durable\n\nhello"));
            store.setPinned(newId, true);
            QCOMPARE(store.sessionCount(allScope()), baselineCount + 1);
        }

        // Reopen the SAME file in a new store instance: state must be restored.
        {
            SqliteSessionStore store(dbPath());
            QCOMPARE(store.sessionCount(allScope()), baselineCount + 1);
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
            SqliteSessionStore store(dbPath());
            keepId = store.createSession(U("n-scratch"));
            dropId = store.createSession(U("n-scratch"));
            store.setArchived(keepId, true);
            store.deleteSession(dropId);
        }
        {
            SqliteSessionStore store(dbPath());
            // Archived conversation is gone from All but present in Archived.
            const QList<Session> archived = store.sessions(archivedScope());
            bool found = false;
            for (const Session& c : archived) {
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
            SqliteSessionStore store(dbPath());
            firstNew = store.createSession(UnitId());
        }
        {
            SqliteSessionStore store(dbPath());
            const int secondNew = store.createSession(UnitId());
            QVERIFY2(secondNew != firstNew, "id counter must persist so ids never collide");
        }
    }
};

QTEST_MAIN(TestSqliteStore)
#include "tst_session_store_sqlite.moc"
