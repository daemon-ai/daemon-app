// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// A7T/G2 → AD — the transcript byte-stability suite (spec 09 §13; LEDGER-a7t/LEDGER-ad).
//
// Drives coalesced transcript block streams through the PRODUCTION path:
//   CachedTranscriptBlockRow -> MirrorTranscriptSink -> ingestor -> w_transcript_blocks
//   -> MirrorSessionStore::mirrorContent()
// and asserts:
//  - GOLDEN byte literals pin the msg-fence grammar (bubble markers, assistant grouping,
//    reasoning fences, tool fences incl. args + structured detail) — the byte-stability contract
//    the deleted legacy-oracle comparison proved (the oracle, CachedSessionStore::content(),
//    died with the legacy store; the grammar is now pinned directly, equal strength);
//  - pipeline integrity: the sink→window→projection output equals projecting the same blocks
//    directly (no window reordering/loss);
//  - the structural rules: todo suppression, rebaseline clear+replay, reasoning re-checkpoint
//    in-place, out-of-order sorted insert.
// The wire-decode mapper (map_transcript_block) coverage is unchanged.

#include "daemon/daemon_cache_store.h"
#include "daemon/mirror_session_store.h"
#include "daemon/mirror_transcript_sink.h"
#include "daemon/transcript_projection.h"
#include "mirror/generated/entities_map_gen.h"
#include "mirror/mirror_service.h"

#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::CachedTranscriptBlockRow;
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

// Feed the stream through the PRODUCTION sink and return the mirror projection + the direct
// projection of the same mapped blocks (the pipeline-integrity pair).
struct Projections {
    QString mirror; // sink -> window -> MirrorSessionStore::mirrorContent
    QString direct; // projectTranscriptBlocks over the sink's own mapping, no window
};

Projections project(const std::vector<CachedTranscriptBlockRow>& blocks) {
    mirror::MirrorService svc;
    svc.openInMemory();
    MirrorTranscriptSink sink(&svc.ingestor());
    for (const CachedTranscriptBlockRow& b : blocks) {
        sink.deliverBlock(b);
    }
    MirrorSessionStore store(&svc.store(), &svc.ingestor());

    // The direct leg re-reads the WINDOW's rows back out in seq order... no — it must be
    // window-independent: map each row the way the sink does and project straight.
    std::vector<mirror::TranscriptBlock> mapped;
    mapped.reserve(blocks.size());
    for (const CachedTranscriptBlockRow& b : blocks) {
        mirror::TranscriptBlock t;
        t.session = b.sessionId;
        t.seq = b.seq;
        t.kind = b.kind;
        t.role = b.role;
        t.text = b.text;
        t.call_id = b.callId;
        t.name = b.toolName;
        t.args_summary = b.argsSummary;
        t.summary = b.summary;
        t.ok = b.ok;
        t.detail_kind = b.detailKind;
        t.detail_body = QString::fromUtf8(b.detailBody);
        mapped.push_back(t);
    }
    return {store.mirrorContent(domain::SessionId(QLatin1String(kSession))),
            daemonapp::daemon::projectTranscriptBlocks(mapped)};
}

} // namespace

class TstMirrorTranscriptParity : public QObject {
    Q_OBJECT

private slots:
    // S1: a single user message bubble — the GOLDEN grammar literal.
    void s1_singleUserMessageGolden() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("hello"))});
        QCOMPARE(p.mirror, QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"user\"}\n```\n\n"
                                          "hello"));
        QCOMPARE(p.mirror, p.direct);
    }

    // S2: user + final assistant message (two bubbles).
    void s2_userThenAssistantGolden() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("hi")),
                     message(2, QStringLiteral("Assistant"), QStringLiteral("there"))});
        QCOMPARE(p.mirror,
                 QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"user\"}\n```\n\nhi\n\n"
                                "```msg\n{\"id\":\"cache-2\",\"role\":\"assistant\"}\n```\n\n"
                                "there"));
        QCOMPARE(p.mirror, p.direct);
    }

    // S3: reasoning (opens an assistant group) then the final assistant message CONTINUES it —
    // ONE assistant bubble marker, keys of the reasoning fence sorted (body before status).
    void s3_reasoningThenAssistantGroupGolden() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("q")),
                     reasoning(2, QStringLiteral("thinking about it")),
                     message(3, QStringLiteral("Assistant"), QStringLiteral("answer"))});
        QCOMPARE(p.mirror,
                 QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"user\"}\n```\n\nq\n\n"
                                "```msg\n{\"id\":\"cache-2\",\"role\":\"assistant\"}\n```\n\n"
                                "```reasoning\n{\"body\":\"thinking about it\","
                                "\"status\":\"complete\"}\n```\n\n"
                                "answer"));
        QCOMPARE(p.mirror, p.direct);
    }

    // S4: tool call with EMPTY args + a plain-text ok result folded into the fence (QJsonObject
    // key sort: argsSummary, callId, name, status, summary).
    void s4_toolCallEmptyArgsPlainResultGolden() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("run it")),
                     toolCall(2, QStringLiteral("c1"), QStringLiteral("build"), QString()),
                     toolResult(3, QStringLiteral("c1"), true, QStringLiteral("done"))});
        QCOMPARE(p.mirror,
                 QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"user\"}\n```\n\nrun it\n\n"
                                "```msg\n{\"id\":\"cache-2\",\"role\":\"assistant\"}\n```\n\n"
                                "```tool\n{\"argsSummary\":\"\",\"callId\":\"c1\","
                                "\"name\":\"build\",\"status\":\"ok\",\"summary\":\"done\"}"
                                "\n```"));
        QCOMPARE(p.mirror, p.direct);
    }

    // S5: a tool call still running (no result) -> status "running".
    void s5_toolCallRunning() {
        const Projections p =
            project({toolCall(1, QStringLiteral("c1"), QStringLiteral("search"), QString())});
        QVERIFY(p.mirror.contains(QStringLiteral("\"status\":\"running\"")));
        QCOMPARE(p.mirror, p.direct);
    }

    // S6: the `todo` tool call + result are suppressed (status-stack feed, not transcript).
    void s6_todoSuppressed() {
        const Projections p =
            project({message(1, QStringLiteral("User"), QStringLiteral("plan")),
                     toolCall(2, QStringLiteral("t1"), QStringLiteral("todo"), QString()),
                     toolResult(3, QStringLiteral("t1"), true, QStringLiteral("[]")),
                     message(4, QStringLiteral("Assistant"), QStringLiteral("planned"))});
        QCOMPARE(p.mirror, p.direct);
        QVERIFY(!p.mirror.contains(QStringLiteral("\"name\":\"todo\"")));
    }

    // S7 (the G2 entity-gap closure): a tool call with NON-EMPTY args reproduces the args key.
    void s7_toolCallNonEmptyArgsGolden() {
        const Projections p = project(
            {toolCall(1, QStringLiteral("c1"), QStringLiteral("shell"), QStringLiteral("ls -la")),
             toolResult(2, QStringLiteral("c1"), true, QStringLiteral("out"))});
        QCOMPARE(p.mirror,
                 QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"assistant\"}\n```\n\n"
                                "```tool\n{\"argsSummary\":\"ls -la\",\"callId\":\"c1\","
                                "\"name\":\"shell\",\"status\":\"ok\",\"summary\":\"out\"}"
                                "\n```"));
        QCOMPARE(p.mirror, p.direct);
    }

    // S8 (the G2 entity-gap closure): a structured detail folds detailKind + detailBody
    // (key sort: argsSummary, callId, detailBody, detailKind, name, status, summary).
    void s8_toolResultStructuredDetailGolden() {
        const Projections p =
            project({toolCall(1, QStringLiteral("c1"), QStringLiteral("edit"), QString()),
                     toolResult(2, QStringLiteral("c1"), true, QStringLiteral("patched"),
                                QStringLiteral("fs.diff"), QByteArrayLiteral("{\"path\":\"a\"}"))});
        QCOMPARE(p.mirror,
                 QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"assistant\"}\n```\n\n"
                                "```tool\n{\"argsSummary\":\"\",\"callId\":\"c1\","
                                "\"detailBody\":\"{\\\"path\\\":\\\"a\\\"}\","
                                "\"detailKind\":\"fs.diff\",\"name\":\"edit\","
                                "\"status\":\"ok\",\"summary\":\"patched\"}\n```"));
        QCOMPARE(p.mirror, p.direct);
    }

    // S9: a journal rebaseline (clear + replay) fully replaces the window — no stale-generation
    // blocks survive the clear.
    void s9_rebaselineClearReplay() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorTranscriptSink sink(&svc.ingestor());
        for (const CachedTranscriptBlockRow& b :
             {message(5, QStringLiteral("User"), QStringLiteral("stale")),
              message(6, QStringLiteral("Assistant"), QStringLiteral("old"))}) {
            sink.deliverBlock(b);
        }
        sink.clear(QLatin1String(kSession));
        for (const CachedTranscriptBlockRow& b :
             {message(1, QStringLiteral("User"), QStringLiteral("fresh")),
              message(2, QStringLiteral("Assistant"), QStringLiteral("new"))}) {
            sink.deliverBlock(b);
        }
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        const domain::SessionId id{QLatin1String(kSession)};
        QVERIFY(store.mirrorContent(id).contains(QStringLiteral("fresh")));
        QVERIFY(!store.mirrorContent(id).contains(QStringLiteral("stale")));
        QVERIFY(!store.mirrorContent(id).contains(QStringLiteral("old")));
    }

    // Reasoning coalescing: re-checkpointing the SAME seq updates the block in place (one fence,
    // the fuller text) rather than appending a second reasoning block.
    void reasoningReCheckpointUpsertsInPlace() {
        const Projections p =
            project({reasoning(2, QStringLiteral("abc")), reasoning(2, QStringLiteral("abcdef"))});
        QCOMPARE(p.mirror.count(QStringLiteral("```reasoning")), 1);
        QVERIFY(p.mirror.contains(QStringLiteral("abcdef")));
        QVERIFY(!p.mirror.contains(QStringLiteral("\"body\":\"abc\"")));
    }

    // upsertWindow is cursor-ordered: blocks delivered out of seq order land sorted (the window
    // projection equals projecting the IN-ORDER stream directly).
    void outOfOrderDeliverySortsBySeq() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorTranscriptSink sink(&svc.ingestor());
        const std::vector<CachedTranscriptBlockRow> inOrder{
            message(1, QStringLiteral("User"), QStringLiteral("one")),
            message(2, QStringLiteral("Assistant"), QStringLiteral("two")),
            message(3, QStringLiteral("User"), QStringLiteral("three"))};
        sink.deliverBlock(inOrder[2]);
        sink.deliverBlock(inOrder[0]);
        sink.deliverBlock(inOrder[1]);
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        const domain::SessionId id{QLatin1String(kSession)};
        QCOMPARE(store.mirrorContent(id), project(inOrder).direct);
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
