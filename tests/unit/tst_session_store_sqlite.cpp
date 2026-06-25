#include "domain/session.h"
#include "domain/ids.h"
#include "domain/sidebar_node.h"
#include "persistence/sqlite_session_store.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
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
    return { NodeType::AllSessions, -1, UnitId() };
}
ListScope archivedScope()
{
    return { NodeType::Archived, -1, UnitId() };
}
UnitId U(const char* s) { return UnitId(QString::fromLatin1(s)); }
} // namespace

// Exercises the durable SQLite session store: a fresh production database starts
// empty, demo data is opt-in, and every mutation survives closing and reopening
// the same file (the offline cache the audit calls for).
class TestSqliteStore : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString dbPath() const { return m_dir.path() + QStringLiteral("/sessions.db"); }

private slots:
    void init()
    {
        QFile::remove(dbPath());
        QFile::remove(dbPath() + QStringLiteral("-shm"));
        QFile::remove(dbPath() + QStringLiteral("-wal"));
    }

    void freshDatabaseStartsEmpty()
    {
        SqliteSessionStore store(dbPath());
        QCOMPARE(store.sessionCount(allScope()), 0);
        QVERIFY(store.unitChildren(UnitId()).isEmpty());
    }

    void demoSeedIsOptIn()
    {
        SqliteSessionStore store(dbPath(), nullptr, true);
        QVERIFY(store.sessionCount(allScope()) > 0);
        QVERIFY(!store.unitChildren(UnitId()).isEmpty());
    }

    void mutationsSurviveReopen()
    {
        domain::SessionId newId;
        int baselineCount = 0;
        {
            SqliteSessionStore store(dbPath());
            baselineCount = store.sessionCount(allScope());

            newId = store.newSession(U("n-scratch"));
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
        domain::SessionId keepId;
        domain::SessionId dropId;
        {
            SqliteSessionStore store(dbPath());
            keepId = store.newSession(U("n-scratch"));
            dropId = store.newSession(U("n-scratch"));
            store.setArchived(keepId, true);
            store.deleteSession(dropId);
        }
        {
            SqliteSessionStore store(dbPath());
            // Archived session is gone from All but present in Archived.
            const QList<Session> archived = store.sessions(archivedScope());
            bool found = false;
            for (const Session& c : archived) {
                if (c.sessionId == keepId) {
                    found = true;
                }
            }
            QVERIFY2(found, "archived session should persist in the Archived scope");
            // Deleted session has no title (unknown) after reopen.
            QVERIFY(store.title(dropId).isEmpty());
        }
    }

    void newIdsDoNotCollideAfterReopen()
    {
        domain::SessionId firstNew;
        {
            SqliteSessionStore store(dbPath());
            firstNew = store.newSession(UnitId());
        }
        {
            SqliteSessionStore store(dbPath());
            const domain::SessionId secondNew = store.newSession(UnitId());
            QVERIFY2(secondNew != firstNew, "newSession mints distinct ids across reopen");
        }
    }

    // The authoritative string sessionId is a persisted column, so it survives a reopen unchanged
    // (no longer regenerated as local-{id}).
    void realSessionIdSurvivesReopen()
    {
        domain::SessionId sid;
        {
            SqliteSessionStore store(dbPath());
            sid = store.newSession(U("n-scratch"));
            QVERIFY(!sid.isEmpty());
            QVERIFY2(!sid.toString().startsWith(QLatin1String("local-")),
                     "newSession mints a real id");
            store.renameSession(sid, QStringLiteral("Persisted"));
        }
        {
            SqliteSessionStore store(dbPath());
            // The same string SessionId keys the row after reopen (persisted verbatim).
            QCOMPARE(store.title(sid), QStringLiteral("Persisted"));
            bool present = false;
            for (const Session& c : store.sessions(allScope())) {
                present = present || c.sessionId == sid;
            }
            QVERIFY(present);
        }
    }

    // A pre-migration db (sessions table without a session_id column) still loads: createSchema adds
    // the column and loadAll backfills the legacy local-{id} form for the null value.
    void legacyRowWithoutSessionIdFallsBack()
    {
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                       QStringLiteral("legacy-seed"));
            db.setDatabaseName(dbPath());
            QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral(
                "CREATE TABLE sessions(id INTEGER PRIMARY KEY, agent_id TEXT, tag_ids TEXT, "
                "title TEXT, content TEXT, archived INTEGER, pinned INTEGER, created TEXT, "
                "modified TEXT, ord INTEGER)")));
            QVERIFY(q.exec(QStringLiteral(
                "INSERT INTO sessions(id, agent_id, tag_ids, title, content, archived, pinned, "
                "created, modified, ord) VALUES(5,'n-scratch','','Legacy thread','body',0,0,'','',0)")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("legacy-seed"));

        SqliteSessionStore store(dbPath());
        QString sid;
        for (const Session& c : store.sessions(allScope())) {
            if (c.title == QStringLiteral("Legacy thread")) {
                sid = c.sessionId.toString();
            }
        }
        QCOMPARE(sid, QStringLiteral("local-5"));
    }
};

QTEST_MAIN(TestSqliteStore)
#include "tst_session_store_sqlite.moc"
