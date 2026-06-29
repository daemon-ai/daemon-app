// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/cache_consolidation_policy.h"
#include "daemon/client_cache_schema.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/integration_slice.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon/seam_migration.h"
#include "persistence/in_memory_session_store.h"

#include <array>
#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class DaemonIntegrationTests : public QObject {
    Q_OBJECT

private slots:
    void transportFramesRoundTrip() {
        const QByteArray payload("\xA1\x66Health\xF6", 9);
        QByteArray framed = daemonapp::daemon::DaemonTransport::framePayload(payload);
        QByteArray decoded;
        QVERIFY(daemonapp::daemon::DaemonTransport::tryTakeFrame(framed, &decoded));
        QCOMPARE(decoded, payload);
        QVERIFY(framed.isEmpty());
    }

    void transportWaitsForCompleteFrame() {
        QByteArray framed = daemonapp::daemon::DaemonTransport::framePayload(QByteArray("abc", 3));
        QByteArray partial = framed.left(5);
        QByteArray decoded;
        QVERIFY(!daemonapp::daemon::DaemonTransport::tryTakeFrame(partial, &decoded));
        partial.append(framed.mid(5));
        QVERIFY(daemonapp::daemon::DaemonTransport::tryTakeFrame(partial, &decoded));
        QCOMPARE(decoded, QByteArray("abc", 3));
    }

    void cacheSchemaUsesDaemonIdsAndStructuredBlocks() {
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateSessionsSql)
                    .contains("session_id TEXT PRIMARY KEY"));
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateSyncCursorsSql)
                    .contains("cursor TEXT"));
        // The transcript is cached as coalesced blocks (the retired
        // daemon_session_log/daemon_approvals tables are gone; pending approvals are live, the
        // transcript renders from these blocks).
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateTranscriptBlocksSql)
                    .contains("daemon_transcript_blocks"));
        QVERIFY(QString::fromUtf8(daemonapp::daemon::cache::kCreateProfilesSql)
                    .contains("spec_cbor BLOB"));
    }

    void firstSliceIsOrderedForThinClientBootstrap() {
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.front(),
                 daemonapp::daemon::IntegrationStep::ConnectUnixSocket);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(1),
                 daemonapp::daemon::IntegrationStep::Health);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(2),
                 daemonapp::daemon::IntegrationStep::SessionsQuery);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(3),
                 daemonapp::daemon::IntegrationStep::Submit);
        QCOMPARE(daemonapp::daemon::kFirstIntegrationSlice.at(4),
                 daemonapp::daemon::IntegrationStep::Subscribe);
        QCOMPARE(daemonapp::daemon::integrationStepName(
                     daemonapp::daemon::IntegrationStep::PersistCursor),
                 QStringView(u"persist-cursor"));
    }

    void cacheStorePersistsRowsAndCursors() {
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

        // The generic cursor KV (used for the roster rev, per-session watermark/epoch,
        // events-since).
        QVERIFY(cache.setCursor(QStringLiteral("roster-rev"), QStringLiteral("7"), 12));
        QCOMPARE(cache.cursor(QStringLiteral("roster-rev")), QStringLiteral("7"));
    }

    void repositoriesUseCache() {
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
    }

    void domainSessionsCarryDaemonIds() {
        persistence::InMemorySessionStore store;
        const auto sessions = store.sessions(domain::ListScope{});
        QVERIFY(!sessions.isEmpty());
        QVERIFY(!sessions.first().sessionId.isEmpty());
        QVERIFY(sessions.first().id >= 0);
    }

    void clientFailsAndRecoversOnBadSocket() {
        // A socket path that does not exist makes connectToServer raise errorOccurred. The
        // transport must reset cleanly and the client must fail the in-flight request and stay
        // usable.
        const QString bogus =
            QDir::tempPath() +
            QStringLiteral("/daemon-app-bogus-%1.sock").arg(QCoreApplication::applicationPid());
        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(bogus);
        daemonapp::daemon::NodeApiClient client(&transport);

        QSignalSpy failures(&client, &daemonapp::daemon::NodeApiClient::failed);
        QVERIFY(failures.isValid());

        // QTRY_COMPARE polls while pumping events, so it is robust whether errorOccurred is
        // delivered synchronously during sendRequest or asynchronously afterwards.
        client.sendRequest(QByteArray("\x66Health", 7), QStringLiteral("c1"));
        QTRY_COMPARE_WITH_TIMEOUT(failures.count(), 1, 3000);
        QCOMPARE(failures.at(0).at(0).toString(), QStringLiteral("c1"));

        // The in-flight flag must have reset: a second request must dispatch and fail in turn
        // rather than stall behind the first.
        client.sendRequest(QByteArray("\x66Health", 7), QStringLiteral("c2"));
        QTRY_COMPARE_WITH_TIMEOUT(failures.count(), 2, 3000);
        QCOMPARE(failures.at(1).at(0).toString(), QStringLiteral("c2"));
    }

    void cachePolicyDocumentsDaemonModeOwnership() {
        using daemonapp::daemon::cache::PersistenceOwner;
        const auto& policies = daemonapp::daemon::cache::kPersistencePolicies;
        QCOMPARE(policies[0].owner, PersistenceOwner::ClientLocalSettings);
        QCOMPARE(policies[1].owner, PersistenceOwner::MockModeOnly);
        QCOMPARE(policies[2].owner, PersistenceOwner::DaemonAuthoritativeCache);
        QCOMPARE(policies[3].owner, PersistenceOwner::ImportOnlyLegacy);
    }

    void seamMigrationTrackerCoversDaemonSeams() {
        using daemonapp::daemon::migration::SeamMigrationStatus;
        const auto& targets = daemonapp::daemon::migration::kTargets;
        QCOMPARE(std::size(targets), static_cast<size_t>(8));
        // The session store is the furthest along (additive SessionId landed); the rest are still
        // mock-only until their daemon-api codec subsets exist.
        QCOMPARE(targets[0].status, SeamMigrationStatus::AdditiveIdReady);
        QVERIFY(QString::fromUtf8(targets[0].seam).contains(QStringLiteral("ISessionStore")));
    }

    void schemaVersionLivesInMetaTable() {
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

    void cacheStoresAndReadsProfiles() {
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

    void codecEncodesFirstSliceRequests() {
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

    void codecDecodesHealthResponse() {
        // {"Health": {"all_ok": true, "services": []}}
        const QByteArray response("\xA1\x66Health\xA2\x66"
                                  "all_ok\xF5\x68services\x80",
                                  27);
        daemonapp::daemon::DecodedHealth health;
        QVERIFY(daemonapp::daemon::NodeApiCodec::decodeHealth(response, &health));
        QVERIFY(health.allOk);
        QVERIFY(health.services.isEmpty());
        QCOMPARE(daemonapp::daemon::NodeApiCodec::responseKind(response),
                 daemonapp::daemon::ApiResponseKind::Health);
    }

    void codecDecodesSessionPageResponse() {
        // {"SessionPage": {"sessions": [ {"session": "s1", "state": "Active"} ], "rev": 0}}
        // (L4 added the required "rev" field; "removed" is optional and omitted here.)
        const QByteArray response(
            "\xA1\x6BSessionPage\xA2\x68sessions\x81\xA2\x67session\x62s1\x65state\x66"
            "Active\x63rev\x00",
            54);
        QList<daemonapp::daemon::CachedSessionRow> rows;
        quint64 rev = 999;
        QVERIFY(daemonapp::daemon::NodeApiCodec::decodeSessionPage(response, &rows, nullptr, &rev));
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.first().sessionId, QStringLiteral("s1"));
        QCOMPARE(rows.first().state, QStringLiteral("Active"));
        QCOMPARE(rev, static_cast<quint64>(0));
    }

    void codecEncodesSubscribeRequest() {
        const QByteArray sub =
            daemonapp::daemon::NodeApiCodec::encodeSubscribeRequest(QStringLiteral("s1"), 3, 64);
        QVERIFY(!sub.isEmpty());
        // {"Subscribe": {...}} is a CBOR map (major type 5).
        QCOMPARE(static_cast<quint8>(sub.front()) >> 5, 5);
    }

    void codecDecodesLogPageCursors() {
        // {"LogPage": {"entries": [], "next_seq": 5, "head_seq": 5, "epoch": 0}} (L2 added epoch)
        const QByteArray response("\xA1\x67LogPage\xA4\x67"
                                  "entries\x80\x68next_seq\x05\x68head_seq\x05\x65"
                                  "epoch\x00",
                                  46);
        QList<daemonapp::daemon::CachedLogRow> rows;
        quint64 nextSeq = 0;
        quint64 headSeq = 0;
        QVERIFY(daemonapp::daemon::NodeApiCodec::decodeLogPage(response, QStringLiteral("s1"),
                                                               &rows, &nextSeq, &headSeq));
        QVERIFY(rows.isEmpty());
        QCOMPARE(nextSeq, static_cast<quint64>(5));
        QCOMPARE(headSeq, static_cast<quint64>(5));
        QCOMPARE(daemonapp::daemon::NodeApiCodec::responseKind(response),
                 daemonapp::daemon::ApiResponseKind::LogPage);
    }
};

QTEST_MAIN(DaemonIntegrationTests)

#include "tst_daemon_integration.moc"
