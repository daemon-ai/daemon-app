// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/cached_session_store.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "daemon/subscription_manager.h"
#include "daemon_turn_engine.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::ApprovalRepository;
using daemonapp::daemon::CachedSessionStore;
using daemonapp::daemon::CachedTranscriptBlockRow;
using daemonapp::daemon::CachedTransportInstanceRow; // [wave2:app-channels-liveness]
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::FleetRepository; // [wave2:app-channels-liveness]
using daemonapp::daemon::ModelRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::SessionRepository;
using daemonapp::daemon::SubscriptionManager;
using daemonapp::daemon::TransportRepository; // [wave2:app-channels-liveness]
using daemonapp::test::WireMuxServer;

namespace {

// A minimal session-info: {"session": id, "state": "Active"} (all other fields optional).
QByteArray sInfo(const char* id) {
    QByteArray b;
    b.append(static_cast<char>(0xA2));
    daemonapp::test::cborText(b, "session");
    daemonapp::test::cborText(b, id);
    daemonapp::test::cborText(b, "state");
    daemonapp::test::cborText(b, "Active");
    return b;
}

// {"SessionPage":{"sessions":[..], "rev":R, ["removed":[..]]}} (fields in CDDL order; <24 counts).
QByteArray sessionPage(const QList<QByteArray>& infos, quint64 rev,
                       const QList<const char*>& removed = {}) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "SessionPage");
    b.append(static_cast<char>(0xA0 | (removed.isEmpty() ? 2 : 3)));
    daemonapp::test::cborText(b, "sessions");
    b.append(static_cast<char>(0x80 | static_cast<char>(infos.size())));
    for (const QByteArray& i : infos) {
        b.append(i);
    }
    daemonapp::test::cborText(b, "rev");
    daemonapp::test::cborUint(b, rev);
    if (!removed.isEmpty()) {
        daemonapp::test::cborText(b, "removed");
        b.append(static_cast<char>(0x80 | static_cast<char>(removed.size())));
        for (const char* r : removed) {
            daemonapp::test::cborText(b, r);
        }
    }
    return b;
}

} // namespace

// A stand-in focused turn engine: the SubscriptionManager nudges it by name on SessionAdvanced.
class FakeEngine : public QObject {
    Q_OBJECT
public:
    int nudges = 0;
public slots:
    void nudge() { ++nudges; }
};

// Phase 3 (L3): the SubscriptionManager consumes the single node-wide EventsSince feed and routes
// each payload-free NodeEvent (roster/meta -> roster refetch, approval -> inbox refresh,
// session-advanced -> focused-engine nudge, resync -> baseline) without polling; plus the durable
// render-from-cache transcript.
class TestSubscriptionManager : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString sock(const QString& name) { return m_tmp.filePath(name); }
    QString dbPath(const QString& name) { return m_tmp.filePath(name); }

private slots:
    // start() opens exactly one node-wide feed stream.
    void startOpensOneFeed() {
        const QString path = sock(QStringLiteral("feed.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("c1.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.openCount() == 1 && fake.lastOpenId() != 0, 3000);
    }

    // An ApprovalPending notification triggers an ApprovalsPending refetch (a one-shot Call).
    void approvalPendingRefreshesInbox() {
        const QString path = sock(QStringLiteral("appr.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("c2.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neApprovalPending("s1", "req-1")}, 1, 1));
        // The routed refreshPending() issues an ApprovalsPending one-shot Call.
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > callsBefore, 3000);
    }

    // RosterChanged triggers a (debounced) roster refetch.
    void rosterChangedRefetchesRoster() {
        const QString path = sock(QStringLiteral("roster.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("c3.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(
            daemonapp::test::buildEventsPage({daemonapp::test::neRosterChanged(7)}, 1, 1));
        // Debounced (~300ms) SessionsQuery one-shot Call.
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > callsBefore, 4000);
    }

    // SessionAdvanced for a registered session nudges its focused engine; an unregistered one does
    // not crash and simply has no target.
    void sessionAdvancedNudgesFocusedEngine() {
        const QString path = sock(QStringLiteral("adv.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("c4.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        FakeEngine engine;
        mgr.registerFocus(QStringLiteral("s-focused"), &engine);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);

        // An advance for an UNregistered session: no nudge.
        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neSessionAdvanced("s-other", 0, 5)}, 1, 1));
        // An advance for the focused session: one nudge.
        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neSessionAdvanced("s-focused", 0, 9)}, 2, 2));
        QTRY_COMPARE_WITH_TIMEOUT(engine.nudges, 1, 3000);
    }

    // A CatalogChanged notification (wire v26) triggers a ModelCatalog refetch — the push path
    // that makes a finished download appear without any client polling.
    void catalogChangedRefetchesModelCatalog() {
        const QString path = sock(QStringLiteral("catalog.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("cc.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(
            daemonapp::test::buildEventsPage({daemonapp::test::neCatalogChanged()}, 1, 1));
        // The routed refreshCatalog() issues a ModelCatalog one-shot Call.
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > callsBefore, 3000);
    }

    // A DownloadProgress notification (wire v26) patches the job row with the REAL byte counters
    // carried on the event (no derived math).
    void downloadProgressAppliesByteCounters() {
        const QString path = sock(QStringLiteral("dlprog.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("dp.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        QSignalSpy downloads(&models, &ModelRepository::downloadsChanged);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);

        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neDownloadProgress(9, 50, "Downloading", 46'000'000, 92'000'000)}, 1,
            1));
        QTRY_COMPARE_WITH_TIMEOUT(downloads.count(), 1, 3000);
        QCOMPARE(models.downloads().size(), 1);
        const auto& s = models.downloads().first();
        QCOMPARE(s.id, quint64(9));
        QCOMPARE(s.state, QStringLiteral("Downloading"));
        QCOMPARE(s.downloadedBytes, quint64(46'000'000));
        QCOMPARE(s.totalBytes, quint64(92'000'000));
    }

    // [wave2:app-channels-liveness] F5: a FleetChanged notification re-queries the Tree directly
    // (a fleet change need not move the roster). Previously fell through to Unknown (ignored).
    void fleetChangedRefetchesTree() {
        const QString path = sock(QStringLiteral("fleet.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("fc.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        FleetRepository fleet(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache, &fleet);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(daemonapp::test::buildEventsPage({daemonapp::test::neFleetChanged(3)}, 1, 1));
        // The routed refreshTree() issues a Tree one-shot Call.
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > callsBefore, 3000);
    }

    // [wave2:app-channels-liveness] B5: a TransportChanged notification patches the cached account
    // row's connection/presence IN PLACE (no round-trip) and emits instancesRefreshed; a connect
    // transition also refreshes that account's ConvList so joined rooms surface (B2).
    void transportChangedPatchesPresenceInPlace() {
        const QString path = sock(QStringLiteral("tx.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("tx.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        TransportRepository transports(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        mgr.setTransportRepository(&transports);

        // Seed a cached account (offline) so the event patches an existing row (not the
        // unknown-transport refetch fallback).
        CachedTransportInstanceRow row;
        row.transport = QStringLiteral("matrix/@bot:hs");
        row.family = QStringLiteral("matrix");
        row.displayName = QStringLiteral("@bot:hs");
        row.connection = QStringLiteral("offline");
        row.presence = QStringLiteral("unknown");
        row.boundProfile = QStringLiteral("p1");
        QVERIFY(cache.upsertTransportInstance(row));
        QSignalSpy refreshed(&transports, &TransportRepository::instancesRefreshed);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neTransportChanged("matrix/@bot:hs", "Connected", "Available")}, 1,
            1));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        // The row was patched in place (connection/presence mapped to lowercase), preserving the
        // fields the event does not carry.
        bool found = false;
        for (const CachedTransportInstanceRow& r : cache.transportInstances()) {
            if (r.transport == QStringLiteral("matrix/@bot:hs")) {
                QCOMPARE(r.connection, QStringLiteral("connected"));
                QCOMPARE(r.presence, QStringLiteral("available"));
                QCOMPARE(r.family, QStringLiteral("matrix"));
                QCOMPARE(r.boundProfile, QStringLiteral("p1"));
                found = true;
            }
        }
        QVERIFY(found);
        // A connect transition also fires a ConvList refresh (a one-shot Call).
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > callsBefore, 3000);
    }

    // ResyncNeeded re-baselines (emits the resyncNeeded signal after kicking the refetch).
    void resyncNeededReBaselines() {
        const QString path = sock(QStringLiteral("resync.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("c5.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        QSignalSpy resync(&mgr, &SubscriptionManager::resyncNeeded);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);

        fake.pushItem(
            daemonapp::test::buildEventsPage({daemonapp::test::neResyncNeeded("all")}, 5, 5));
        QTRY_COMPARE_WITH_TIMEOUT(resync.count(), 1, 3000);
    }

    // The durable render-from-cache transcript: persisted blocks project into the CANONICAL
    // msg-fence transcript markdown (the ```msg boundary markers the live ingest produces, so a
    // reload renders the same chat bubbles — never a "**Role:**" digest), and re-reading after a
    // "refocus" (a fresh store over the same db) still renders them.
    void transcriptBlocksRenderFromCache() {
        const QString db = dbPath(QStringLiteral("transcript.db"));
        {
            DaemonCacheStore cache(db);
            QVERIFY(cache.isOpen());
            CachedTranscriptBlockRow user;
            user.sessionId = QStringLiteral("s1");
            user.seq = 1;
            user.kind = QStringLiteral("Message");
            user.role = QStringLiteral("User");
            user.text = QStringLiteral("hello there");
            QVERIFY(cache.upsertTranscriptBlock(user));
            CachedTranscriptBlockRow asst;
            asst.sessionId = QStringLiteral("s1");
            asst.seq = 2;
            asst.kind = QStringLiteral("Message");
            asst.role = QStringLiteral("Assistant");
            asst.text = QStringLiteral("general kenobi");
            QVERIFY(cache.upsertTranscriptBlock(asst));

            CachedSessionStore store(&cache, nullptr);
            const QString md = store.content(domain::SessionId(QStringLiteral("s1")));
            QVERIFY2(md.contains(QStringLiteral("hello there")), qPrintable(md));
            QVERIFY2(md.contains(QStringLiteral("general kenobi")), qPrintable(md));
            // Canonical msg-fence boundaries with the persisted roles, not a lossy digest.
            QVERIFY2(
                md.contains(QStringLiteral("```msg\n{\"id\":\"cache-1\",\"role\":\"user\"}\n```")),
                qPrintable(md));
            QVERIFY2(md.contains(QStringLiteral(
                         "```msg\n{\"id\":\"cache-2\",\"role\":\"assistant\"}\n```")),
                     qPrintable(md));
            QVERIFY2(!md.contains(QStringLiteral("**Assistant:**")), qPrintable(md));
        }
        // Refocus / cold start: a new store over the same persisted db renders from disk.
        {
            DaemonCacheStore cache(db);
            CachedSessionStore store(&cache, nullptr);
            const QString md = store.content(domain::SessionId(QStringLiteral("s1")));
            QVERIFY2(md.contains(QStringLiteral("general kenobi")), qPrintable(md));
        }
    }

    // The `todo` tool's blocks are the status stack's feed, not transcript content: the cache
    // projection skips its ToolCall AND the matching ToolResult (mirrors the live suppression),
    // while other tools render as canonical ```tool fences (result folded in as status; a
    // resultless call stays "running") inside the assistant bubble.
    void todoToolBlocksAreSkippedInProjection() {
        DaemonCacheStore cache(dbPath(QStringLiteral("todo-skip.db")));
        QVERIFY(cache.isOpen());
        CachedTranscriptBlockRow todoCall;
        todoCall.sessionId = QStringLiteral("s1");
        todoCall.seq = 1;
        todoCall.kind = QStringLiteral("ToolCall");
        todoCall.callId = QStringLiteral("c1");
        todoCall.toolName = QStringLiteral("todo");
        todoCall.argsSummary = QStringLiteral("3 items");
        QVERIFY(cache.upsertTranscriptBlock(todoCall));
        CachedTranscriptBlockRow todoResult;
        todoResult.sessionId = QStringLiteral("s1");
        todoResult.seq = 2;
        todoResult.kind = QStringLiteral("ToolResult");
        todoResult.callId = QStringLiteral("c1");
        todoResult.ok = true;
        todoResult.summary = QStringLiteral("todo: 1/3 complete");
        QVERIFY(cache.upsertTranscriptBlock(todoResult));
        CachedTranscriptBlockRow shellCall;
        shellCall.sessionId = QStringLiteral("s1");
        shellCall.seq = 3;
        shellCall.kind = QStringLiteral("ToolCall");
        shellCall.callId = QStringLiteral("c2");
        shellCall.toolName = QStringLiteral("terminal");
        shellCall.argsSummary = QStringLiteral("ls");
        QVERIFY(cache.upsertTranscriptBlock(shellCall));
        CachedTranscriptBlockRow shellResult;
        shellResult.sessionId = QStringLiteral("s1");
        shellResult.seq = 4;
        shellResult.kind = QStringLiteral("ToolResult");
        shellResult.callId = QStringLiteral("c2");
        shellResult.ok = true;
        QVERIFY(cache.upsertTranscriptBlock(shellResult));
        CachedTranscriptBlockRow openCall;
        openCall.sessionId = QStringLiteral("s1");
        openCall.seq = 5;
        openCall.kind = QStringLiteral("ToolCall");
        openCall.callId = QStringLiteral("c3");
        openCall.toolName = QStringLiteral("compile");
        openCall.argsSummary = QStringLiteral("ninja");
        QVERIFY(cache.upsertTranscriptBlock(openCall));

        CachedSessionStore store(&cache, nullptr);
        const QString md = store.content(domain::SessionId(QStringLiteral("s1")));
        QVERIFY2(!md.contains(QStringLiteral("todo")), qPrintable(md));
        // The finished tool: one ```tool fence with the result folded in as status=ok.
        QVERIFY2(md.contains(QStringLiteral(
                     "```tool\n{\"argsSummary\":\"ls\",\"callId\":\"c2\",\"name\":\"terminal\","
                     "\"status\":\"ok\"}\n```")),
                 qPrintable(md));
        // The resultless tool projects as still running.
        QVERIFY2(md.contains(QStringLiteral("\"name\":\"compile\",\"status\":\"running\"")),
                 qPrintable(md));
    }

    // D1: a persisted rich tool result (full content in `summary` + typed detail) folds RAW into
    // its call's ```tool fence, so the reload's shared buildToolView projection re-renders the
    // same diff/output/hits card the live turn showed.
    void toolDetailSurvivesCacheProjection() {
        DaemonCacheStore cache(dbPath(QStringLiteral("tool-detail.db")));
        QVERIFY(cache.isOpen());
        CachedTranscriptBlockRow call;
        call.sessionId = QStringLiteral("s1");
        call.seq = 1;
        call.kind = QStringLiteral("ToolCall");
        call.callId = QStringLiteral("c1");
        call.toolName = QStringLiteral("fs");
        call.argsSummary = QStringLiteral("main.cpp");
        QVERIFY(cache.upsertTranscriptBlock(call));
        CachedTranscriptBlockRow result;
        result.sessionId = QStringLiteral("s1");
        result.seq = 2;
        result.kind = QStringLiteral("ToolResult");
        result.callId = QStringLiteral("c1");
        result.ok = true;
        result.summary = QStringLiteral("--- a/main.cpp\n+++ b/main.cpp\n@@ -1 +1 @@\n-a\n+b\n");
        result.detailKind = QStringLiteral("fs");
        result.detailBody = QByteArrayLiteral(R"({"op":"edit","path":"main.cpp","count":1})");
        QVERIFY(cache.upsertTranscriptBlock(result));

        CachedSessionStore store(&cache, nullptr);
        const QString md = store.content(domain::SessionId(QStringLiteral("s1")));
        // The fence carries the raw fields (JSON-escaped); status still folds from the result.
        QVERIFY2(md.contains(QStringLiteral("\"detailKind\":\"fs\"")), qPrintable(md));
        QVERIFY2(md.contains(QStringLiteral(
                     "\"detailBody\":\"{\\\"op\\\":\\\"edit\\\",\\\"path\\\":\\\"main.cpp\\\",")),
                 qPrintable(md));
        QVERIFY2(md.contains(QStringLiteral("\"summary\":\"--- a/main.cpp")), qPrintable(md));
        QVERIFY2(md.contains(QStringLiteral("\"status\":\"ok\"")), qPrintable(md));
    }

    // Reasoning disclosures persist as cache blocks and project as canonical ```reasoning
    // fences inside the assistant bubble — nothing in the transcript drops on a reload. The
    // final assistant message continues the same bubble (one msg marker), matching the live
    // single-assistant-turn grouping.
    void reasoningBlocksSurviveReload() {
        DaemonCacheStore cache(dbPath(QStringLiteral("reasoning.db")));
        QVERIFY(cache.isOpen());
        CachedTranscriptBlockRow user;
        user.sessionId = QStringLiteral("s1");
        user.seq = 1;
        user.kind = QStringLiteral("Message");
        user.role = QStringLiteral("User");
        user.text = QStringLiteral("why?");
        QVERIFY(cache.upsertTranscriptBlock(user));
        CachedTranscriptBlockRow reasoning;
        reasoning.sessionId = QStringLiteral("s1");
        reasoning.seq = 2;
        reasoning.kind = QStringLiteral("Reasoning");
        reasoning.text = QStringLiteral("Think first, then answer.");
        QVERIFY(cache.upsertTranscriptBlock(reasoning));
        CachedTranscriptBlockRow asst;
        asst.sessionId = QStringLiteral("s1");
        asst.seq = 3;
        asst.kind = QStringLiteral("Message");
        asst.role = QStringLiteral("Assistant");
        asst.text = QStringLiteral("because"); // continues the reasoning-opened bubble
        QVERIFY(cache.upsertTranscriptBlock(asst));

        CachedSessionStore store(&cache, nullptr);
        const QString md = store.content(domain::SessionId(QStringLiteral("s1")));
        QVERIFY2(md.contains(QStringLiteral("```reasoning\n{\"body\":\"Think first, then answer.\","
                                            "\"status\":\"complete\"}\n```")),
                 qPrintable(md));
        QVERIFY2(md.contains(QStringLiteral("because")), qPrintable(md));
        // One assistant marker: the disclosure opened the bubble, the message continued it.
        QCOMPARE(md.count(QStringLiteral("\"role\":\"assistant\"")), 1);
        QCOMPARE(md.count(QStringLiteral("\"role\":\"user\"")), 1);
    }

    // L4: the first (cold) refresh is a full page that seeds the cache + persists the roster rev; a
    // subsequent refresh is a delta (since_rev) that merges changed rows + prunes removed; a delta
    // whose returned rev went backwards (daemon reset) falls back to a full replace.
    void rosterDeltaMergesPrunesAndReplaceFallback() {
        const QString path = sock(QStringLiteral("roster-delta.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("rd.db")));
        SessionRepository sessions(&client, &cache);
        QSignalSpy refreshed(&sessions, &SessionRepository::sessionsRefreshed);

        // 1. Cold full page (rev 5, sessions A + B) -> cache seeded, rev persisted.
        fake.setReplyPayload(sessionPage({sInfo("A"), sInfo("B")}, 5));
        sessions.refreshSessions();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        auto ids = [&] {
            QStringList v;
            for (const auto& r : cache.sessions()) {
                v << r.sessionId;
            }
            v.sort();
            return v;
        };
        QCOMPARE(ids(), (QStringList{"A", "B"}));
        QCOMPARE(cache.cursor(QStringLiteral("roster-rev")), QStringLiteral("5"));

        // 2. Delta (rev 7: A changed, B removed) -> merge A, prune B.
        fake.setReplyPayload(sessionPage({sInfo("A")}, 7, {"B"}));
        sessions.refreshSessions();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 2, 3000);
        QCOMPARE(ids(), (QStringList{"A"}));
        QCOMPARE(cache.cursor(QStringLiteral("roster-rev")), QStringLiteral("7"));

        // 3. Fallback: we send since_rev 7 but the server returns a *lower* rev 2 (it restarted) ->
        // the page is a full replace (prune A, seed C).
        fake.setReplyPayload(sessionPage({sInfo("C")}, 2));
        sessions.refreshSessions();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 3, 3000);
        QCOMPARE(ids(), (QStringList{"C"}));
        QCOMPARE(cache.cursor(QStringLiteral("roster-rev")), QStringLiteral("2"));
    }

    // P3 focus wiring (now live): a DaemonTurnEngine self-registers with the SubscriptionManager on
    // setSessionId, so a SessionAdvanced for its bound session nudges it -> it opens a catch-up
    // subscribe stream (a second Open beyond the feed).
    void sessionAdvancedNudgesSelfRegisteredEngine() {
        const QString path = sock(QStringLiteral("focus.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("focus.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);

        // The engine self-registers its bound session with the manager.
        DaemonTurnEngine engine(&client, &cache, &mgr);
        engine.setSessionId(QStringLiteral("s-focus"));

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.openCount() == 1 && fake.lastOpenId() != 0, 3000);

        // SessionAdvanced for the bound session -> nudge -> the engine opens a catch-up subscribe.
        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neSessionAdvanced("s-focus", 0, 3)}, 1, 1));
        QTRY_VERIFY_WITH_TIMEOUT(fake.openCount() >= 2, 3000);
    }

    // clearTranscript wipes a session's blocks (the re-baseline path).
    void clearTranscriptWipesSession() {
        DaemonCacheStore cache(dbPath(QStringLiteral("clear.db")));
        CachedTranscriptBlockRow row;
        row.sessionId = QStringLiteral("s1");
        row.seq = 1;
        row.kind = QStringLiteral("Message");
        row.role = QStringLiteral("Assistant");
        row.text = QStringLiteral("stale");
        QVERIFY(cache.upsertTranscriptBlock(row));
        QCOMPARE(cache.transcriptBlocks(QStringLiteral("s1")).size(), 1);
        QVERIFY(cache.clearTranscript(QStringLiteral("s1")));
        QCOMPARE(cache.transcriptBlocks(QStringLiteral("s1")).size(), 0);
    }
};

QTEST_GUILESS_MAIN(TestSubscriptionManager)
#include "tst_subscription_manager.moc"
