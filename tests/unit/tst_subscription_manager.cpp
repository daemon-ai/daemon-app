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
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::ModelRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::SessionRepository;
using daemonapp::daemon::SubscriptionManager;
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

    // The durable render-from-cache transcript: persisted blocks project into the content()
    // markdown, and re-reading after a "refocus" (a fresh store over the same db) still renders
    // them.
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
            QVERIFY2(md.contains(QStringLiteral("**Assistant:**")), qPrintable(md));
        }
        // Refocus / cold start: a new store over the same persisted db renders from disk.
        {
            DaemonCacheStore cache(db);
            CachedSessionStore store(&cache, nullptr);
            const QString md = store.content(domain::SessionId(QStringLiteral("s1")));
            QVERIFY2(md.contains(QStringLiteral("general kenobi")), qPrintable(md));
        }
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
