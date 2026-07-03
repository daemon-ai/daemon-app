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
#include "wire_mux_fixture.h"

#include <array>
#include <QCoreApplication>
#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

namespace {

// One session-log-entry map: {"seq", "direction", "origin", "disposition", "payload"} with a
// Command payload (zcbor decodes canonical member order, so the key order mirrors the CDDL).
void appendCommandEntry(QByteArray& b, quint64 seq, const char* direction, const char* disposition,
                        const QByteArray& commandBody) {
    using daemonapp::test::cborText;
    using daemonapp::test::cborUint;
    b.append(static_cast<char>(0xA5)); // map(5): the session-log-entry
    cborText(b, "seq");
    cborUint(b, seq);
    cborText(b, "direction");
    cborText(b, direction);
    cborText(b, "origin");
    b.append(static_cast<char>(0xA2)); // map(2): {"transport", "scope"}
    cborText(b, "transport");
    cborText(b, "api");
    cborText(b, "scope");
    cborText(b, "Internal");
    cborText(b, "disposition");
    cborText(b, disposition);
    cborText(b, "payload");
    b.append(static_cast<char>(0xA1)); // map(1): {"Command": agent-command}
    cborText(b, "Command");
    b.append(commandBody);
}

// A LogPage carrying the node's inbound command echoes: a StartTurn (the user's message), a
// Steer (a mid-turn user note), and an Interrupt (no user text).
QByteArray commandEchoLogPage() {
    using daemonapp::test::cborText;
    using daemonapp::test::cborUint;

    QByteArray startTurn; // {"StartTurn": {"input": {"text": ...}, "request_id": 1}}
    startTurn.append(static_cast<char>(0xA1));
    cborText(startTurn, "StartTurn");
    startTurn.append(static_cast<char>(0xA2));
    cborText(startTurn, "input");
    startTurn.append(static_cast<char>(0xA1)); // user-msg without the optional attachments
    cborText(startTurn, "text");
    cborText(startTurn, "hello node");
    cborText(startTurn, "request_id");
    cborUint(startTurn, 1);

    QByteArray steer; // {"Steer": {"text": ..., "request_id": 2}}
    steer.append(static_cast<char>(0xA1));
    cborText(steer, "Steer");
    steer.append(static_cast<char>(0xA2));
    cborText(steer, "text");
    cborText(steer, "go left");
    cborText(steer, "request_id");
    cborUint(steer, 2);

    QByteArray interrupt; // {"Interrupt": {"reason": null}}
    interrupt.append(static_cast<char>(0xA1));
    cborText(interrupt, "Interrupt");
    interrupt.append(static_cast<char>(0xA1));
    cborText(interrupt, "reason");
    interrupt.append(static_cast<char>(0xF6));

    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "LogPage");
    b.append(static_cast<char>(0xA4)); // {"entries", "next_seq", "head_seq", "epoch"}
    cborText(b, "entries");
    b.append(static_cast<char>(0x83)); // array(3)
    appendCommandEntry(b, 1, "Inbound", "Context", startTurn);
    appendCommandEntry(b, 2, "Inbound", "Context", steer);
    appendCommandEntry(b, 3, "Inbound", "Transport", interrupt);
    cborText(b, "next_seq");
    cborUint(b, 4);
    cborText(b, "head_seq");
    cborUint(b, 3);
    cborText(b, "epoch");
    cborUint(b, 0);
    return b;
}

// {"SessionCreated": {"session": <id>}}
QByteArray sessionCreatedResponse(const char* sessionId) {
    using daemonapp::test::cborText;
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "SessionCreated");
    b.append(static_cast<char>(0xA1));
    cborText(b, "session");
    cborText(b, sessionId);
    return b;
}

// {"Error": {"Other": <message>}}
QByteArray errorOtherResponse(const char* message) {
    using daemonapp::test::cborText;
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "Error");
    b.append(static_cast<char>(0xA1));
    cborText(b, "Other");
    cborText(b, message);
    return b;
}

// {"SessionPage": {"sessions":[{"session":<id>,"state":"Active"}...], ?"next_cursor":<id>,
//  "rev":N}} — keys in CDDL order (the generated decoder reads sequentially).
QByteArray sessionPageResponse(const QList<QByteArray>& ids, const char* nextCursor, quint64 rev) {
    using daemonapp::test::cborText;
    using daemonapp::test::cborTextLen;
    using daemonapp::test::cborUint;
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "SessionPage");
    b.append(static_cast<char>(nextCursor != nullptr ? 0xA3 : 0xA2));
    cborText(b, "sessions");
    b.append(static_cast<char>(0x80 | static_cast<char>(ids.size())));
    for (const QByteArray& id : ids) {
        b.append(static_cast<char>(0xA2));
        cborText(b, "session");
        cborTextLen(b, id);
        cborText(b, "state");
        cborText(b, "Active");
    }
    if (nextCursor != nullptr) {
        cborText(b, "next_cursor");
        cborText(b, nextCursor);
    }
    cborText(b, "rev");
    cborUint(b, rev);
    return b;
}

} // namespace

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

    // Per-user cache namespacing (auth6): switching the user namespace points the cache at a
    // distinct db file, so one user can never read another user's cached roster/sessions.
    void perUserCacheNamespacingIsolatesRows() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("daemon_cache.db")));
        QVERIFY(cache.isOpen());
        const QString basePath = cache.databasePath();

        // User A's roster.
        cache.setUserNamespace(QStringLiteral("user-A-opaque-id"));
        const QString pathA = cache.databasePath();
        QVERIFY(pathA != basePath); // a distinct, per-user db file
        daemonapp::daemon::CachedSessionRow a;
        a.sessionId = QStringLiteral("a-secret-session");
        a.title = QStringLiteral("A only");
        a.state = QStringLiteral("Active");
        a.updatedAtMs = 1;
        QVERIFY(cache.isOpen());
        QVERIFY(cache.upsertSession(a));
        QCOMPARE(cache.sessions().size(), 1);

        // Switch to user B: a different db file, and none of A's rows are visible.
        cache.setUserNamespace(QStringLiteral("user-B-opaque-id"));
        const QString pathB = cache.databasePath();
        QVERIFY(pathB != pathA);
        QVERIFY(cache.sessions().isEmpty()); // no cross-user leak
        daemonapp::daemon::CachedSessionRow b;
        b.sessionId = QStringLiteral("b-session");
        b.state = QStringLiteral("Active");
        b.updatedAtMs = 2;
        QVERIFY(cache.upsertSession(b));
        QCOMPARE(cache.sessions().size(), 1);
        QCOMPARE(cache.sessions().first().sessionId, QStringLiteral("b-session"));

        // Back to A: A's row is still there (its own db), B's is not.
        cache.setUserNamespace(QStringLiteral("user-A-opaque-id"));
        QCOMPARE(cache.databasePath(), pathA);
        QCOMPARE(cache.sessions().size(), 1);
        QCOMPARE(cache.sessions().first().sessionId, QStringLiteral("a-secret-session"));

        // Empty namespace resets to the shared/default db (pre-auth / local-trust).
        cache.setUserNamespace(QString());
        QCOMPARE(cache.databasePath(), basePath);
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

    // The node records the user's inbound commands on the merged log; the decoder must surface
    // the conversational text (StartTurn input / Steer text) so the transcript can render the
    // user's message from the node's echo (bug fix 1a: no optimistic client fabrication).
    void codecDecodesCommandEchoText() {
        using daemonapp::daemon::DecodedLogEntry;
        QList<DecodedLogEntry> entries;
        quint64 nextSeq = 0;
        quint64 headSeq = 0;
        quint64 epoch = 99;
        QVERIFY(daemonapp::daemon::NodeApiCodec::decodeLogPageEntries(
            commandEchoLogPage(), &entries, &nextSeq, &headSeq, &epoch));
        QCOMPARE(entries.size(), 3);

        QCOMPARE(entries.at(0).payloadKind, DecodedLogEntry::PayloadKind::Command);
        QCOMPARE(entries.at(0).direction, QStringLiteral("Inbound"));
        QCOMPARE(entries.at(0).seq, static_cast<quint64>(1));
        QCOMPARE(entries.at(0).commandText, QStringLiteral("hello node"));

        QCOMPARE(entries.at(1).payloadKind, DecodedLogEntry::PayloadKind::Command);
        QCOMPARE(entries.at(1).commandText, QStringLiteral("go left"));

        // Non-conversational arms carry no user text.
        QCOMPARE(entries.at(2).payloadKind, DecodedLogEntry::PayloadKind::Command);
        QVERIFY(entries.at(2).commandText.isEmpty());

        QCOMPARE(nextSeq, static_cast<quint64>(4));
        QCOMPARE(headSeq, static_cast<quint64>(3));
        QCOMPARE(epoch, static_cast<quint64>(0));
    }

    // Bug fix 1b: a SessionCreated reply upserts a minimal cached row immediately (the All
    // Sessions list must not wait on the debounced roster refetch), emits sessionCreated for the
    // auto-select path, and issues an immediate non-debounced SessionsQuery for the full row.
    void sessionCreateUpsertsCacheAndRefreshes() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("create.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(sessionCreatedResponse("s-node-1"));
        QVERIFY2(fake.start(path), "listen");

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("cache.db")));
        daemonapp::daemon::SessionRepository sessions(&client, &cache);

        QSignalSpy created(&sessions, &daemonapp::daemon::SessionRepository::sessionCreated);
        QSignalSpy refreshed(&sessions, &daemonapp::daemon::SessionRepository::sessionsRefreshed);
        QVERIFY(created.isValid());
        QVERIFY(refreshed.isValid());

        sessions.createSession(QStringLiteral("agent-x"));
        QTRY_COMPARE_WITH_TIMEOUT(created.count(), 1, 3000);
        QCOMPARE(created.at(0).at(0).toString(), QStringLiteral("s-node-1"));
        QCOMPARE(created.at(0).at(1).toString(), QStringLiteral("agent-x"));

        // The minimal row is in the cache before any roster refetch resolves, and the store-level
        // refresh signal fired so the UI reprojects now.
        QCOMPARE(cache.sessions().size(), 1);
        QCOMPARE(cache.sessions().first().sessionId, QStringLiteral("s-node-1"));
        QCOMPARE(cache.sessions().first().profileRef, QStringLiteral("agent-x"));
        QCOMPARE(cache.sessions().first().state, QStringLiteral("Ready"));
        QVERIFY(cache.sessions().first().title.isEmpty());
        QVERIFY(refreshed.count() >= 1);

        // The immediate authoritative refetch went out (Call #2, no debounce). Its canned reply
        // is not a SessionPage here; only the dispatch is under test.
        QTRY_VERIFY_WITH_TIMEOUT(fake.requestCount() >= 2, 3000);
    }

    // Bug fix 1b (failure surfacing): a failed create must not vanish silently — both the node's
    // ApiError reply and a transport-level failure surface through refreshFailed.
    void sessionCreateErrorSurfaces() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("create-err.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(errorOtherResponse("no such profile"));
        QVERIFY2(fake.start(path), "listen");

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("cache-err.db")));
        daemonapp::daemon::SessionRepository sessions(&client, &cache);

        QSignalSpy created(&sessions, &daemonapp::daemon::SessionRepository::sessionCreated);
        QSignalSpy failed(&sessions, &daemonapp::daemon::SessionRepository::refreshFailed);
        sessions.createSession(QStringLiteral("ghost"));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("no such profile"));
        QCOMPARE(created.count(), 0);
        QVERIFY(cache.sessions().isEmpty());

        // Transport-level failure (no listener at all) routes through handleFailure's new
        // create-correlation arm rather than being dropped.
        daemonapp::daemon::DaemonTransport deadTransport;
        deadTransport.setSocketPath(dir.filePath(QStringLiteral("absent.sock")));
        daemonapp::daemon::NodeApiClient deadClient(&deadTransport);
        daemonapp::daemon::SessionRepository deadSessions(&deadClient, &cache);
        QSignalSpy deadFailed(&deadSessions, &daemonapp::daemon::SessionRepository::refreshFailed);
        deadSessions.createSession(QString());
        QTRY_COMPARE_WITH_TIMEOUT(deadFailed.count(), 1, 3000);
    }

    // Wire v24 roster page loop: a full refresh follows next_cursor to completion, the
    // continuation query carries `after`, and the replace + prune runs ONCE over the whole
    // accumulated set — a stale cached session is gone at the end, both pages' rows are present,
    // and sessionsRefreshed fires exactly once.
    void sessionsFullRefreshPagesToCompletion() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("roster.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplySequence({sessionPageResponse({QByteArrayLiteral("s1")}, "s1", 7),
                               sessionPageResponse({QByteArrayLiteral("s2")}, nullptr, 7)});
        QVERIFY2(fake.start(path), "listen");

        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        daemonapp::daemon::NodeApiClient client(&transport);
        daemonapp::daemon::DaemonCacheStore cache(dir.filePath(QStringLiteral("roster.db")));
        // A stale cached row the full listing no longer contains: the loop's final replace must
        // prune it (pruning per page would also have dropped s2 between the pages).
        daemonapp::daemon::CachedSessionRow stale;
        stale.sessionId = QStringLiteral("stale-1");
        stale.state = QStringLiteral("Active");
        stale.updatedAtMs = 1;
        QVERIFY(cache.upsertSession(stale));

        daemonapp::daemon::SessionRepository sessions(&client, &cache);
        QSignalSpy refreshed(&sessions, &daemonapp::daemon::SessionRepository::sessionsRefreshed);
        sessions.refreshSessions();
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QTest::qWait(100); // settle: a per-page emit or a runaway loop would fire again
        QCOMPARE(refreshed.count(), 1);

        // The merged roster holds both pages' rows; the stale row was pruned.
        QStringList ids;
        for (const auto& row : cache.sessions()) {
            ids << row.sessionId;
        }
        ids.sort();
        QCOMPARE(ids, (QStringList{QStringLiteral("s1"), QStringLiteral("s2")}));

        // The continuation query carried the first page's next_cursor as `after`.
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(1), daemonapp::daemon::NodeApiCodec::encodeSessionsQueryRequest(
                                  false, 0, QString(), QStringLiteral("s1")));
    }
};

QTEST_MAIN(DaemonIntegrationTests)

#include "tst_daemon_integration.moc"
