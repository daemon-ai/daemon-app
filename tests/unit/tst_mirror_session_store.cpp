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
#include "mirror/mirror_service.h"
#include "mirror/parity.h"
#include "mirror/store.h"
#include "session_controller.h"

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

// A recording legacy stub: mutation/transcript delegation target + relay source.
class StubLegacyStore : public persistence::ISessionStore {
    Q_OBJECT

public:
    using persistence::ISessionStore::ISessionStore;

    QList<domain::UnitNode> unitChildren(const domain::UnitId&) const override { return {}; }
    domain::UnitNode unit(const domain::UnitId&) const override { return {}; }
    QList<domain::Tag> tags() const override { return {}; }
    QList<domain::Participant> participants() const override { return {}; }
    QList<domain::Session> sessions(const domain::ListScope&) const override { return {}; }
    int sessionCount(const domain::ListScope&) const override { return 0; }
    QString content(const domain::SessionId& id) const override {
        contentReads << id.toString();
        return QStringLiteral("legacy transcript");
    }
    QString title(const domain::SessionId&) const override { return {}; }
    bool isPinned(const domain::SessionId&) const override { return false; }
    domain::SessionId newSession(const domain::UnitId&) override { return {}; }
    void setContent(const domain::SessionId&, const QString&) override {}
    void setArchived(const domain::SessionId& id, bool a) override {
        calls << QStringLiteral("archive:%1:%2").arg(id.toString()).arg(a ? 1 : 0);
    }
    void renameSession(const domain::SessionId& id, const QString& t) override {
        calls << QStringLiteral("rename:%1:%2").arg(id.toString(), t);
    }
    void deleteSession(const domain::SessionId& id) override {
        calls << QStringLiteral("delete:%1").arg(id.toString());
    }
    void setPinned(const domain::SessionId& id, bool p) override {
        calls << QStringLiteral("pin:%1:%2").arg(id.toString()).arg(p ? 1 : 0);
    }
    void moveSession(const domain::SessionId&, int) override {}
    void requestNewSession(const QString& profileId) override {
        calls << QStringLiteral("create:%1").arg(profileId);
    }
    void refreshSessionsForProfile(const QString& p) override {
        calls << QStringLiteral("refreshProfile:%1").arg(p);
    }
    void refreshArchivedSessions() override { calls << QStringLiteral("refreshArchived"); }
    void refreshSessionsForTransport(const QString& t) override {
        calls << QStringLiteral("refreshTransport:%1").arg(t);
    }
    domain::UnitId createUnit(const domain::UnitId&, domain::UnitKind) override { return {}; }
    int createTag(const QString&, const QString&) override { return -1; }

    QStringList calls;
    mutable QStringList contentReads;
};

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
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), &legacy);

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

    // Mutations + transcript reads delegate to the legacy store; the wire round-trip signals
    // are relayed; the scoped refreshes hit BOTH sides (legacy cache + mirror fetch).
    void mutationsAndContentDelegateAndRelaySignals() {
        mirror::MirrorService svc;
        svc.openInMemory();
        StubLegacyStore legacy;
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), &legacy);

        store.setPinned(domain::SessionId(QStringLiteral("s-a")), true);
        store.renameSession(domain::SessionId(QStringLiteral("s-a")), QStringLiteral("T"));
        store.requestNewSession(QStringLiteral("prof-1"));
        store.refreshArchivedSessions();
        QCOMPARE(legacy.calls,
                 (QStringList{QStringLiteral("pin:s-a:1"), QStringLiteral("rename:s-a:T"),
                              QStringLiteral("create:prof-1"), QStringLiteral("refreshArchived")}));
        QCOMPARE(store.content(domain::SessionId(QStringLiteral("s-a"))),
                 QStringLiteral("legacy transcript"));
        QCOMPARE(legacy.contentReads, QStringList{QStringLiteral("s-a")});

        QSignalSpy created(&store, &persistence::ISessionStore::sessionCreated);
        QSignalSpy failed(&store, &persistence::ISessionStore::metaUpdateFailed);
        emit legacy.sessionCreated(QStringLiteral("s-new"), QStringLiteral("prof-1"));
        emit legacy.metaUpdateFailed(QStringLiteral("s-a"), QStringLiteral("denied"));
        QCOMPARE(created.count(), 1);
        QCOMPARE(failed.count(), 1);
    }

    // M4 sub-gate 4: the detail pane consumer — a SessionController (the VM TranscriptPage.qml
    // + the TUI TabSessionManager bind) over the mirror store renders the DELEGATED transcript
    // content (legacy source until sub-gate 6) and refreshes on the relayed legacy changed().
    void detailPaneControllerReadsThroughMirrorStore() {
        mirror::MirrorService svc;
        svc.openInMemory();
        StubLegacyStore legacy;
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), &legacy);

        SessionController controller;
        controller.setStore(&store);
        controller.open(QStringLiteral("s-a"));
        QCOMPARE(controller.content(), QStringLiteral("legacy transcript"));
        QVERIFY(legacy.contentReads.contains(QStringLiteral("s-a")));

        // A legacy-side transcript update (the engine writes the cache, cache emits changed):
        // the relayed changed() re-reads content through the same delegated path.
        const int before = static_cast<int>(legacy.contentReads.size());
        emit legacy.changed();
        QVERIFY(static_cast<int>(legacy.contentReads.size()) > before);
    }
};

QTEST_GUILESS_MAIN(TestMirrorSessionStore)
#include "tst_mirror_session_store.moc"
