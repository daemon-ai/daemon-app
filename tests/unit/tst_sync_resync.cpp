// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon_turn_engine.h"
#include "i_turn_engine.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CachedTranscriptBlockRow;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::NodeApiClient;
using daemonapp::test::WireMuxServer;

namespace {

// An empty LogPage carrying an explicit epoch: {"LogPage":{"entries":[],next_seq,head_seq,epoch}}.
QByteArray logPage(quint64 nextSeq, quint64 headSeq, quint64 epoch) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "LogPage");
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "entries");
    b.append(static_cast<char>(0x80)); // empty array
    daemonapp::test::cborText(b, "next_seq");
    daemonapp::test::cborUint(b, nextSeq);
    daemonapp::test::cborText(b, "head_seq");
    daemonapp::test::cborUint(b, headSeq);
    daemonapp::test::cborText(b, "epoch");
    daemonapp::test::cborUint(b, epoch);
    return b;
}

// A CBOR byte string (major 2) - the contract's `byte-array = bstr` (tool-detail body).
void cborBstr(QByteArray& b, const QByteArray& bytes) {
    const auto len = static_cast<quint64>(bytes.size());
    if (len < 24) {
        b.append(static_cast<char>(0x40 | static_cast<char>(len)));
    } else if (len <= 0xFF) {
        b.append(static_cast<char>(0x58));
        b.append(static_cast<char>(len));
    } else {
        b.append(static_cast<char>(0x59));
        b.append(static_cast<char>((len >> 8) & 0xFF));
        b.append(static_cast<char>(len & 0xFF));
    }
    b.append(bytes);
}

// A tool-result-view map: {"call_id","ok","summary","detail":{"kind","body"}} (CDDL order).
void cborToolResultView(QByteArray& b, const char* callId, bool ok, const QByteArray& summary,
                        const char* detailKind, const QByteArray& detailBody) {
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "call_id");
    daemonapp::test::cborText(b, callId);
    daemonapp::test::cborText(b, "ok");
    b.append(static_cast<char>(ok ? 0xF5 : 0xF4));
    daemonapp::test::cborText(b, "summary");
    daemonapp::test::cborTextLen(b, summary);
    daemonapp::test::cborText(b, "detail");
    b.append(static_cast<char>(0xA2));
    daemonapp::test::cborText(b, "kind");
    daemonapp::test::cborText(b, detailKind);
    daemonapp::test::cborText(b, "body");
    cborBstr(b, detailBody);
}

// A LogPage carrying one Outbound Event entry: a ToolFinished with a rich result (full content in
// `summary` + typed JSON `detail`) - the D1 payload the engine must plumb and persist.
QByteArray toolFinishedLogPage(quint64 seq, const QByteArray& summary, const char* detailKind,
                               const QByteArray& detailBody) {
    QByteArray ev; // {"ToolFinished": {"seq": N, "result": tool-result-view}}
    ev.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(ev, "ToolFinished");
    ev.append(static_cast<char>(0xA2));
    daemonapp::test::cborText(ev, "seq");
    daemonapp::test::cborUint(ev, seq);
    daemonapp::test::cborText(ev, "result");
    cborToolResultView(ev, "c9", true, summary, detailKind, detailBody);

    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "LogPage");
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "entries");
    b.append(static_cast<char>(0x81)); // array(1)
    b.append(static_cast<char>(0xA5)); // session-log-entry map(5)
    daemonapp::test::cborText(b, "seq");
    daemonapp::test::cborUint(b, seq);
    daemonapp::test::cborText(b, "direction");
    daemonapp::test::cborText(b, "Outbound");
    daemonapp::test::cborText(b, "origin");
    b.append(static_cast<char>(0xA2));
    daemonapp::test::cborText(b, "transport");
    daemonapp::test::cborText(b, "api");
    daemonapp::test::cborText(b, "scope");
    daemonapp::test::cborText(b, "Internal");
    daemonapp::test::cborText(b, "disposition");
    daemonapp::test::cborText(b, "Context");
    daemonapp::test::cborText(b, "payload");
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Event");
    b.append(ev);
    daemonapp::test::cborText(b, "next_seq");
    daemonapp::test::cborUint(b, seq + 1);
    daemonapp::test::cborText(b, "head_seq");
    daemonapp::test::cborUint(b, seq);
    daemonapp::test::cborText(b, "epoch");
    daemonapp::test::cborUint(b, 0);
    return b;
}

// One journal-record map(9) wrapping a transcript Block payload (CDDL order).
void cborJournalBlockRecord(QByteArray& b, quint64 cursor, quint64 seq, const QByteArray& block) {
    b.append(static_cast<char>(0xA9));
    daemonapp::test::cborText(b, "cursor");
    daemonapp::test::cborUint(b, cursor);
    daemonapp::test::cborText(b, "segment");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "seq");
    daemonapp::test::cborUint(b, seq);
    daemonapp::test::cborText(b, "epoch");
    daemonapp::test::cborUint(b, 1);
    daemonapp::test::cborText(b, "trace");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "kind");
    daemonapp::test::cborText(b, "block");
    daemonapp::test::cborText(b, "timestamp_ms");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "verified");
    b.append(static_cast<char>(0xF5));
    daemonapp::test::cborText(b, "payload");
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Block");
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "block");
    b.append(block);
}

// A durable Journal holding a ToolCall + its rich ToolResult (summary + typed detail), so a
// re-baseline replay must re-emit and re-cache the same rich card.
QByteArray journalWithToolDetail(const QByteArray& summary, const char* detailKind,
                                 const QByteArray& detailBody) {
    QByteArray toolCall; // {"ToolCall": {"call_id","name","args_summary"}}
    toolCall.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(toolCall, "ToolCall");
    toolCall.append(static_cast<char>(0xA3));
    daemonapp::test::cborText(toolCall, "call_id");
    daemonapp::test::cborText(toolCall, "c9");
    daemonapp::test::cborText(toolCall, "name");
    daemonapp::test::cborText(toolCall, "fs");
    daemonapp::test::cborText(toolCall, "args_summary");
    daemonapp::test::cborText(toolCall, "x");

    QByteArray toolResult; // {"ToolResult": tool-result-view}
    toolResult.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(toolResult, "ToolResult");
    cborToolResultView(toolResult, "c9", true, summary, detailKind, detailBody);

    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Journal");
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "entries");
    b.append(static_cast<char>(0x82)); // array(2)
    cborJournalBlockRecord(b, 1, 1, toolCall);
    cborJournalBlockRecord(b, 2, 2, toolResult);
    daemonapp::test::cborText(b, "next_cursor");
    daemonapp::test::cborUint(b, 3);
    daemonapp::test::cborText(b, "head_cursor");
    daemonapp::test::cborUint(b, 2);
    daemonapp::test::cborText(b, "sealed_after");
    b.append(static_cast<char>(0xF6)); // null
    return b;
}

// The first emitted `toolFinished` event map in the spy, or empty.
QVariantMap firstToolFinished(const QSignalSpy& events) {
    for (const QList<QVariant>& emission : events) {
        const QVariantList list = emission.first().toList();
        for (const QVariant& v : list) {
            const QVariantMap m = v.toMap();
            if (m.value(QStringLiteral("type")).toString() == QLatin1String("toolFinished")) {
                return m;
            }
        }
    }
    return {};
}

// An empty durable Journal: {"Journal":{entries:[],next_cursor:0,head_cursor:0,sealed_after:null}}.
QByteArray emptyJournal() {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Journal");
    b.append(static_cast<char>(0xA4));
    daemonapp::test::cborText(b, "entries");
    b.append(static_cast<char>(0x80));
    daemonapp::test::cborText(b, "next_cursor");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "head_cursor");
    daemonapp::test::cborUint(b, 0);
    daemonapp::test::cborText(b, "sealed_after");
    b.append(static_cast<char>(0xF6)); // null
    return b;
}

} // namespace

// Phase 2 (L2) client resync: the DaemonTurnEngine consumes a push Subscribe stream, tracks the
// session-activation epoch, and re-baselines from the durable journal on a generation change (an
// epoch bump or a Reset) - the fix for the daemon-restart desync - instead of silently stalling.
class TestSyncResync : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

    QString sock(const QString& name) { return m_tmp.filePath(name); }

private slots:
    // An epoch bump on the streamed LogPage triggers a journal re-baseline (a SessionHistory call
    // crosses) and finishes the turn cleanly rather than hanging.
    void epochBumpReBaselinesFromJournal() {
        const QString path = sock(QStringLiteral("epoch.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal()); // Submit-ack (non-error) + the SessionHistory reply
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000); // push stream opened
        const int callsBefore = fake.requestCount();

        fake.pushItem(logPage(0, 0, 0)); // establish epoch 0
        fake.pushItem(logPage(0, 0, 1)); // epoch 1 > 0 -> re-baseline

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
        // The re-baseline issued a SessionHistory one-shot (a new Call beyond Submit).
        QVERIFY(fake.requestCount() > callsBefore);
    }

    // A Reset frame for the active stream triggers the same journal re-baseline.
    void resetFrameReBaselines() {
        const QString path = sock(QStringLiteral("reset.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal());
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(logPage(0, 0, 0)); // establish epoch 0
        fake.pushReset(1, 0);            // generation change -> re-baseline

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
    }

    // A mid-turn transport drop (daemon alive) is recoverable: the stream ends with an error, the
    // engine suspends into "stalled" (still active, not finished), and reopens when the socket
    // returns - it does not re-baseline (same epoch) and does not die.
    void streamDropSuspendsAndReopens() {
        const QString path = sock(QStringLiteral("drop.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal());
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonTurnEngine engine(&client);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(logPage(0, 0, 0));
        QTRY_VERIFY_WITH_TIMEOUT(engine.active() && engine.turnState() == QLatin1String("thinking"),
                                 3000);
        const int opensBefore = fake.openCount();

        fake.stop(); // transport drop -> streamEnded(error) -> stalled
        QTRY_COMPARE_WITH_TIMEOUT(engine.turnState(), QString("stalled"), 5000);
        QVERIFY(engine.active());
        QCOMPARE(finished.count(), 0);

        QVERIFY2(fake.start(path), "relisten");
        // The backoff reopen issues a fresh Open on the new connection.
        QTRY_VERIFY_WITH_TIMEOUT(fake.openCount() > opensBefore, 8000);
        fake.pushItem(logPage(0, 0, 0)); // a good page lifts "stalled"
        QTRY_VERIFY_WITH_TIMEOUT(engine.active() && engine.turnState() == QLatin1String("thinking"),
                                 5000);
        QCOMPARE(finished.count(), 0);
        engine.cancel();
    }

    // D1 live path: a streamed ToolFinished carrying the rich result (full content in `summary` +
    // typed JSON `detail`) reaches the ingest event RAW and lands in the transcript cache row, so
    // the renderer projection and a later cold start see the same payload.
    void toolFinishedDetailPlumbsAndPersists() {
        const QString path = sock(QStringLiteral("detail.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(emptyJournal()); // Submit-ack (non-error)
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("detail-cache.db")));
        QVERIFY(cache.isOpen());
        DaemonTurnEngine engine(&client, &cache);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy events(&engine, &ITurnEngine::eventsEmitted);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);

        const QByteArray summary = QByteArrayLiteral("--- a/x\n+++ b/x\n@@ -1 +1 @@\n-a\n+b\n");
        const QByteArray body = QByteArrayLiteral(R"({"op":"edit","path":"x","count":1})");
        fake.pushItem(toolFinishedLogPage(1, summary, "fs", body));

        QTRY_VERIFY_WITH_TIMEOUT(!firstToolFinished(events).isEmpty(), 3000);
        const QVariantMap m = firstToolFinished(events);
        QCOMPARE(m.value(QStringLiteral("callId")).toString(), QStringLiteral("c9"));
        QCOMPARE(m.value(QStringLiteral("status")).toString(), QStringLiteral("ok"));
        QCOMPARE(m.value(QStringLiteral("summary")).toString(), QString::fromUtf8(summary));
        QCOMPARE(m.value(QStringLiteral("detailKind")).toString(), QStringLiteral("fs"));
        QCOMPARE(m.value(QStringLiteral("detailBody")).toString(), QString::fromUtf8(body));

        // The persisted row carries the same rich result (restart / scroll-back durability).
        const QList<CachedTranscriptBlockRow> rows = cache.transcriptBlocks(QStringLiteral("s1"));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.first().kind, QStringLiteral("ToolResult"));
        QCOMPARE(rows.first().summary, QString::fromUtf8(summary));
        QCOMPARE(rows.first().detailKind, QStringLiteral("fs"));
        QCOMPARE(rows.first().detailBody, body);
        engine.cancel();
    }

    // D1 journal path: a re-baseline replay re-emits the durable ToolResult's rich payload and
    // rebuilds the cache row with it - detail survives a daemon restart / reactivation.
    void journalReplayCarriesToolDetail() {
        const QString path = sock(QStringLiteral("jdetail.sock"));
        const QByteArray summary = QByteArrayLiteral("exit=0\n--- stdout ---\nok\n");
        const QByteArray body =
            QByteArrayLiteral(R"({"command":"ls","exit_code":0,"stdout_len":3,"stderr_len":0})");
        WireMuxServer fake;
        fake.setReplyPayload(journalWithToolDetail(summary, "shell", body));
        QVERIFY2(fake.start(path), "listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("jdetail-cache.db")));
        QVERIFY(cache.isOpen());
        DaemonTurnEngine engine(&client, &cache);
        engine.setSessionId(QStringLiteral("s1"));
        QSignalSpy events(&engine, &ITurnEngine::eventsEmitted);
        QSignalSpy finished(&engine, &ITurnEngine::turnFinished);

        engine.start(QStringLiteral("hi"));
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        fake.pushItem(logPage(0, 0, 0)); // establish epoch 0
        fake.pushItem(logPage(0, 0, 1)); // epoch bump -> journal re-baseline -> replay

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
        const QVariantMap m = firstToolFinished(events);
        QCOMPARE(m.value(QStringLiteral("summary")).toString(), QString::fromUtf8(summary));
        QCOMPARE(m.value(QStringLiteral("detailKind")).toString(), QStringLiteral("shell"));
        QCOMPARE(m.value(QStringLiteral("detailBody")).toString(), QString::fromUtf8(body));

        // The rebuilt durable cache retains the rich result on the ToolResult row.
        bool found = false;
        const QList<CachedTranscriptBlockRow> rows = cache.transcriptBlocks(QStringLiteral("s1"));
        for (const CachedTranscriptBlockRow& row : rows) {
            if (row.kind == QLatin1String("ToolResult")) {
                QCOMPARE(row.summary, QString::fromUtf8(summary));
                QCOMPARE(row.detailKind, QStringLiteral("shell"));
                QCOMPARE(row.detailBody, body);
                found = true;
            }
        }
        QVERIFY(found);
    }
};

QTEST_GUILESS_MAIN(TestSyncResync)
#include "tst_sync_resync.moc"
