// tst_mirror_write_behind — the durable journal consumer (spec 09 §4.4/§5.1, B7). Asserts the
// one-transaction invariant: entity rows + window rows + window_meta + mirror_journal + watermark
// + wire-cursor all commit together (or none — crash consistency), and boot reload round-trips.

#include "mirror/observe_coarse.h"
#include "mirror/persistence.h"
#include "mirror/store.h"
#include "mirror/write_behind.h"

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

using namespace mirror;

namespace {
Conversation conv(const QString& t, const QString& id, const QString& title) {
    Conversation c;
    c.transport = t;
    c.id = id;
    c.title = title;
    return c;
}
ChatMessage chat(const QString& t, const QString& c, quint64 cursor, const QString& text) {
    ChatMessage m;
    m.transport = t;
    m.conv = c;
    m.cursor = cursor;
    m.text = text;
    return m;
}
} // namespace

class TstMirrorWriteBehind : public QObject {
    Q_OBJECT

private slots:
    void oneTransactionRowsJournalWatermarkCursor() {
        QTemporaryDir dir;
        FixedDbPaths paths(dir.filePath(QStringLiteral("mirror.db")), QString());
        Persistence db;
        QVERIFY(db.open(paths));

        CoarseObserve obs;
        Store store(obs);
        WriteBehind wb(store, db);
        wb.start();

        store.beginBatch().upsert(conv("m", "!a", "A")).upsert(conv("m", "!b", "B")).commit();
        wb.setPendingCursor(QStringLiteral("events-since"), 512, 3);
        QVERIFY(wb.flush());

        // Rows + journal + watermark + cursor all landed in the one transaction (B7).
        MirrorModel loaded;
        QVERIFY(db.loadInto(loaded));
        QCOMPARE(loaded.conversations.size(), std::size_t(2));
        QCOMPARE(db.persistedJournalHead(), quint64(2));
        QCOMPARE(db.loadWatermark(QStringLiteral("write-behind")), quint64(2));
        quint64 epoch = 0;
        QCOMPARE(db.loadCursor(QStringLiteral("events-since"), &epoch), quint64(512));
        QCOMPARE(epoch, quint64(3));
        QCOMPARE(wb.watermark(), quint64(2)); // in-memory watermark advanced only after commit
    }

    void failedWriteLeavesWatermarkUnadvanced() {
        // Crash consistency: if the transaction cannot commit, the watermark does NOT advance —
        // the batch is retried whole next time (never a partial/skipped apply).
        QTemporaryDir dir;
        FixedDbPaths paths(dir.filePath(QStringLiteral("mirror.db")), QString());
        Persistence db;
        QVERIFY(db.open(paths));
        CoarseObserve obs;
        Store store(obs);
        WriteBehind wb(store, db);
        wb.start();

        store.beginBatch().upsert(conv("m", "!a", "A")).commit();
        QVERIFY(wb.flush());
        QCOMPARE(wb.watermark(), quint64(1));

        // Close the DB to force the next write-behind transaction to fail (a torn-write surrogate).
        db.close();
        store.beginBatch().upsert(conv("m", "!b", "B")).commit();
        QVERIFY(!wb.flush());                 // transaction failed
        QCOMPARE(wb.watermark(), quint64(1)); // watermark held — the delta is retried, never lost
    }

    void chatWindowRowsAndMetaPersisted() {
        QTemporaryDir dir;
        const QString dbPath = dir.filePath(QStringLiteral("mirror.db"));
        FixedDbPaths paths(dbPath, QString());
        Persistence db;
        QVERIFY(db.open(paths));
        CoarseObserve obs;
        Store store(obs);
        WriteBehind wb(store, db);
        wb.start();

        auto b = store.beginBatch();
        b.appendWindow(chat("m", "!r", 1, "x"));
        b.appendWindow(chat("m", "!r", 2, "y"));
        b.commit();
        QVERIFY(wb.flush());
        db.close();

        // Inspect the persisted window rows + window_meta on a fresh read connection.
        {
            QSqlDatabase rdb =
                QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("wb_reader"));
            rdb.setDatabaseName(dbPath);
            QVERIFY(rdb.open());
            QSqlQuery rows(rdb);
            QVERIFY(rows.exec(
                QStringLiteral("SELECT COUNT(*) FROM w_chat_messages WHERE scope='m\x1f!r'")));
            QVERIFY(rows.next());
            QCOMPARE(rows.value(0).toInt(), 2);
            QSqlQuery meta(rdb);
            QVERIFY(meta.exec(QStringLiteral(
                "SELECT item_count,newest_cursor FROM window_meta WHERE kind='ChatMessage'")));
            QVERIFY(meta.next());
            QCOMPARE(meta.value(0).toInt(), 2);
            QCOMPARE(meta.value(1).toULongLong(), quint64(2));
            rdb.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("wb_reader"));
    }

    void reloadRoundTripsConversationsContactsPersons() {
        QTemporaryDir dir;
        FixedDbPaths paths(dir.filePath(QStringLiteral("mirror.db")), QString());
        Persistence db;
        QVERIFY(db.open(paths));
        CoarseObserve obs;
        Store store(obs);
        WriteBehind wb(store, db);
        wb.start();

        Contact ct;
        ct.transport = QStringLiteral("m");
        ct.id = QStringLiteral("@bob");
        ct.display_name = QStringLiteral("Bob");
        Person pr;
        pr.id = QStringLiteral("p1");
        pr.alias = QStringLiteral("Robert");

        auto b = store.beginBatch();
        b.upsert(conv("m", "!a", "A"));
        b.upsert(ct);
        b.upsert(pr);
        b.commit();
        QVERIFY(wb.flush());

        MirrorModel loaded;
        QVERIFY(db.loadInto(loaded));
        QCOMPARE(loaded.conversations.size(), std::size_t(1));
        QCOMPARE(loaded.contacts.size(), std::size_t(1));
        QCOMPARE(loaded.persons.size(), std::size_t(1));
        const Contact* c =
            loaded.contacts.find(ContactKey{QStringLiteral("m"), QStringLiteral("@bob")});
        QVERIFY(c != nullptr);
        QCOMPARE(c->display_name, QStringLiteral("Bob"));
    }
};

QTEST_GUILESS_MAIN(TstMirrorWriteBehind)
#include "tst_mirror_write_behind.moc"
