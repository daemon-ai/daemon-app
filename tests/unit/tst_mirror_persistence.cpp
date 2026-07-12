// tst_mirror_persistence — mirror-<id>.db (spec 09 §4.5): schema create from the generated DDL,
// transactional write-behind (rows + journal + watermark + cursor in ONE transaction, B7),
// boot load via transients, schema-version drop-and-rebuild, and the compaction sweep (§4.4).
//
// The write-behind "state becomes visible only after the commit, and cursor advance is atomic
// with the row apply" behavior is re-expressed (NOT copied) from Sink's LGPL storage/pipeline
// tests: tests/storagetest.cpp:481-504 (transaction visibility) and tests/pipelinetest.cpp:
// 238-284 (a processed change is durable + the sync cursor moved together).

#include "mirror/persistence.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

using namespace mirror;

namespace {
Conversation conv(const QString& transport, const QString& id, const QString& title) {
    Conversation c;
    c.transport = transport;
    c.id = id;
    c.title = title;
    return c;
}

JournalRecord rec(quint64 rev, const QString& key, JournalOp op) {
    JournalRecord r;
    r.rev = rev;
    r.kind = EntityKind::Conversation;
    r.key = key;
    r.op = op;
    r.origin = JournalOrigin::RefetchDiff;
    r.at_ms = 1000;
    return r;
}
} // namespace

class TstMirrorPersistence : public QObject {
    Q_OBJECT

private:
    QTemporaryDir dir_;
    QString current_; // unique per test method (isolation)
    [[nodiscard]] QString mirrorPath() const { return current_; }
    [[nodiscard]] FixedDbPaths paths() const {
        return FixedDbPaths(current_, current_ + QStringLiteral(".local"));
    }

private slots:
    void init() {
        // Fresh DB file per test method so persisted state never leaks across cases.
        current_ = dir_.filePath(QString::fromLatin1(QTest::currentTestFunction()) +
                                 QStringLiteral(".db"));
    }

    void createsSchemaAtTargetVersion() {
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));
        QCOMPARE(p.schemaVersion(), Persistence::kSchemaVersion);
        QCOMPARE(p.persistedJournalHead(), quint64(0));
    }

    void writeBehindIsAtomicAndDurable() {
        {
            Persistence p;
            const auto pth = paths();
            QVERIFY2(p.open(pth), qPrintable(p.lastError()));

            WriteBatch wb;
            wb.conversationUpserts.push_back(
                conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")));
            wb.conversationUpserts.push_back(
                conv(QStringLiteral("m"), QStringLiteral("!b"), QStringLiteral("B")));
            wb.journalRecords.push_back(rec(1, QStringLiteral("m\x1f!a"), JournalOp::Upsert));
            wb.journalRecords.push_back(rec(2, QStringLiteral("m\x1f!b"), JournalOp::Upsert));
            wb.advanceWatermark = true;
            wb.watermarkConsumer = QStringLiteral("persistence");
            wb.watermarkRev = 2;
            wb.advanceCursor = true;
            wb.cursorName = QStringLiteral("events-since");
            wb.cursorValue = 4242;
            wb.cursorEpoch = 7;
            QVERIFY2(p.writeBehind(wb), qPrintable(p.lastError()));
            p.close();
        }
        // Reopen: rows + journal + watermark + cursor all survived the same commit.
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));
        QCOMPARE(p.schemaVersion(), Persistence::kSchemaVersion); // existing DB, not rebuilt
        QCOMPARE(p.persistedJournalHead(), quint64(2));
        QCOMPARE(p.loadWatermark(QStringLiteral("persistence")), quint64(2));
        quint64 epoch = 0;
        QCOMPARE(p.loadCursor(QStringLiteral("events-since"), &epoch), quint64(4242));
        QCOMPARE(epoch, quint64(7));

        MirrorModel model;
        QVERIFY(p.loadInto(model));
        QCOMPARE(model.conversations.size(), std::size_t(2));
        const Conversation* a =
            model.conversations.find(ConversationKey{QStringLiteral("m"), QStringLiteral("!a")});
        QVERIFY(a != nullptr);
        QCOMPARE(a->title, QStringLiteral("A"));
        // Boot rebuilds the §3.5 index from loaded rows.
        QCOMPARE(model.conversationsByTransport[QStringLiteral("m")].size(), std::size_t(2));
    }

    // AD (1b.3): the transcript window persists + reloads like chat — the durability the deleted
    // legacy cache leg provided. A scope CLEAR (the engine's rebaseline wipe) drops the persisted
    // generation in the same transaction discipline; the replacement rows survive a reload.
    void transcriptWindowRoundTripsAndClearDropsScope() {
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));

        TranscriptBlock b;
        b.session = QStringLiteral("s1");
        b.seq = 4;
        b.kind = QStringLiteral("ToolResult");
        b.call_id = QStringLiteral("c9");
        b.ok = true;
        b.summary = QStringLiteral("patched");
        b.detail_kind = QStringLiteral("fs.diff");
        b.detail_body = QStringLiteral("{'path':'a'}"); // structured detail body (opaque string)

        WriteBatch wb;
        WindowRowWrite w;
        w.kind = QStringLiteral("TranscriptBlock");
        w.scope = QStringLiteral("s1");
        w.cursor = b.seq;
        // The write-behind's canonical payload shape: the Q_GADGET property dump as compact JSON.
        QJsonObject payload;
        payload.insert(QStringLiteral("session"), b.session);
        payload.insert(QStringLiteral("seq"), static_cast<double>(b.seq));
        payload.insert(QStringLiteral("kind"), b.kind);
        payload.insert(QStringLiteral("call_id"), b.call_id);
        payload.insert(QStringLiteral("ok"), b.ok);
        payload.insert(QStringLiteral("summary"), b.summary);
        payload.insert(QStringLiteral("detail_kind"), b.detail_kind);
        payload.insert(QStringLiteral("detail_body"), b.detail_body);
        w.payload = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        w.lastRev = 1;
        wb.windowUpserts.push_back(w);
        wb.journalRecords.push_back(
            rec(1, QStringLiteral("s1\x1f") + QString::number(b.seq), JournalOp::Upsert));
        QVERIFY2(p.writeBehind(wb), qPrintable(p.lastError()));

        MirrorModel model;
        QVERIFY(p.loadInto(model));
        const TranscriptBlockScope scope{QStringLiteral("s1")};
        QVERIFY(model.transcripts.count(scope) != 0U);
        const Window<TranscriptBlock>& win = model.transcripts[scope];
        QCOMPARE(win.items.size(), std::size_t(1));
        QCOMPARE(win.items[0].get().kind, QStringLiteral("ToolResult"));
        QCOMPARE(win.items[0].get().summary, QStringLiteral("patched"));
        QCOMPARE(win.items[0].get().detail_kind, QStringLiteral("fs.diff"));
        QCOMPARE(win.items[0].get().detail_body, b.detail_body);
        QCOMPARE(win.meta.newest_cursor, quint64(4));

        // The rebaseline wipe: a scope clear drops the persisted rows (nothing resurrects).
        WriteBatch clear;
        clear.transcriptWindowClears.push_back(QStringLiteral("s1"));
        clear.journalRecords.push_back(rec(2, QStringLiteral("s1"), JournalOp::Tombstone));
        QVERIFY(p.writeBehind(clear));
        MirrorModel wiped;
        QVERIFY(p.loadInto(wiped));
        QCOMPARE(wiped.transcripts.count(scope), std::size_t(0));
    }

    // AD (1a): the tree/hub M-tables (adapters / transport accounts / person endpoints) persist
    // and reload like the other M rows — the offline integrations tree / channels hub source.
    void treeHubTablesRoundTrip() {
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));

        WriteBatch wb;
        Adapter a;
        a.family = QStringLiteral("matrix");
        a.display_name = QStringLiteral("Matrix");
        a.directory = true;
        a.cap_rooms = true;
        a.cap_direct_messages = true;
        a.ops_json = QStringLiteral("{\"rosterOps\":{\"list\":true}}");
        wb.adapterUpserts.push_back(a);
        TransportAccount t;
        t.transport = QStringLiteral("matrix/@you:hs");
        t.family = QStringLiteral("matrix");
        t.display_name = QStringLiteral("@you:hs");
        t.connection = QStringLiteral("connected");
        t.presence = QStringLiteral("available");
        t.enabled = true;
        wb.transportAccountUpserts.push_back(t);
        PersonEndpoint e;
        e.person = QStringLiteral("p-alice");
        e.transport = t.transport;
        e.contact = QStringLiteral("@alice:hs");
        e.display_name = QStringLiteral("Alice");
        e.presence_primitive = QStringLiteral("available");
        wb.personEndpointUpserts.push_back(e);
        wb.journalRecords.push_back(rec(1, QStringLiteral("matrix"), JournalOp::Upsert));
        QVERIFY2(p.writeBehind(wb), qPrintable(p.lastError()));

        MirrorModel model;
        QVERIFY(p.loadInto(model));
        const Adapter* la = model.adapters.find(AdapterKey{QStringLiteral("matrix")});
        QVERIFY(la != nullptr);
        QCOMPARE(la->display_name, QStringLiteral("Matrix"));
        QVERIFY(la->directory);
        QCOMPARE(la->ops_json, a.ops_json);
        const TransportAccount* lt =
            model.transport_accounts.find(TransportAccountKey{t.transport});
        QVERIFY(lt != nullptr);
        QCOMPARE(lt->connection, QStringLiteral("connected"));
        QVERIFY(lt->enabled);
        const PersonEndpoint* le = model.person_endpoints.find(e.key());
        QVERIFY(le != nullptr);
        QCOMPARE(le->display_name, QStringLiteral("Alice"));
        QCOMPARE(le->presence_primitive, QStringLiteral("available"));

        // Tombstones drop the rows.
        WriteBatch del;
        del.adapterTombstones.push_back(AdapterKey{QStringLiteral("matrix")});
        del.transportAccountTombstones.push_back(TransportAccountKey{t.transport});
        del.personEndpointTombstones.push_back(e.key());
        del.journalRecords.push_back(rec(2, QStringLiteral("matrix"), JournalOp::Tombstone));
        QVERIFY(p.writeBehind(del));
        MirrorModel wiped;
        QVERIFY(p.loadInto(wiped));
        QCOMPARE(wiped.adapters.size(), std::size_t(0));
        QCOMPARE(wiped.transport_accounts.size(), std::size_t(0));
        QCOMPARE(wiped.person_endpoints.size(), std::size_t(0));
    }

    void tombstoneWriteBehindDeletesRow() {
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));
        WriteBatch add;
        add.conversationUpserts.push_back(
            conv(QStringLiteral("m"), QStringLiteral("!x"), QStringLiteral("X")));
        add.journalRecords.push_back(rec(1, QStringLiteral("m\x1f!x"), JournalOp::Upsert));
        QVERIFY(p.writeBehind(add));

        WriteBatch del;
        del.conversationTombstones.push_back(
            ConversationKey{QStringLiteral("m"), QStringLiteral("!x")});
        del.journalRecords.push_back(rec(2, QStringLiteral("m\x1f!x"), JournalOp::Tombstone));
        QVERIFY(p.writeBehind(del));

        MirrorModel model;
        QVERIFY(p.loadInto(model));
        QCOMPARE(model.conversations.size(), std::size_t(0));
        QCOMPARE(p.persistedJournalHead(), quint64(2));
    }

    void compactionSweepDeletesBelowRev() {
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));
        WriteBatch wb;
        for (quint64 r = 1; r <= 10; ++r) {
            wb.journalRecords.push_back(
                rec(r, QStringLiteral("m\x1f!%1").arg(r), JournalOp::Upsert));
        }
        QVERIFY(p.writeBehind(wb));
        const std::size_t dropped = p.compactJournal(/*deleteBelowRev=*/8);
        QCOMPARE(dropped, std::size_t(7)); // revs 1..7
        // head unchanged (8,9,10 remain)
        QCOMPARE(p.persistedJournalHead(), quint64(10));
    }

    void schemaMismatchDropsAndRebuilds() {
        // Disposable-cache posture (§4.5 / 07§4.1): a schema-version bump drops + rebuilds.
        {
            Persistence p;
            const auto pth = paths();
            QVERIFY2(p.open(pth), qPrintable(p.lastError()));
            WriteBatch wb;
            wb.conversationUpserts.push_back(
                conv(QStringLiteral("m"), QStringLiteral("!a"), QStringLiteral("A")));
            wb.journalRecords.push_back(rec(1, QStringLiteral("m\x1f!a"), JournalOp::Upsert));
            QVERIFY(p.writeBehind(wb));
            p.close();
        }
        // Tamper the persisted schema_version to an older value.
        {
            const QString conn = QStringLiteral("tamper");
            {
                QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
                db.setDatabaseName(mirrorPath());
                QVERIFY(db.open());
                QSqlQuery q(db);
                QVERIFY(q.exec(
                    QStringLiteral("UPDATE mirror_meta SET v='5' WHERE k='schema_version'")));
                db.close();
            }
            QSqlDatabase::removeDatabase(conn);
        }
        // Reopen: mismatch => drop + rebuild => data gone, version reset.
        Persistence p;
        const auto pth = paths();
        QVERIFY2(p.open(pth), qPrintable(p.lastError()));
        QCOMPARE(p.schemaVersion(), Persistence::kSchemaVersion);
        MirrorModel model;
        QVERIFY(p.loadInto(model));
        QCOMPARE(model.conversations.size(), std::size_t(0));
        QCOMPARE(p.persistedJournalHead(), quint64(0));
    }
};

QTEST_MAIN(TstMirrorPersistence)
#include "tst_mirror_persistence.moc"
