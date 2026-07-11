// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// A7T — M4 sub-step 6 transcript re-homing parity harness (spec 09 §13; LEDGER-a7t).
//
// Drives the SAME coalesced transcript block stream through BOTH transcript paths:
//   legacy:  CachedTranscriptBlockRow -> DaemonCacheStore -> CachedSessionStore::content()
//   mirror:  CachedTranscriptBlockRow -> MirrorTranscriptSink -> ingestor -> w_transcript_blocks
//            -> MirrorSessionStore::mirrorContent()
// and asserts byte-equality of the msg-fence projection per representative scenario.
//
// The mirror path uses the PRODUCTION sink adapter (MirrorTranscriptSink) — the very mapping the
// live turn engine dual-writes through — so the harness proves the real re-homing, not a test
// fixture. Scenarios S1-S6/S9 prove byte-IDENTICAL parity; S7/S8 PIN the one structural divergence
// (the generated mirror::TranscriptBlock carries no argsSummary/detailKind/detailBody), which is
// exactly why the facade flip is withheld (LEDGER-a7t "ENTITY-FIELD GAP" / "Flip decision").

#include "daemon/cached_session_store.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/mirror_session_store.h"
#include "daemon/mirror_transcript_sink.h"
#include "mirror/generated/entities_map_gen.h"
#include "mirror/mirror_service.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::CachedSessionStore;
using daemonapp::daemon::CachedTranscriptBlockRow;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::MirrorSessionStore;
using daemonapp::daemon::MirrorTranscriptSink;

namespace {

constexpr auto kSession = "s-1";

CachedTranscriptBlockRow message(quint64 seq, const QString& role, const QString& text) {
    CachedTranscriptBlockRow r;
    r.sessionId = QStringLiteral("s-1");
    r.seq = seq;
    r.kind = QStringLiteral("Message");
    r.role = role;
    r.text = text;
    return r;
}

CachedTranscriptBlockRow reasoning(quint64 seq, const QString& text) {
    CachedTranscriptBlockRow r;
    r.sessionId = QStringLiteral("s-1");
    r.seq = seq;
    r.kind = QStringLiteral("Reasoning");
    r.text = text;
    return r;
}

CachedTranscriptBlockRow toolCall(quint64 seq, const QString& callId, const QString& name,
                                  const QString& argsSummary) {
    CachedTranscriptBlockRow r;
    r.sessionId = QStringLiteral("s-1");
    r.seq = seq;
    r.kind = QStringLiteral("ToolCall");
    r.callId = callId;
    r.toolName = name;
    r.argsSummary = argsSummary;
    return r;
}

CachedTranscriptBlockRow toolResult(quint64 seq, const QString& callId, bool ok,
                                    const QString& summary, const QString& detailKind = {},
                                    const QByteArray& detailBody = {}) {
    CachedTranscriptBlockRow r;
    r.sessionId = QStringLiteral("s-1");
    r.seq = seq;
    r.kind = QStringLiteral("ToolResult");
    r.callId = callId;
    r.ok = ok;
    r.summary = summary;
    r.detailKind = detailKind;
    r.detailBody = detailBody;
    return r;
}

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// A decoded wire ::journal_record carrying a Block payload (the SessionHistory reply entry the
// map_transcript_block wire-decode feed maps; §3.6). The caller stamps `session` from the request
// scope, so the record itself carries only seq/origin_op/payload.
::journal_record blockRecord(quint64 seq) {
    ::journal_record rec{};
    rec.journal_record_seq = seq;
    rec.journal_record_origin_op_present = false;
    rec.journal_record_payload.journal_record_payload_t_choice =
        ::journal_record_payload_t_r::journal_record_payload_t_journal_record_payload_block_m_c;
    return rec;
}

// One run through both projections over the same block stream.
struct Projections {
    QString legacy;
    QString mirror;
};

Projections project(const std::vector<CachedTranscriptBlockRow>& blocks) {
    QTemporaryDir dir;
    // Legacy path.
    DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
    // Mirror path.
    mirror::MirrorService svc;
    svc.openInMemory();
    MirrorTranscriptSink sink(&svc.ingestor());
    for (const CachedTranscriptBlockRow& b : blocks) {
        cache.upsertTranscriptBlock(b);
        sink.deliverBlock(b);
    }
    CachedSessionStore legacy(&cache, nullptr);
    MirrorSessionStore store(&svc.store(), &svc.ingestor());
    return {legacy.content(domain::SessionId(QLatin1String(kSession))),
            store.mirrorContent(domain::SessionId(QLatin1String(kSession)))};
}

} // namespace

class TstMirrorTranscriptParity : public QObject {
    Q_OBJECT

private slots:
    // S1: a single user message bubble.
    void s1_singleUserMessage() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("hello"))});
        QVERIFY(!p.legacy.isEmpty());
        QCOMPARE(p.mirror, p.legacy);
    }

    // S2: user + final assistant message (two bubbles).
    void s2_userThenAssistant() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("hi")),
                     message(2, QStringLiteral("Assistant"), QStringLiteral("there"))});
        QCOMPARE(p.mirror, p.legacy);
    }

    // S3: reasoning (opens an assistant group) then the final assistant message CONTINUES it.
    void s3_reasoningThenAssistantGroup() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("q")),
                     reasoning(2, QStringLiteral("thinking about it")),
                     message(3, QStringLiteral("Assistant"), QStringLiteral("answer"))});
        QCOMPARE(p.mirror, p.legacy);
    }

    // S4: tool call with EMPTY args + a plain-text ok result folded into the fence.
    void s4_toolCallEmptyArgsPlainResult() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("run it")),
                     toolCall(2, QStringLiteral("c1"), QStringLiteral("build"), QString()),
                     toolResult(3, QStringLiteral("c1"), true, QStringLiteral("done"))});
        QCOMPARE(p.mirror, p.legacy);
    }

    // S5: a tool call still running (no result) -> status "running".
    void s5_toolCallRunning() {
        const Projections p =
            project({toolCall(1, QStringLiteral("c1"), QStringLiteral("search"), QString())});
        QCOMPARE(p.mirror, p.legacy);
    }

    // S6: the `todo` tool call + result are suppressed (status-stack feed, not transcript).
    void s6_todoSuppressed() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("plan")),
                     toolCall(2, QStringLiteral("t1"), QStringLiteral("todo"), QString()),
                     toolResult(3, QStringLiteral("t1"), true, QStringLiteral("[]")),
                     message(4, QStringLiteral("Assistant"), QStringLiteral("planned"))});
        QCOMPARE(p.mirror, p.legacy);
        // The suppressed todo call must not leak a ```tool fence into either projection.
        QVERIFY(!p.mirror.contains(QStringLiteral("\"name\":\"todo\"")));
        QVERIFY(!p.legacy.contains(QStringLiteral("\"name\":\"todo\"")));
    }

    // S9: a journal rebaseline (clear + replay) yields the same projection as a direct feed, and
    // the mirror window is fully replaced (no stale-generation blocks survive the clear).
    void s9_rebaselineClearReplay() {
        QTemporaryDir dir;
        DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorTranscriptSink sink(&svc.ingestor());

        // Stale generation.
        for (const CachedTranscriptBlockRow& b :
             {message(5, QStringLiteral("User"), QStringLiteral("stale")),
              message(6, QStringLiteral("Assistant"), QStringLiteral("old"))}) {
            cache.upsertTranscriptBlock(b);
            sink.deliverBlock(b);
        }
        // Rebaseline: clear both, replay the durable generation.
        cache.clearTranscript(QLatin1String(kSession));
        sink.clear(QLatin1String(kSession));
        for (const CachedTranscriptBlockRow& b :
             {message(1, QStringLiteral("User"), QStringLiteral("fresh")),
              message(2, QStringLiteral("Assistant"), QStringLiteral("new"))}) {
            cache.upsertTranscriptBlock(b);
            sink.deliverBlock(b);
        }
        CachedSessionStore legacy(&cache, nullptr);
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        const domain::SessionId id{QLatin1String(kSession)};
        QCOMPARE(store.mirrorContent(id), legacy.content(id));
        QVERIFY(!store.mirrorContent(id).contains(QStringLiteral("stale")));
        QVERIFY(!store.mirrorContent(id).contains(QStringLiteral("old")));
    }

    // Reasoning coalescing: re-checkpointing the SAME seq updates the block in place (one fence,
    // the fuller text) rather than appending a second reasoning block.
    void reasoningReCheckpointUpsertsInPlace() {
        const Projections p =
            project({reasoning(2, QStringLiteral("abc")), reasoning(2, QStringLiteral("abcdef"))});
        QCOMPARE(p.mirror, p.legacy);
        QCOMPARE(p.mirror.count(QStringLiteral("```reasoning")), 1);
        QVERIFY(p.mirror.contains(QStringLiteral("abcdef")));
    }

    // upsertWindow is cursor-ordered: blocks delivered out of seq order land sorted, matching the
    // legacy cache's ORDER BY seq. (Defensive — the engine delivers in seq order.)
    void outOfOrderDeliverySortsBySeq() {
        QTemporaryDir dir;
        DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorTranscriptSink sink(&svc.ingestor());
        const std::vector<CachedTranscriptBlockRow> inOrder{
            message(1, QStringLiteral("User"), QStringLiteral("one")),
            message(2, QStringLiteral("Assistant"), QStringLiteral("two")),
            message(3, QStringLiteral("User"), QStringLiteral("three"))};
        for (const CachedTranscriptBlockRow& b : inOrder) {
            cache.upsertTranscriptBlock(b);
        }
        // Deliver to the mirror OUT of order.
        sink.deliverBlock(inOrder[2]);
        sink.deliverBlock(inOrder[0]);
        sink.deliverBlock(inOrder[1]);
        CachedSessionStore legacy(&cache, nullptr);
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        const domain::SessionId id{QLatin1String(kSession)};
        QCOMPARE(store.mirrorContent(id), legacy.content(id));
    }

    // --- G2 (M5): the ENTITY-FIELD GAP is CLOSED — these now go byte-identical (the flip signal)
    // -- S7: a tool call with NON-EMPTY args. The mirror entity now carries `args_summary`, so the
    // fence reproduces the legacy args byte-for-byte.
    void s7_toolCallNonEmptyArgsParity() {
        const Projections p = project(
            {toolCall(1, QStringLiteral("c1"), QStringLiteral("shell"), QStringLiteral("ls -la")),
             toolResult(2, QStringLiteral("c1"), true, QStringLiteral("out"))});
        QVERIFY(p.legacy.contains(QStringLiteral("ls -la"))); // legacy carries the args
        QVERIFY(p.mirror.contains(QStringLiteral("ls -la"))); // the entity now does too
        QCOMPARE(p.mirror, p.legacy);
    }

    // S8: a tool result with a structured detail (detailKind/detailBody). The mirror entity now
    // carries `detail_kind`/`detail_body`, so the fence folds the detail byte-for-byte.
    void s8_toolResultStructuredDetailParity() {
        const Projections p =
            project({toolCall(1, QStringLiteral("c1"), QStringLiteral("edit"), QString()),
                     toolResult(2, QStringLiteral("c1"), true, QStringLiteral("patched"),
                                QStringLiteral("fs.diff"), QByteArrayLiteral("{\"path\":\"a\"}"))});
        QVERIFY(p.legacy.contains(QStringLiteral("fs.diff"))); // legacy folds the detail
        QVERIFY(p.mirror.contains(QStringLiteral("fs.diff"))); // the entity now folds it too
        QCOMPARE(p.mirror, p.legacy);
    }

    // --- map_transcript_block (G2): the wire-decode / offline-reload feed ----------------------
    // The mapper's DTO is the SessionHistory reply's ::journal_record (its Block payload carries
    // the transcript-block union). The sink twin above covers the app-local stream; these cover
    // the wire arms, including the G2 enrichment (args_summary + tool-detail kind/body).

    void mapsWireMessageBlock() {
        static const QByteArray text = QByteArrayLiteral("hello from the wire");
        ::journal_record rec = blockRecord(4);
        static const QByteArray op = QByteArrayLiteral("op-123");
        rec.journal_record_origin_op_present = true;
        rec.journal_record_origin_op.journal_record_origin_op_choice =
            ::journal_record_origin_op_r::journal_record_origin_op_origin_op_m_c;
        setStr(rec.journal_record_origin_op.journal_record_origin_op_origin_op_m, op);
        auto& block = rec.journal_record_payload
                          .journal_record_payload_t_journal_record_payload_block_m.Block_block;
        block.transcript_block_choice = ::transcript_block_r::transcript_block_message_m_c;
        block.transcript_block_message_m.Message_role.transcript_role_choice =
            ::transcript_role_r::transcript_role_User_tstr_c;
        setStr(block.transcript_block_message_m.Message_text, text);

        const mirror::TranscriptBlock out = mirror::map_transcript_block(rec);
        QCOMPARE(out.seq, quint64{4});
        QCOMPARE(out.kind, QStringLiteral("Message"));
        QCOMPARE(out.role, QStringLiteral("User"));
        QCOMPARE(out.text, QStringLiteral("hello from the wire"));
        QCOMPARE(out.origin_op, QStringLiteral("op-123"));
        QVERIFY(out.session.isEmpty()); // the caller stamps the request scope (§3.6)
    }

    void mapsWireToolCallWithArgs() {
        static const QByteArray cid = QByteArrayLiteral("c-9");
        static const QByteArray name = QByteArrayLiteral("shell");
        static const QByteArray args = QByteArrayLiteral("ls -la");
        ::journal_record rec = blockRecord(5);
        auto& block = rec.journal_record_payload
                          .journal_record_payload_t_journal_record_payload_block_m.Block_block;
        block.transcript_block_choice = ::transcript_block_r::transcript_block_tool_call_m_c;
        ::transcript_block_tool_call& tc = block.transcript_block_tool_call_m;
        setStr(tc.ToolCall_call_id, cid);
        setStr(tc.ToolCall_name, name);
        setStr(tc.ToolCall_args_summary, args); // the G2 enrichment: args ride the entity
        tc.ToolCall_detail_present = false;

        const mirror::TranscriptBlock out = mirror::map_transcript_block(rec);
        QCOMPARE(out.kind, QStringLiteral("ToolCall"));
        QCOMPARE(out.call_id, QStringLiteral("c-9"));
        QCOMPARE(out.name, QStringLiteral("shell"));
        QCOMPARE(out.args_summary, QStringLiteral("ls -la"));
        QVERIFY(out.detail_kind.isEmpty());
    }

    void mapsWireToolResultWithStructuredDetail() {
        static const QByteArray cid = QByteArrayLiteral("c-9");
        static const QByteArray summary = QByteArrayLiteral("patched");
        static const QByteArray dkind = QByteArrayLiteral("fs.diff");
        static const QByteArray dbody = QByteArrayLiteral("{\"path\":\"a\"}");
        ::journal_record rec = blockRecord(6);
        auto& block = rec.journal_record_payload
                          .journal_record_payload_t_journal_record_payload_block_m.Block_block;
        block.transcript_block_choice = ::transcript_block_r::transcript_block_tool_result_m_c;
        ::transcript_block_tool_result& tr = block.transcript_block_tool_result_m;
        setStr(tr.ToolResult_call_id, cid);
        tr.ToolResult_ok = true;
        setStr(tr.ToolResult_summary, summary);
        tr.ToolResult_detail_present = true;
        tr.ToolResult_detail.ToolResult_detail_choice =
            ::ToolResult_detail_r::ToolResult_detail_tool_detail_m_c;
        setStr(tr.ToolResult_detail.ToolResult_detail_tool_detail_m.tool_detail_kind, dkind);
        setStr(tr.ToolResult_detail.ToolResult_detail_tool_detail_m.tool_detail_body, dbody);

        const mirror::TranscriptBlock out = mirror::map_transcript_block(rec);
        QCOMPARE(out.kind, QStringLiteral("ToolResult"));
        QCOMPARE(out.call_id, QStringLiteral("c-9"));
        QVERIFY(out.ok);
        QCOMPARE(out.summary, QStringLiteral("patched"));
        // The G2 enrichment: the structured tool-detail rides the entity, body decoded as UTF-8
        // (the exact transform the fence projection folds in).
        QCOMPARE(out.detail_kind, QStringLiteral("fs.diff"));
        QCOMPARE(out.detail_body, QStringLiteral("{\"path\":\"a\"}"));
    }

    void mapsNonBlockPayloadToKindlessRow() {
        static const QByteArray detail = QByteArrayLiteral("gc");
        ::journal_record rec = blockRecord(7);
        rec.journal_record_payload.journal_record_payload_t_choice = ::journal_record_payload_t_r::
            journal_record_payload_t_journal_record_payload_management_m_c;
        setStr(rec.journal_record_payload
                   .journal_record_payload_t_journal_record_payload_management_m.Management_detail,
               detail);
        const mirror::TranscriptBlock out = mirror::map_transcript_block(rec);
        QVERIFY(out.kind.isEmpty()); // a Management/Chat record is not transcript content
        QCOMPARE(out.seq, quint64{7});
    }
};

QTEST_GUILESS_MAIN(TstMirrorTranscriptParity)
#include "tst_mirror_transcript_parity.moc"
