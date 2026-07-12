// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
using daemonapp::daemon::CachedTranscriptBlockRow;
using daemonapp::daemon::CachedTransportInstanceRow; // [wave2:app-channels-liveness]
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::ModelRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::ProfileRepository; // profile roster refetch (wire v31, phase H)
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

    // [waveB:app-v30] D2: a ConversationsChanged event refetches that transport's ConvList (an
    // invalidation pointer — the client derives no membership itself). This is what replaces the
    // retired per-tab-enter / per-expand polling.
    void conversationsChangedRefetchesConvList() {
        const QString path = sock(QStringLiteral("convch.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("convch.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        TransportRepository transports(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        mgr.setTransportRepository(&transports);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neConversationsChanged("matrix/@bot:hs", "!room:hs", "Added")}, 1,
            1));
        // The routed refreshConversations() issues a ConvList one-shot Call.
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() > callsBefore, 3000);
    }

    // [waveB:app-v30] D2 → AD: a MembershipChanged refetches the transport's ConvList (the
    // repo-side arm). The routing pin-table + roster refetches are the mirror ingestor's arms.
    void membershipChangedRefetchesConvList() {
        const QString path = sock(QStringLiteral("memberch.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("memberch.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        TransportRepository transports(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache);
        mgr.setTransportRepository(&transports);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        // is_self=true, Kicked -> the transport's ConvList refetch (one new Call).
        fake.pushItem(daemonapp::test::buildEventsPage(
            {daemonapp::test::neMembershipChanged("matrix/@bot:hs", "!room:hs", "@bot:hs", "Kicked",
                                                  true)},
            1, 1));
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() >= callsBefore + 1, 3000);
    }

    // Phase H: a ProfilesChanged event refetches the profile roster (ProfileList) — the
    // invalidation pointer the node fires on every profile create/edit/delete/select (operator or
    // agent-authored via profile_manage), so the provenance/filter surfaces re-render without
    // polling.
    void profilesChangedRefetchesProfiles() {
        const QString path = sock(QStringLiteral("profilesch.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(dbPath(QStringLiteral("profilesch.db")));
        SessionRepository sessions(&client, &cache);
        ApprovalRepository approvals(&client, &cache);
        ModelRepository models(&client, &cache);
        ProfileRepository profiles(&client, &cache);
        SubscriptionManager mgr(&client, &sessions, &approvals, &models, &cache, &profiles);

        mgr.start();
        QTRY_VERIFY_WITH_TIMEOUT(fake.lastOpenId() != 0, 3000);
        const int callsBefore = fake.requestCount();

        fake.pushItem(
            daemonapp::test::buildEventsPage({daemonapp::test::neProfilesChanged(4)}, 1, 1));
        // The routed refreshProfiles() issues a ProfileList one-shot Call.
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
};

QTEST_GUILESS_MAIN(TestSubscriptionManager)
#include "tst_subscription_manager.moc"
