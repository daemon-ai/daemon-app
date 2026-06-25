#include "daemon/client_cache_schema.h"
#include "daemon/cache_consolidation_policy.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/integration_slice.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon/seam_migration.h"
#include "persistence/in_memory_session_store.h"

#include <QtTest/QtTest>
#include <QTemporaryDir>

#include <array>

class DaemonIntegrationTests : public QObject {
    Q_OBJECT

private slots:
    void transportFramesRoundTrip()
    {
        const QByteArray payload("\xA1\x66Health\xF6", 9);
        QByteArray framed = daemonapp::daemon::DaemonTransport::framePayload(payload);
        QByteArray decoded;
        QVERIFY(daemonapp::daemon::DaemonTransport::tryTakeFrame(framed, &decoded));
        QCOMPARE(decoded, payload);
        QVERIFY(framed.isEmpty());
    }

    void transportWaitsForCompleteFrame()
    {
        QByteArray framed = daemonapp::daemon::DaemonTransport::framePayload(QByteArray("abc", 3));
        QByteArray partial = framed.left(5);
        QByteArray decoded;
        QVERIFY(!daemonapp::daemon::DaemonTransport::tryTakeFrame(partial, &decoded));
        partial.append(framed.mid(5));
        QVERIFY(daemonapp::daemon::DaemonTransport::tryTakeFrame(partial, &decoded));
        QCOMPARE(decoded, QByteArray("abc", 3));
    }

    void cacheSchemaUsesDaemonIdsAndStructuredLogs()
    {
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateSessionsSql).contains("session_id TEXT PRIMARY KEY"));
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateSessionLogSql).contains("payload_cbor BLOB"));
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateSyncCursorsSql).contains("cursor TEXT"));
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateApprovalsSql).contains("request_id TEXT"));
    }

    void firstSliceIsOrderedForThinClientBootstrap()
    {
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.front(), daemonapp::daemon::IntegrationStep::ConnectUnixSocket);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(1), daemonapp::daemon::IntegrationStep::Health);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(2), daemonapp::daemon::IntegrationStep::SessionsQuery);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(3), daemonapp::daemon::IntegrationStep::Submit);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(4), daemonapp::daemon::IntegrationStep::Subscribe);
        QCOMPARE(daemonapp::daemon::integrationStepName(daemonapp::daemon::IntegrationStep::PersistCursor),
                 QStringView(u"persist-cursor"));
    }

    void cacheStorePersistsRowsAndCursors()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        QVERIFY(cache.isOpen());

        daemonapp::daemon::CachedSessionRow session;
        session.sessionId = QStringLiteral("s1");
        session.title = QStringLiteral("Hello");
        session.state = QStringLiteral("Active");
        session.updatedAtMs = 10;
        QVERIFY(cache.upsertSession(session));
        QCOMPARE(cache.sessions().size(), 1);
        QCOMPARE(cache.sessions().first().sessionId, QStringLiteral("s1"));

        daemonapp::daemon::CachedLogRow log;
        log.sessionId = QStringLiteral("s1");
        log.seq = 7;
        log.payloadCbor = QByteArray("\xA1", 1);
        log.direction = QStringLiteral("Inbound");
        log.disposition = QStringLiteral("Context");
        log.updatedAtMs = 11;
        QVERIFY(cache.appendSessionLog(log));
        QCOMPARE(cache.sessionLog(QStringLiteral("s1")).first().payloadCbor, QByteArray("\xA1", 1));

        QVERIFY(cache.setCursor(QStringLiteral("session-log/s1"), QStringLiteral("7"), 12));
        QCOMPARE(cache.cursor(QStringLiteral("session-log/s1")), QStringLiteral("7"));

        daemonapp::daemon::CachedApprovalRow approval;
        approval.sessionId = QStringLiteral("s1");
        approval.requestId = QStringLiteral("r1");
        approval.prompt = QStringLiteral("Allow?");
        approval.updatedAtMs = 13;
        QVERIFY(cache.upsertApproval(approval));
        QCOMPARE(cache.approvals().first().requestId, QStringLiteral("r1"));
    }

    void repositoriesUseCache()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        daemonapp::daemon::SessionRepository sessions(nullptr, &cache);

        daemonapp::daemon::CachedSessionRow row;
        row.sessionId = QStringLiteral("s2");
        row.state = QStringLiteral("Ready");
        row.updatedAtMs = 20;
        QVERIFY(sessions.upsertCachedSession(row));
        QCOMPARE(sessions.cachedSessions().first().sessionId, QStringLiteral("s2"));
        QVERIFY(sessions.setLogCursor(QStringLiteral("s2"), 42));
        QCOMPARE(sessions.logCursor(QStringLiteral("s2")), 42);
    }

    void domainSessionsCarryDaemonIds()
    {
        persistence::InMemorySessionStore store;
        const auto sessions = store.sessions(domain::ListScope{});
        QVERIFY(!sessions.isEmpty());
        QVERIFY(!sessions.first().sessionId.isEmpty());
        QVERIFY(sessions.first().id >= 0);
    }

    void cachePolicyDocumentsDaemonModeOwnership()
    {
        using daemonapp::daemon::cache::PersistenceOwner;
        const auto& policies = daemonapp::daemon::cache::kPersistencePolicies;
        QCOMPARE(policies[0].owner, PersistenceOwner::ClientLocalSettings);
        QCOMPARE(policies[1].owner, PersistenceOwner::MockModeOnly);
        QCOMPARE(policies[2].owner, PersistenceOwner::DaemonAuthoritativeCache);
        QCOMPARE(policies[3].owner, PersistenceOwner::ImportOnlyLegacy);
    }

    void seamMigrationTrackerCoversDaemonSeams()
    {
        using daemonapp::daemon::migration::SeamMigrationStatus;
        const auto& targets = daemonapp::daemon::migration::kTargets;
        QCOMPARE(std::size(targets), static_cast<size_t>(6));
        // The session store is the furthest along (additive SessionId landed); the rest are still
        // mock-only until their daemon-api codec subsets exist.
        QCOMPARE(targets[0].status, SeamMigrationStatus::AdditiveIdReady);
        QVERIFY(QString::fromUtf8(targets[0].seam).contains(QStringLiteral("ISessionStore")));
    }

    void schemaVersionLivesInMetaTable()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        QVERIFY(cache.isOpen());
        QCOMPARE(cache.schemaVersion(), daemonapp::daemon::cache::kSchemaVersion);
        QCOMPARE(cache.meta(QStringLiteral("schema_version")),
                 QString::number(daemonapp::daemon::cache::kSchemaVersion));
        // The schema version must NOT be parked in the sync-cursor table any more.
        QVERIFY(cache.cursor(QStringLiteral("schema_version")).isEmpty());
    }

    void cacheStoresAndReadsProfiles()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        daemonapp::daemon::CachedProfileRow profile;
        profile.profileRef = QStringLiteral("default");
        profile.displayName = QStringLiteral("Default");
        profile.specCbor = QByteArray("\xA0", 1);
        profile.active = true;
        profile.updatedAtMs = 5;
        QVERIFY(cache.upsertProfile(profile));
        QCOMPARE(cache.profiles().size(), 1);
        QCOMPARE(cache.profiles().first().profileRef, QStringLiteral("default"));
        QVERIFY(cache.profiles().first().active);
    }

    void codecEncodesFirstSliceRequests()
    {
        // Health is the bare CBOR text string "Health".
        const QByteArray health = daemonapp::daemon::NodeApiCodec::encodeHealthRequest();
        QCOMPARE(health, QByteArray("\x66Health", 7));
        // SessionsQuery is a CBOR map {"SessionsQuery": {"query": {}}}; zcbor emits maps with the
        // indefinite-length marker (0xBF), so assert the major type is "map" (top 3 bits == 5)
        // rather than a specific length encoding.
        const QByteArray query = daemonapp::daemon::NodeApiCodec::encodeSessionsQueryRequest();
        QVERIFY(!query.isEmpty());
        QCOMPARE(static_cast<quint8>(query.front()) >> 5, 5);
    }

    void codecDecodesHealthResponse()
    {
        // {"Health": {"all_ok": true, "services": []}}
        const QByteArray response("\xA1\x66Health\xA2\x66""all_ok\xF5\x68services\x80", 27);
        daemonapp::daemon::DecodedHealth health;
        QVERIFY(daemonapp::daemon::NodeApiCodec::decodeHealth(response, &health));
        QVERIFY(health.allOk);
        QVERIFY(health.services.isEmpty());
        QCOMPARE(daemonapp::daemon::NodeApiCodec::responseKind(response),
                 daemonapp::daemon::ApiResponseKind::Health);
    }

    void codecDecodesSessionPageResponse()
    {
        // {"SessionPage": {"sessions": [ {"session": "s1", "state": "Active"} ]}}
        const QByteArray response(
            "\xA1\x6BSessionPage\xA1\x68sessions\x81\xA2\x67session\x62s1\x65state\x66""Active", 49);
        QList<daemonapp::daemon::CachedSessionRow> rows;
        QVERIFY(daemonapp::daemon::NodeApiCodec::decodeSessionPage(response, &rows));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.first().sessionId, QStringLiteral("s1"));
        QCOMPARE(rows.first().state, QStringLiteral("Active"));
    }
};

QTEST_MAIN(DaemonIntegrationTests)

#include "tst_daemon_integration.moc"
