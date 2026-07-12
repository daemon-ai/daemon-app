// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// A2 (mirror architecture): the durable outbox + local-<id>.db module (spec 09 §6, §3.4, §4.5;
// ADR-005/006). Covers UUIDv7 minting (§6.2), the versioned+migrated local db (§4.5, never
// dropped), the outbox state machine (lanes/FIFO/backoff/gates/rejection/confirm/bounds §6.1-§6.6),
// crash-injection across the enqueue|send|confirm phases + boot reconciliation (§6.6), the disabled
// auto-replay gate (§6.8), and the PendingOpsModel lens (§6.5/§8.4).
//
// Ported behaviors (license-disciplined — Sink/Akonadi are (L)GPL; behaviors RE-EXPRESSED, no
// source text copied):
//  - queue ordering / concurrency  <- sink/tests/messagequeuetest.cpp:38-46,112-155
//  - transient-vs-permanent replay  <- sink/tests/synchronizertest.cpp:102-210
//                                      + common/synchronizer.cpp:783-800 (the error split)
//  - replay-after-restart contract  <- akonadi/autotests/libs/changerecordertest.cpp:45-175

#include "local_database.h"
#include "outbox.h"
#include "pending_ops_model.h"
#include "uuidv7.h"
#include "verb_class.h"

#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

using namespace mirror;

namespace {

// A hand-built "shipped v1" local.db (schema_version=1, sidecar_conversation WITHOUT `collapsed`),
// seeded with one outbox row + one sidecar row, so opening it with the current LocalDatabase proves
// the FORWARD migration preserves user data and NEVER drops the file (spec 09 §4.5).
void writeLegacyV1Db(const QString& path) {
    const QString conn = QStringLiteral("legacy-v1-writer");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(q.exec(QStringLiteral("CREATE TABLE local_meta(k TEXT PRIMARY KEY, v TEXT)")));
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE outbox_ops(op_id TEXT PRIMARY KEY, lane TEXT NOT NULL, verb TEXT NOT "
            "NULL, payload BLOB NOT NULL, display TEXT NOT NULL, state INTEGER NOT NULL, attempts "
            "INTEGER NOT NULL DEFAULT 0, enqueued_ms INTEGER NOT NULL, last_error TEXT, schema_v "
            "INTEGER NOT NULL)")));
        // v1 sidecar_conversation: note NO `collapsed` column (added in v2).
        QVERIFY(q.exec(QStringLiteral(
            "CREATE TABLE sidecar_conversation(key TEXT PRIMARY KEY, unread_count INTEGER NOT NULL "
            "DEFAULT 0, last_read_cursor INTEGER NOT NULL DEFAULT 0, muted INTEGER NOT NULL "
            "DEFAULT "
            "0, draft TEXT, draft_refs TEXT, updated_ms INTEGER NOT NULL DEFAULT 0)")));
        QVERIFY(q.exec(QStringLiteral(
            "INSERT INTO outbox_ops(op_id,lane,verb,payload,display,state,attempts,enqueued_ms,"
            "schema_v) "
            "VALUES('op-legacy','chat-send\037matrix','ConvSend','xx','hi',0,0,1000,1)")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO sidecar_conversation(key,unread_count,draft) "
                                      "VALUES('matrix\037!r',3,'wip')")));
        QVERIFY(q.exec(QStringLiteral("INSERT INTO local_meta(k,v) VALUES('schema_version','1')")));
        db.close();
    }
    QSqlDatabase::removeDatabase(conn);
}

} // namespace

class TestOutbox : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        qRegisterMetaType<mirror::PendingOp>("mirror::PendingOp");
    }

    void init() {
        m_dir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_dir->isValid());
    }

    void cleanup() { m_dir.reset(); }

    QString dbPath(const QString& name = QStringLiteral("local.db")) const {
        return m_dir->filePath(name);
    }

    // ---- UUIDv7 (spec 09 §6.2) --------------------------------------------------------------
    void uuidv7ShapeAndVariant() {
        const QString u = generateUuidV7(0x0189FEDCBA98ULL);
        // Canonical 8-4-4-4-12 with hyphens.
        QCOMPARE(u.size(), 36);
        QCOMPARE(u.count(QLatin1Char('-')), 4);
        const QString hex = u.left(13).remove(QLatin1Char('-')); // first 48 bits = the timestamp
        QCOMPARE(hex, QStringLiteral("0189fedcba98"));
        // Version nibble (first char of the 3rd group) == '7'.
        const QStringList groups = u.split(QLatin1Char('-'));
        QCOMPARE(groups.size(), 5);
        QCOMPARE(groups.at(2).at(0), QLatin1Char('7'));
        // Variant: first char of the 4th group is one of 8/9/a/b (0b10xx).
        const QChar var = groups.at(3).at(0);
        QVERIFY(var == QLatin1Char('8') || var == QLatin1Char('9') || var == QLatin1Char('a') ||
                var == QLatin1Char('b'));
    }

    void uuidv7TimeOrderedAndUnique() {
        const QString a = generateUuidV7(1000);
        const QString b = generateUuidV7(2000);
        QVERIFY(a < b); // lexicographic order follows time (lane FIFO survives export)
        QSet<QString> seen;
        for (int i = 0; i < 500; ++i) {
            seen.insert(generateUuidV7());
        }
        QCOMPARE(seen.size(), 500); // collision-free
    }

    // ---- verb-class table (spec 09 §6.4) ---------------------------------------------------
    void verbClassTable() {
        QCOMPARE(verbClass(QStringLiteral("ConvSend")), VerbClass::ChatSend);
        QCOMPARE(verbClass(QStringLiteral("ConvSetTopic")), VerbClass::ConvMeta);
        QCOMPARE(verbClass(QStringLiteral("ConvSetTitle")), VerbClass::ConvMeta);
        QCOMPARE(verbClass(QStringLiteral("RosterAdd")), VerbClass::RosterEdit);
        QCOMPARE(verbClass(QStringLiteral("ContactSetAlias")), VerbClass::RosterEdit);
        QCOMPARE(verbClass(QStringLiteral("SessionUpdateMeta")), VerbClass::SessionMeta);
        QCOMPARE(verbClass(QStringLiteral("TurnPrompt")), VerbClass::TurnPrompt);
        QCOMPARE(verbClass(QStringLiteral("Nonsense")), VerbClass::Unknown);
        // Lane kinds: wire verbs are mutation lanes; turn-prompt is a dispatch lane (§6.6).
        QCOMPARE(laneKind(VerbClass::ChatSend), LaneKind::Mutation);
        QCOMPARE(laneKind(VerbClass::TurnPrompt), LaneKind::Dispatch);
        QVERIFY(isCoalescing(VerbClass::ConvMeta));
        QVERIFY(!isCoalescing(VerbClass::ChatSend));
        QVERIFY(requiresConnected(VerbClass::ChatSend));
        QVERIFY(!requiresConnected(VerbClass::TurnPrompt));
        // Error classes (§6.5): the closed v38 set + fail-visible default.
        QCOMPARE(classifyError(QStringLiteral("Unauthenticated")), ErrorClass::Unauthenticated);
        QCOMPARE(classifyError(QStringLiteral("Timeout")), ErrorClass::Transient);
        QCOMPARE(classifyError(QStringLiteral("Forbidden")), ErrorClass::Rejection);
        QCOMPARE(classifyError(QStringLiteral("SomethingNew")), ErrorClass::Rejection);
    }

    // ---- local-<id>.db: versioned + migrated, never dropped (spec 09 §4.5) -----------------
    void localDbFreshSchema() {
        LocalDatabase db(dbPath());
        QVERIFY(db.isOpen());
        QCOMPARE(db.schemaVersion(), LocalDatabase::kSchemaVersion);
        QVERIFY(db.hasColumn(QStringLiteral("sidecar_conversation"), QStringLiteral("collapsed")));
    }

    void localDbMigratesV1WithoutDrop() {
        const QString path = dbPath();
        writeLegacyV1Db(path);
        const qint64 sizeBefore = QFileInfo(path).size();
        {
            LocalDatabase db(path);
            QVERIFY(db.isOpen());
            // Migrated forward to current.
            QCOMPARE(db.schemaVersion(), LocalDatabase::kSchemaVersion);
            // The v2 ALTER added `collapsed`.
            QVERIFY(
                db.hasColumn(QStringLiteral("sidecar_conversation"), QStringLiteral("collapsed")));
            // The outbox row survived (never dropped, unlike the cache).
            const QList<PendingOp> ops = db.loadOps();
            QCOMPARE(ops.size(), 1);
            QCOMPARE(ops.first().opId, QStringLiteral("op-legacy"));
            // The sidecar row survived.
            const SidecarConversation sc = db.sidecarConversation(QStringLiteral("matrix\037!r"));
            QCOMPARE(sc.unreadCount, 3);
            QCOMPARE(sc.draft, QStringLiteral("wip"));
        }
        QVERIFY(QFileInfo::exists(path)); // the precious db was never deleted
        QVERIFY(QFileInfo(path).size() >= sizeBefore);
    }

    // ---- sidecar tables (spec 09 §3.4) -----------------------------------------------------
    void sidecarRoundTrip() {
        LocalDatabase db(dbPath());
        SidecarConversation c;
        c.key = QStringLiteral("matrix\037!room");
        c.unreadCount = 7;
        c.draft = QStringLiteral("draft body");
        c.muted = true;
        c.collapsed = true;
        c.updatedMs = 42;
        QVERIFY(db.upsertSidecarConversation(c));
        const SidecarConversation got = db.sidecarConversation(c.key);
        QCOMPARE(got.unreadCount, 7);
        QCOMPARE(got.draft, QStringLiteral("draft body"));
        QVERIFY(got.muted);
        QVERIFY(got.collapsed);

        SidecarSession s;
        s.key = QStringLiteral("sess-1");
        s.draft = QStringLiteral("half typed prompt");
        QVERIFY(db.upsertSidecarSession(s));
        QCOMPARE(db.sidecarSession(s.key).draft, QStringLiteral("half typed prompt"));
    }

    // ---- outbox: FIFO per lane, independent lanes, one in-flight/lane, global cap 4 ---------
    // (ported behavior: sink messagequeuetest ordering/concurrency)
    void outboxLaneFifoAndConcurrency() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        QSignalSpy sends(&ob, &Outbox::sendRequested);

        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        const QString a2 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a2"), QStringLiteral("a2"));
        ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037B"), QByteArrayLiteral("b1"),
                   QStringLiteral("b1"));
        QVERIFY(!a1.isEmpty());

        ob.drain();
        // One in-flight per lane: A's head + B's head.
        QCOMPARE(ob.inFlightCount(), 2);
        QCOMPARE(sends.count(), 2);
        // FIFO: lane A's first in-flight op is a1, not a2.
        QCOMPARE(ob.op(a1).state, OpState::Inflight);
        QCOMPARE(ob.op(a2).state, OpState::Pending);

        // Ack a1 (v38 terminal): a1 gone, then a2 drains as A's new head.
        ob.onAck(a1);
        QVERIFY(!ob.contains(a1));
        ob.drain();
        QCOMPARE(ob.op(a2).state, OpState::Inflight);
    }

    void outboxGlobalInFlightCap() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        for (int i = 0; i < 6; ++i) {
            ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037%1").arg(i),
                       QByteArrayLiteral("x"), QStringLiteral("x"));
        }
        ob.drain();
        QCOMPARE(ob.inFlightCount(), Outbox::kMaxInFlight); // capped at 4 across ready lanes
    }

    // ---- backoff schedule (spec 09 §6.3) ---------------------------------------------------
    void backoffSchedule() {
        QCOMPARE(Outbox::backoffMs(0), qint64(1000));
        QCOMPARE(Outbox::backoffMs(1), qint64(2000));
        QCOMPARE(Outbox::backoffMs(2), qint64(4000));
        QCOMPARE(Outbox::backoffMs(5), qint64(32000));
        QCOMPARE(Outbox::backoffMs(6), qint64(60000)); // capped
        QCOMPARE(Outbox::backoffMs(40), qint64(60000));
    }

    // Transient failure re-queues keeping FIFO order + honoring backoff.
    // (ported behavior: sink synchronizer transient path — retry later, keep the watermark)
    void outboxTransientRequeuesKeepingOrder() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        qint64 clock = 10'000;
        ob.setClock([&clock] { return clock; });
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"), QByteArrayLiteral("a2"),
                   QStringLiteral("a2"));
        ob.drain();
        QCOMPARE(ob.op(a1).state, OpState::Inflight);

        ob.onTransientFailure(a1, QStringLiteral("connection lost"));
        QCOMPARE(ob.op(a1).state, OpState::Pending);
        QCOMPARE(ob.op(a1).attempts, 1);

        // Immediately draining does nothing: a1 is still the FIFO head but inside its backoff
        // window, and FIFO forbids reordering a2 ahead of it.
        ob.drain();
        QCOMPARE(ob.inFlightCount(), 0);

        // After the backoff window, a1 (still the head) drains again.
        clock += Outbox::backoffMs(1) + 1;
        ob.drain();
        QCOMPARE(ob.op(a1).state, OpState::Inflight);
    }

    // ---- rejection contract (spec 09 §6.5): pause + retry/edit/discard/send-remaining --------
    // (ported behavior: sink synchronizer permanent-error path — but with the rejection story Sink
    // never finished: the lane pauses and stays visible instead of advance-and-forget)
    void outboxRejectionPausesLane() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"), QByteArrayLiteral("a2"),
                   QStringLiteral("a2"));
        const QString lane = ob.op(a1).lane;
        ob.drain();
        ob.onRejection(a1, QStringLiteral("Forbidden"), QStringLiteral("nope"));
        QCOMPARE(ob.op(a1).state, OpState::Rejected);
        QVERIFY(ob.lanePaused(lane));
        // Paused: nothing drains (order preserved over throughput).
        ob.drain();
        QCOMPARE(ob.inFlightCount(), 0);
    }

    void outboxRetryUnpauses() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        const QString lane = ob.op(a1).lane;
        ob.drain();
        ob.onRejection(a1, QStringLiteral("Conflict"), QStringLiteral("x"));
        ob.retry(a1); // same op-id -> pending, lane unpaused (dedup-safe)
        QCOMPARE(ob.op(a1).state, OpState::Pending);
        QVERIFY(!ob.lanePaused(lane));
        ob.drain();
        QCOMPARE(ob.op(a1).state, OpState::Inflight);
    }

    void outboxEditDiscardSendRemaining() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        // edit: takeForEdit returns the payload and removes the op (caller mints a NEW op-id).
        const QString e1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("hello"), QStringLiteral("hello"));
        const QByteArray payload = ob.takeForEdit(e1);
        QCOMPARE(payload, QByteArrayLiteral("hello"));
        QVERIFY(!ob.contains(e1));

        // discard: just removes.
        const QString d1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("x"), QStringLiteral("x"));
        ob.discard(d1);
        QVERIFY(!ob.contains(d1));

        // send-remaining-anyway: after a rejection the paused head is dropped and the lane resumes.
        const QString r1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037B"),
                                      QByteArrayLiteral("r1"), QStringLiteral("r1"));
        const QString r2 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037B"),
                                      QByteArrayLiteral("r2"), QStringLiteral("r2"));
        const QString lane = ob.op(r1).lane;
        ob.drain();
        ob.onRejection(r1, QStringLiteral("Unsupported"), QStringLiteral("x"));
        ob.sendRemainingAnyway(lane);
        QVERIFY(!ob.contains(r1));
        QVERIFY(!ob.lanePaused(lane));
        ob.drain();
        QCOMPARE(ob.op(r2).state, OpState::Inflight);
    }

    // ---- confirm contract (spec 09 §6.6): provenance-keyed, uniform across lanes -------------
    void outboxConfirmV38TerminalOnAck() {
        LocalDatabase db(dbPath());
        Outbox ob(&db); // default: NOT provenance-capable (api < 39)
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.drain();
        ob.onAck(a1); // v38: an ack is terminal success -> row deleted
        QVERIFY(!ob.contains(a1));
    }

    void outboxConfirmProvenanceKeyed() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setProvenanceCapable(true); // api >= 39
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.drain();
        ob.onAck(a1); // -> accepted, awaiting provenance
        QCOMPARE(ob.op(a1).state, OpState::Accepted);
        ob.onProvenanceStamped(a1); // provenance-stamped delta -> landed -> deleted
        QVERIFY(!ob.contains(a1));

        // The SAME contract on a dispatch (turn-prompt) lane — no per-verb matching.
        const QString t1 = ob.enqueue(QStringLiteral("TurnPrompt"), QStringLiteral("sess-1"),
                                      QByteArrayLiteral("prompt"), QStringLiteral("prompt"));
        ob.drain();
        ob.onProvenanceStamped(t1); // provenance can arrive before the ack
        QVERIFY(!ob.contains(t1));
    }

    void outboxNullOriginUnattributed() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setProvenanceCapable(true);
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.drain();
        ob.onAck(a1);
        ob.onProvenanceStamped(QString()); // null origin: unattributed -> no-op, a1 survives
        QVERIFY(ob.contains(a1));
        QCOMPARE(ob.op(a1).state, OpState::Accepted);
    }

    void outboxAcceptedExpiry() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setProvenanceCapable(true);
        qint64 clock = 1'000'000;
        ob.setClock([&clock] { return clock; });
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.drain();
        ob.onAck(a1); // accepted at clock=1'000'000
        clock += 5 * 60 * 1000;
        ob.expireAcceptedOps(clock, 10 * 60 * 1000); // 5 min < 10 min grace: still present
        QVERIFY(ob.contains(a1));
        clock += 6 * 60 * 1000;
        ob.expireAcceptedOps(clock, 10 * 60 * 1000); // now > grace: disposed
        QVERIFY(!ob.contains(a1));
    }

    // ---- bounds (spec 09 §6.1): never a silent drop --------------------------------------
    void outboxPerLaneBound() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        QSignalSpy rejected(&ob, &Outbox::enqueueRejected);
        for (int i = 0; i < Outbox::kMaxPerLane; ++i) {
            const QString id = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                          QByteArrayLiteral("x"), QStringLiteral("x"));
            QVERIFY(!id.isEmpty());
        }
        const QString over = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                        QByteArrayLiteral("x"), QStringLiteral("x"));
        QVERIFY(over.isEmpty()); // failed the verb, not a silent drop
        QCOMPARE(rejected.count(), 1);
        QCOMPARE(ob.laneDepth(buildLane(VerbClass::ChatSend, QStringLiteral("t\037A"))),
                 Outbox::kMaxPerLane);
    }

    // ---- conv-meta latest-wins coalescing (spec 09 §6.4) ----------------------------------
    void outboxConvMetaCoalesces() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        const QString first = ob.enqueue(QStringLiteral("ConvSetTopic"), QStringLiteral("t\037A"),
                                         QByteArrayLiteral("topic-1"), QStringLiteral("topic-1"));
        const QString second = ob.enqueue(QStringLiteral("ConvSetTopic"), QStringLiteral("t\037A"),
                                          QByteArrayLiteral("topic-2"), QStringLiteral("topic-2"));
        // Latest-wins per (verb, target): the second edit replaced the pending first.
        QCOMPARE(second, first); // coalesced in place (same op-id kept)
        QCOMPARE(ob.op(first).payload, QByteArrayLiteral("topic-2"));
        QCOMPARE(ob.laneDepth(buildLane(VerbClass::ConvMeta, QStringLiteral("t\037A"))), 1);
    }

    // ---- crash-injection across enqueue|send|confirm (spec 09 §6.6) ------------------------
    void crashAfterEnqueueBeforeSend() {
        const QString path = dbPath();
        QString opId;
        {
            LocalDatabase db(path);
            Outbox ob(&db);
            ob.load();
            opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                              QByteArrayLiteral("payload"), QStringLiteral("hi"));
            // CRASH here — the enqueue transaction committed BEFORE any send (the durability
            // point).
        }
        LocalDatabase db2(path);
        Outbox ob2(&db2);
        ob2.load();
        QVERIFY(ob2.contains(opId));
        QCOMPARE(ob2.op(opId).state, OpState::Pending);
        QCOMPARE(ob2.op(opId).payload, QByteArrayLiteral("payload"));
    }

    void crashWhileInflightRevertsToPending() {
        const QString path = dbPath();
        QString opId;
        {
            LocalDatabase db(path);
            Outbox ob(&db);
            ob.load();
            ob.setGate([](const QString&, VerbClass) { return true; });
            opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                              QByteArrayLiteral("p"), QStringLiteral("p"));
            ob.drain(); // -> inflight (handed to the wire)
            QCOMPARE(ob.op(opId).state, OpState::Inflight);
            // CRASH mid-flight.
        }
        LocalDatabase db2(path);
        Outbox ob2(&db2);
        ob2.load(); // boot: inflight -> pending (at-least-once)
        QCOMPARE(ob2.op(opId).state, OpState::Pending);
    }

    void crashAfterAcceptReconciledByProvenance() {
        const QString path = dbPath();
        QString opId;
        {
            LocalDatabase db(path);
            Outbox ob(&db);
            ob.load();
            ob.setProvenanceCapable(true);
            ob.setGate([](const QString&, VerbClass) { return true; });
            opId = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                              QByteArrayLiteral("p"), QStringLiteral("p"));
            ob.drain();
            ob.onAck(opId); // accepted, awaiting provenance
            // CRASH between accept and the provenance-stamped delta landing.
        }
        LocalDatabase db2(path);
        Outbox ob2(&db2);
        ob2.load();
        QCOMPARE(ob2.op(opId).state, OpState::Accepted); // survived, still awaiting provenance
        // Two-phase landing: the mirror journal already persisted this op-id's provenance; boot
        // reconciliation deletes it (the journal-scan that SUPPLIES the set is A1's seam).
        ob2.reconcileLandedOps({opId});
        QVERIFY(!ob2.contains(opId));
    }

    // Replay-after-restart FIFO contract (ported behavior: akonadi changerecorder reload+replay).
    void crashReplayAfterRestartPreservesFifo() {
        const QString path = dbPath();
        QStringList ids;
        {
            LocalDatabase db(path);
            Outbox ob(&db);
            ob.load();
            for (int i = 0; i < 3; ++i) {
                ids << ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                  QByteArrayLiteral("x"), QStringLiteral("e%1").arg(i));
            }
        }
        // "reload": a fresh outbox off the same durable file replays in the same FIFO order.
        LocalDatabase db2(path);
        Outbox ob2(&db2);
        ob2.load();
        ob2.setGate([](const QString&, VerbClass) { return true; });
        QSignalSpy sends(&ob2, &Outbox::sendRequested);
        ob2.drain();
        QCOMPARE(sends.count(), 1); // one in-flight per lane
        const auto sent = qvariant_cast<mirror::PendingOp>(sends.at(0).at(0));
        QCOMPARE(sent.opId, ids.first()); // the ORIGINAL head, order preserved across the restart
    }

    // ---- auto-replay gate flips on rung 3 (spec 09 §6.8) -----------------------------------
    void autoReplayGateEnabledFromApi39() {
        // BR flip: auto-replay (unattended drain after reconnect) is enabled per connection iff the
        // node advertises api/<N> with N >= 39 (dedup + provenance shipped, §10.3). Against api/38
        // it holds — a blind resend can duplicate (conv_send has no v38 dedup) — and only a manual
        // tap drains.
        QVERIFY(!Outbox::autoReplayEnabled(38));
        QVERIFY(Outbox::autoReplayEnabled(39));
        QVERIFY(Outbox::autoReplayEnabled(40));

        // MANUAL drain always works, on every api version (the outbox ships fully now).
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        ob.setGate([](const QString&, VerbClass) { return true; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.drain(); // manual tap
        QCOMPARE(ob.op(a1).state, OpState::Inflight);
    }

    // ---- auto-replay drains a held lane on reconnect (spec 09 §6.8) ------------------------
    void autoReplayDrainsHeldLaneOnReconnect() {
        // The graph's reconnect handler calls drain() when autoReplayEnabled(api). Simulate it: a
        // wire-lane op sits pending while the gate is closed (offline), then the api/39 reconnect
        // drain advances the lane head to inflight — unattended (no user tap).
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        bool connected = false;
        ob.setGate([&connected](const QString&, VerbClass) { return connected; });
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("a1"));
        ob.drain(); // offline: the gated wire lane holds
        QCOMPARE(ob.op(a1).state, OpState::Pending);
        connected = true;
        if (Outbox::autoReplayEnabled(39)) {
            ob.drain(); // reconnect on api/39 ⇒ unattended replay
        }
        QCOMPARE(ob.op(a1).state, OpState::Inflight);
    }

    // ---- PendingOpsModel lens (spec 09 §6.5/§8.4) ------------------------------------------
    void pendingModelExactRowOps() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        PendingOpsModel model;
        auto* tester = new QAbstractItemModelTester(
            &model, QAbstractItemModelTester::FailureReportingMode::QtTest, this);
        Q_UNUSED(tester);
        model.setOutbox(&ob);
        ob.setGate([](const QString&, VerbClass) { return true; });

        QSignalSpy inserted(&model, &QAbstractListModel::rowsInserted);
        const QString a1 = ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"),
                                      QByteArrayLiteral("a1"), QStringLiteral("hello"));
        QCOMPARE(model.count(), 1);
        QCOMPARE(inserted.count(), 1);
        QCOMPARE(model.data(model.index(0), PendingOpsModel::DisplayRole).toString(),
                 QStringLiteral("hello"));

        QSignalSpy changed(&model, &QAbstractListModel::dataChanged);
        ob.drain();
        ob.onRejection(a1, QStringLiteral("Forbidden"), QStringLiteral("nope"));
        QVERIFY(changed.count() >= 1);
        QCOMPARE(model.data(model.index(0), PendingOpsModel::StateRole).toInt(),
                 static_cast<int>(OpState::Rejected));
        QVERIFY(model.data(model.index(0), PendingOpsModel::RejectedRole).toBool());

        QSignalSpy removed(&model, &QAbstractListModel::rowsRemoved);
        ob.discard(a1);
        QCOMPARE(model.count(), 0);
        QCOMPARE(removed.count(), 1);
    }

    void pendingModelFilters() {
        LocalDatabase db(dbPath());
        Outbox ob(&db);
        PendingOpsModel model;
        model.setOutbox(&ob);
        model.setScopeFilter(QStringLiteral("t\037A")); // per-conversation strip (A5)
        ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037A"), QByteArrayLiteral("a"),
                   QStringLiteral("a"));
        ob.enqueue(QStringLiteral("ConvSend"), QStringLiteral("t\037B"), QByteArrayLiteral("b"),
                   QStringLiteral("b"));
        QCOMPARE(model.count(), 1); // only the matching-scope op
        QCOMPARE(model.data(model.index(0), PendingOpsModel::DisplayRole).toString(),
                 QStringLiteral("a"));
    }

private:
    std::unique_ptr<QTemporaryDir> m_dir;
};

QTEST_MAIN(TestOutbox)
#include "tst_outbox.moc"
