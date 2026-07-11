// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The M4 mirror-backed ISessionStore (spec 09 §13 wave M4; LEDGER-a7) — the ONE projection the
// session consumers rebind onto (6→1). Asserts:
//  - DUAL-DECODER PARITY: the same SessionPage wire bytes decoded through the legacy path
//    (NodeApiCodec::decodeSessionPage → DaemonCacheStore → CachedSessionStore) and the mirror
//    path (decodeSessionsToMirror → ingestor → MirrorSessionStore) yield the same row-set
//    (parity::sessionKeys, §13 "parity asserts vs legacy roster until deletion") and the same
//    per-view answers (ids, titles, pinned).
//  - the scoped views project mirror rows (AllSessions/Archived/Agent/ByTransport);
//  - changed() re-derives only on Session/FleetUnit deltas (journal-filtered watermark);
//  - mutations + transcript reads delegate to the legacy store; its wire round-trip signals
//    (sessionCreated / metaUpdateFailed) are relayed.

#include "daemon/cached_session_store.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/mirror_decode.h"
#include "daemon/mirror_session_store.h"
#include "daemon/node_api_codec.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "local_database.h"
#include "mirror/mirror_service.h"
#include "mirror/parity.h"
#include "mirror/store.h"
#include "outbox.h"
#include "session_controller.h"
#include "transcript_exporter.h"
#include "verb_class.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::CachedSessionStore;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::decodeSessionsToMirror;
using daemonapp::daemon::MirrorSessionStore;
using daemonapp::daemon::NodeApiCodec;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

QByteArray encodeResponse(const api_response_r& resp) {
    QByteArray out(64 * 1024, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                 static_cast<size_t>(out.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// {"SessionPage": {sessions: [s-pinned (pinned, prof-1, title), s-plain (Ready)], rev: 4}}.
QByteArray sessionPageResponse() {
    static const QByteArray s1 = QByteArrayLiteral("s-pinned");
    static const QByteArray t1 = QByteArrayLiteral("Pinned thread");
    static const QByteArray p1 = QByteArrayLiteral("prof-1");
    static const QByteArray s2 = QByteArrayLiteral("s-plain");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_session_page_m_c;
    session_page& page =
        resp.api_response_response_session_page_m.response_session_page_SessionPage;
    page.session_page_sessions_session_info_m_count = 2;
    page.session_page_next_cursor_present = false;
    page.session_page_rev = 4;
    page.session_page_removed_present = false;
    page.session_page_origin_ops_present = false;

    session_info& a = page.session_page_sessions_session_info_m[0];
    setStr(a.session_info_session, s1);
    a.session_info_state.session_state_choice = session_state_r::session_state_Active_tstr_c;
    a.session_info_title_present = true;
    a.session_info_title.session_info_title_choice =
        session_info_title_r::session_info_title_tstr_c;
    setStr(a.session_info_title.session_info_title_tstr, t1);
    a.session_info_bound_profile_present = true;
    a.session_info_bound_profile.session_info_bound_profile_choice =
        session_info_bound_profile_r::session_info_bound_profile_profile_ref_m_c;
    setStr(a.session_info_bound_profile.session_info_bound_profile_profile_ref_m, p1);
    a.session_info_pinned_present = true;
    a.session_info_pinned.session_info_pinned = true;

    session_info& b = page.session_page_sessions_session_info_m[1];
    setStr(b.session_info_session, s2);
    b.session_info_state.session_state_choice = session_state_r::session_state_Ready_tstr_c;
    b.session_info_title_present = false;

    return encodeResponse(resp);
}

mirror::Session sess(const QString& id, const QString& title = QString()) {
    mirror::Session s;
    s.session = id;
    s.title = title;
    return s;
}

QSet<QString> idsOf(const QList<domain::Session>& rows) {
    QSet<QString> out;
    for (const domain::Session& s : rows) {
        out.insert(s.sessionId.toString());
    }
    return out;
}

} // namespace

class TestMirrorSessionStore : public QObject {
    Q_OBJECT

private slots:
    // §13 M4 parity: the SAME SessionPage bytes through both decode paths yield the same
    // row-set and the same per-view answers. This is the dual-write parity assert the sub-gates
    // hold until the legacy roster dies.
    void dualDecoderParityOverSameWireBytes() {
        const QByteArray bytes = sessionPageResponse();

        // Legacy path: decodeSessionPage → DaemonCacheStore → CachedSessionStore.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        QVERIFY(cache.isOpen());
        QList<daemonapp::daemon::CachedSessionRow> rows;
        QVERIFY(NodeApiCodec::decodeSessionPage(bytes, &rows, nullptr, nullptr, nullptr));
        for (const auto& row : rows) {
            QVERIFY(cache.upsertSession(row));
        }
        CachedSessionStore legacy(&cache, nullptr);

        // Mirror path: decodeSessionsToMirror → ingestor → MirrorSessionStore.
        mirror::MirrorService svc;
        svc.openInMemory();
        std::vector<mirror::Session> mirrorRows;
        quint64 rev = 0;
        QVERIFY(decodeSessionsToMirror(bytes, &mirrorRows, nullptr, &rev, nullptr));
        svc.ingestor().deliverSessions(mirrorRows, /*isFinalPage=*/true, rev);
        MirrorSessionStore store(&svc.store(), &svc.ingestor());

        // Row-set parity (the §13 M4 exit assert).
        QSet<QString> legacyKeys;
        for (const auto& row : cache.sessions()) {
            legacyKeys.insert(row.sessionId);
        }
        const mirror::parity::Result r = mirror::parity::compareKeys(
            mirror::parity::sessionKeys(svc.store().snapshot()), legacyKeys);
        QVERIFY(mirror::parity::checkAndLog(QStringLiteral("sessions"), r));

        // View parity: same ids, count, titles, pinned flags through the ISessionStore reads.
        const domain::ListScope all{domain::NodeType::AllSessions, -1, {}, {}};
        QCOMPARE(idsOf(store.sessions(all)), idsOf(legacy.sessions(all)));
        QCOMPARE(store.sessionCount(all), legacy.sessionCount(all));
        for (const QString& id : legacyKeys) {
            QCOMPARE(store.title(domain::SessionId(id)), legacy.title(domain::SessionId(id)));
            QCOMPARE(store.isPinned(domain::SessionId(id)), legacy.isPinned(domain::SessionId(id)));
        }
        // The pinned row floats first in both projections.
        QCOMPARE(store.sessions(all).first().sessionId.toString(), QStringLiteral("s-pinned"));
        QCOMPARE(legacy.sessions(all).first().sessionId.toString(), QStringLiteral("s-pinned"));
    }

    // The scoped views project mirror rows: AllSessions excludes archived; Archived shows them;
    // Agent filters bound_profile; ByTransport projects the node-resolved id set.
    void scopedViewsProjectMirrorRows() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);

        mirror::Session a = sess(QStringLiteral("s-a"), QStringLiteral("A"));
        a.bound_profile = QStringLiteral("prof-1");
        mirror::Session b = sess(QStringLiteral("s-b"), QStringLiteral("B"));
        mirror::Session arch = sess(QStringLiteral("s-arch"), QStringLiteral("Old"));
        arch.archived = true;
        svc.ingestor().deliverSessions({a, b}, /*isFinalPage=*/true);
        svc.ingestor().deliverSessionsAdditive({arch}, /*isFinalPage=*/true);

        const domain::ListScope all{domain::NodeType::AllSessions, -1, {}, {}};
        const domain::ListScope archived{domain::NodeType::Archived, -1, {}, {}};
        const domain::ListScope agent{domain::NodeType::Agent, -1, {}, QStringLiteral("prof-1")};
        QCOMPARE(idsOf(store.sessions(all)),
                 (QSet<QString>{QStringLiteral("s-a"), QStringLiteral("s-b")}));
        QCOMPARE(idsOf(store.sessions(archived)), (QSet<QString>{QStringLiteral("s-arch")}));
        QCOMPARE(idsOf(store.sessions(agent)), (QSet<QString>{QStringLiteral("s-a")}));
        QCOMPARE(store.sessionCount(archived), 1);

        // ByTransport: projected from the ingestor's node-resolved id set, never re-derived.
        const domain::ListScope byTransport{
            domain::NodeType::ByTransport, -1, {}, QStringLiteral("matrix-1")};
        QCOMPARE(store.sessionCount(byTransport), 0); // no resolution recorded yet
        svc.ingestor().deliverTransportSessions(QStringLiteral("matrix-1"), {b},
                                                /*isFinalPage=*/true);
        QCOMPARE(idsOf(store.sessions(byTransport)), (QSet<QString>{QStringLiteral("s-b")}));
    }

    // changed() re-derives on Session deltas and stays quiet for unrelated kinds (the
    // journal-filtered watermark, §8.1 discipline).
    void changedFiresOnSessionDeltasOnly() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);
        QSignalSpy changed(&store, &persistence::ISessionStore::changed);

        mirror::Conversation conv;
        conv.transport = QStringLiteral("m");
        conv.id = QStringLiteral("!a");
        svc.ingestor().deliverConversations(QStringLiteral("m"), {conv}, /*isFinalPage=*/true);
        QCOMPARE(changed.count(), 0); // unrelated kind: no roster re-derivation

        svc.ingestor().deliverSessions({sess(QStringLiteral("s-a"), QStringLiteral("A"))},
                                       /*isFinalPage=*/true);
        QCOMPARE(changed.count(), 1);
        const domain::ListScope all{domain::NodeType::AllSessions, -1, {}, {}};
        QCOMPARE(store.sessionCount(all), 1);
    }

    // AD (§6.4/§7): pin/archive/rename route through the session-meta OUTBOX lane — durable
    // before any send, op_id minted (§6.2), lane scope = the session — never a delegation and
    // never an optimistic mirror write. content() projects the mirror transcript window (G2).
    void metaMutationsEnqueueToSessionMetaLane() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        mirror::LocalDatabase db(QStringLiteral(":memory:"));
        mirror::Outbox outbox(&db);
        outbox.load();
        store.setOutbox(&outbox);

        store.setPinned(domain::SessionId(QStringLiteral("s-a")), true);
        store.renameSession(domain::SessionId(QStringLiteral("s-a")), QStringLiteral("T"));
        store.setArchived(domain::SessionId(QStringLiteral("s-b")), true);

        // Three durable ops in the per-session session-meta lanes.
        const QList<mirror::PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 3);
        const QString laneA =
            mirror::buildLane(mirror::VerbClass::SessionMeta, QStringLiteral("s-a"));
        QCOMPARE(ops.at(0).lane, laneA);
        QCOMPARE(ops.at(0).verb, QStringLiteral("SessionUpdateMeta"));
        const QJsonObject pin = QJsonDocument::fromJson(ops.at(0).payload).object();
        QCOMPARE(pin.value(QStringLiteral("session")).toString(), QStringLiteral("s-a"));
        QVERIFY(pin.value(QStringLiteral("pinned")).toBool());
        QVERIFY(!pin.contains(QStringLiteral("archived"))); // tri-state: only the touched key
        QVERIFY(!pin.contains(QStringLiteral("title")));
        const QJsonObject rename = QJsonDocument::fromJson(ops.at(1).payload).object();
        QCOMPARE(rename.value(QStringLiteral("title")).toString(), QStringLiteral("T"));
        const QJsonObject archive = QJsonDocument::fromJson(ops.at(2).payload).object();
        QVERIFY(archive.value(QStringLiteral("archived")).toBool());

        // No mirror row was written by the mutations (§14.7: no optimistic/fake rows) — the row
        // appears only when the node's authoritative read lands.
        QCOMPARE(svc.store().snapshot().sessions.size(), std::size_t(0));

        // The flip: content() serves the mirror w_transcript_blocks projection.
        mirror::TranscriptBlock b;
        b.session = QStringLiteral("s-a");
        b.seq = 1;
        b.kind = QStringLiteral("Message");
        b.role = QStringLiteral("User");
        b.text = QStringLiteral("mirror words");
        svc.ingestor().deliverTranscriptBlock(b);
        const QString content = store.content(domain::SessionId(QStringLiteral("s-a")));
        QVERIFY(content.contains(QStringLiteral("mirror words")));
        QVERIFY(content.contains(QStringLiteral("```msg"))); // the msg-fence projection
    }

    // AD (§7 direct create): without a repo (mock), requestNewSession emits createRequested and
    // the composition's answer (onNodeSessionCreated) relays as sessionCreated — the async
    // node-authority adoption path. newSession() is empty in both modes (nothing client-minted);
    // the no-verb operations (delete/move/setContent/createUnit/createTag) are aligned no-ops.
    void requestNewSessionEmitsCreateSeamWithoutRepo() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        QSignalSpy requested(&store, &MirrorSessionStore::createRequested);
        QSignalSpy created(&store, &persistence::ISessionStore::sessionCreated);

        QVERIFY(store.newSession(domain::UnitId()).isEmpty());
        store.requestNewSession(QStringLiteral("prof-1"));
        QCOMPARE(requested.count(), 1);
        QCOMPARE(requested.first().at(0).toString(), QStringLiteral("prof-1"));
        QCOMPARE(created.count(), 0); // async: adoption waits for the composition's answer

        store.onNodeSessionCreated(QStringLiteral("s-mock-1"), QStringLiteral("prof-1"));
        QCOMPARE(created.count(), 1);
        QCOMPARE(created.first().at(0).toString(), QStringLiteral("s-mock-1"));

        // Aligned no-ops (daemon parity; the mock local mutations were a shape fork).
        store.deleteSession(domain::SessionId(QStringLiteral("s-a")));
        store.moveSession(domain::SessionId(QStringLiteral("s-a")), -1);
        store.setContent(domain::SessionId(QStringLiteral("s-a")), QStringLiteral("x"));
        QVERIFY(store.createUnit(domain::UnitId(), domain::UnitKind::Engine).isEmpty());
        QCOMPARE(store.createTag(QStringLiteral("t"), QStringLiteral("#fff")), -1);
        QCOMPARE(svc.store().snapshot().sessions.size(), std::size_t(0));
    }

    // §6.5: a session-meta lane rejection pauses the lane AND reaches the initiating surface
    // through the verb seam's failure signal — the SAME metaUpdateFailed the GUI toast + TUI
    // notification already bind (no new surface wiring needed for the failure signal).
    void sessionMetaRejectionRelaysMetaUpdateFailed() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);
        mirror::LocalDatabase db(QStringLiteral(":memory:"));
        mirror::Outbox outbox(&db);
        outbox.load();
        outbox.setProvenanceCapable(true);
        store.setOutbox(&outbox);
        QSignalSpy failed(&store, &persistence::ISessionStore::metaUpdateFailed);

        store.setPinned(domain::SessionId(QStringLiteral("s-a")), true);
        const QList<mirror::PendingOp> ops = outbox.ops();
        QCOMPARE(ops.size(), 1);
        // drain() was nudged (gate default-permissive here): the op went inflight; reject it.
        QCOMPARE(outbox.op(ops.first().opId).state, mirror::OpState::Inflight);
        outbox.onRejection(ops.first().opId, QStringLiteral("Forbidden"),
                           QStringLiteral("not the owner"));

        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.first().at(0).toString(), QStringLiteral("s-a")); // the lane scope tail
        QVERIFY(failed.first().at(1).toString().contains(QStringLiteral("Forbidden")));
        QVERIFY(outbox.lanePaused(
            mirror::buildLane(mirror::VerbClass::SessionMeta, QStringLiteral("s-a"))));
    }

    // §6.6/§10.3: the provenance landing — the node-acked op goes `accepted`, and the
    // SessionMetaChanged-threaded SessionGet apply (deliverSession carrying origin_op) lands it
    // (row deleted). The roster row reflects the node's authoritative state in the same commit.
    void sessionMetaOpLandsViaProvenanceStampedApply() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);
        mirror::LocalDatabase db(QStringLiteral(":memory:"));
        mirror::Outbox outbox(&db);
        outbox.load();
        outbox.setProvenanceCapable(true); // api/39: ack ⇒ accepted, awaiting the echo
        store.setOutbox(&outbox);
        // The graph's provenance wiring (§5.1 read-side coupling), reproduced.
        QObject::connect(
            &svc, &mirror::MirrorService::provenanceStamped, &outbox,
            [&outbox](const QString& originOp) { outbox.onProvenanceStamped(originOp); });

        svc.ingestor().deliverSessions({sess(QStringLiteral("s-a"), QStringLiteral("A"))},
                                       /*isFinalPage=*/true);
        store.setPinned(domain::SessionId(QStringLiteral("s-a")), true);
        const QString opId = outbox.ops().first().opId;
        outbox.onAck(opId);
        QCOMPARE(outbox.op(opId).state, mirror::OpState::Accepted);

        // The node's read-path echo: SessionGet hydration carrying origin_op == op_id.
        mirror::Session patched = sess(QStringLiteral("s-a"), QStringLiteral("A"));
        patched.pinned = true;
        svc.ingestor().deliverSession(patched, opId);

        QVERIFY(!outbox.contains(opId)); // landed — terminal success, row deleted (§6.6)
        QVERIFY(store.isPinned(domain::SessionId(QStringLiteral("s-a"))));

        // The delta-read carrier (§10.3 carrier 2) lands identically via the origin_ops map.
        store.renameSession(domain::SessionId(QStringLiteral("s-a")), QStringLiteral("B"));
        const QString op2 = outbox.ops().first().opId;
        outbox.onAck(op2);
        mirror::Session renamed = patched;
        renamed.title = QStringLiteral("B");
        svc.ingestor().deliverSessionsDelta({renamed}, {}, /*rev=*/7, /*isFinalPage=*/true,
                                            {{QStringLiteral("s-a"), op2}});
        QVERIFY(!outbox.contains(op2));
        QCOMPARE(store.title(domain::SessionId(QStringLiteral("s-a"))), QStringLiteral("B"));
    }

    // M4 sub-gate 4 / G2 flip: the detail pane consumer — a SessionController (the VM
    // TranscriptPage.qml + the TUI TabSessionManager bind) over the mirror store renders the
    // MIRROR transcript projection, and a TranscriptBlock journal delta fires changed() so the
    // controller refreshes on an engine transcript write.
    void detailPaneControllerReadsThroughMirrorStore() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor());

        SessionController controller;
        controller.setStore(&store);
        controller.open(QStringLiteral("s-a"));
        QVERIFY(controller.content().isEmpty()); // nothing mirrored yet

        // An engine transcript write lands in the mirror window → the TranscriptBlock delta arm
        // fires changed() → the controller re-reads the (mirror) projection.
        mirror::TranscriptBlock b;
        b.session = QStringLiteral("s-a");
        b.seq = 1;
        b.kind = QStringLiteral("Message");
        b.role = QStringLiteral("Assistant");
        b.text = QStringLiteral("streamed answer");
        svc.ingestor().deliverTranscriptBlock(b);
        QVERIFY(controller.content().contains(QStringLiteral("streamed answer")));
    }

    // M4 sub-gate 5 / G2 flip: the transcript chrome consumer — the shared TranscriptExporter
    // (GUI Exporter context property + the TUI exportToPath) composes the export from ONE store:
    // the mirror row's TITLE and the mirror-projected CONTENT.
    void transcriptExporterComposesMirrorTitleAndContent() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor());
        svc.ingestor().deliverSessions(
            {sess(QStringLiteral("s-a"), QStringLiteral("Mirror title"))},
            /*isFinalPage=*/true);
        mirror::TranscriptBlock b;
        b.session = QStringLiteral("s-a");
        b.seq = 1;
        b.kind = QStringLiteral("Message");
        b.role = QStringLiteral("User");
        b.text = QStringLiteral("mirror transcript body");
        svc.ingestor().deliverTranscriptBlock(b);

        const TranscriptExporter exporter;
        const QString json = exporter.toJson(&store, QStringLiteral("s-a"));
        QVERIFY(json.contains(QStringLiteral("Mirror title")));           // mirror row title
        QVERIFY(json.contains(QStringLiteral("mirror transcript body"))); // mirror content
    }
};

QTEST_GUILESS_MAIN(TestMirrorSessionStore)
#include "tst_mirror_session_store.moc"
