// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QSqlQuery)

namespace daemonapp::daemon {

struct CachedSessionRow {
    QString sessionId;
    QString title;
    QString state;
    QString profileRef;
    QString lifecycle;
    QString role;
    QString parentSessionId;
    bool pinned = false;
    bool archived = false;
    qint64 updatedAtMs = 0;
};

struct CachedLogRow {
    QString sessionId;
    quint64 seq = 0;
    QByteArray payloadCbor;
    QString direction;
    QString disposition;
    qint64 updatedAtMs = 0;
};

struct CachedFsEntryRow {
    QString rootId;
    QString path;
    QString kind;
    quint64 size = 0;
    qint64 mtimeMs = 0;
    QByteArray revisionCbor;
    qint64 updatedAtMs = 0;
};

struct CachedProfileRow {
    QString profileRef;
    QString displayName;
    QByteArray specCbor;
    bool active = false;
    qint64 updatedAtMs = 0;
};

// One unit of the offline-first fleet/subagent tree (daemon_fleet_units; schema v4).
// `depth`/`ordinal` preserve the pre-order flatten so the tree re-renders from cache without a
// connection.
struct CachedFleetUnitRow {
    QString unitId;
    QString parentId; // "" = a root
    int depth = 0;
    int ordinal = 0; // position in the pre-order flatten
    QString name;
    QString kind;  // "Engine" | "Host" | "Orchestrator"
    QString state; // "Running" | "Finished" | "Unknown"
    QString role;
    QString profileRef;
    QString sessionId;
    QString work;
    // [wave2:app-delegation] v7 (F3): authoritative per-child lifetime + engine (UnitNode v29).
    QString lifetime;    // "" | "Persistent" | "Ephemeral"
    QString engineKind;  // "" | "Core" | "Foreign"
    QString engineAgent; // foreign agent name (only when engineKind == "Foreign")
    // [waveB:app-v30] v8 (stretch): UnitNode.end_reason for a Finished unit ("Completed" |
    // "Failed" | "Interrupted" | ...); "" for a running unit. The subagent strip reads this to
    // render an error status (node-reported, not client-derived).
    QString endReason;
    qint64 updatedAtMs = 0;
};

// One offline-first transport instance/account row (daemon_transport_instances; Phase 6a).
struct CachedTransportInstanceRow {
    QString transport; // pk (instance-qualified id)
    QString family;
    QString displayName;
    QString connection = QStringLiteral("offline");
    QString presence = QStringLiteral("unknown");
    QString boundProfile;
    // [waveB:app-v30] v8 (D1): node-reported disconnect provenance. `connectionReason` is a coarse
    // token (disconnectReasonName), `connectionMessage` the node's verbatim human string, `fatal`
    // gates the re-auth affordance. Empty/false when the node reported none.
    QString connectionReason;
    QString connectionMessage;
    bool fatal = false;
    // [acct-mgmt] v9 (wire v35): node-persisted desired state + display-label override. `enabled`
    // defaults true; `label` (empty = none) overlays displayName wherever the account renders.
    bool enabled = true;
    QString label;
    qint64 updatedAtMs = 0;
};

// One offline-first conversation row (daemon_conversations; Phase 6a).
struct CachedConversationRow {
    QString transport;
    QString convId;
    QString kind;
    QString title;
    QString topic;
    qint64 updatedAtMs = 0;
};

// One coalesced durable transcript block (schema v2; render-from-cache). Mirrors
// DecodedTranscriptBlock (journal) / the live coalesced shape, keyed by (sessionId, seq).
struct CachedTranscriptBlockRow {
    QString sessionId;
    quint64 seq = 0;
    QString kind; // "Message" | "ToolCall" | "ToolResult" | "Request" | "Content" | "Other"
    QString role; // Message: "Assistant" | "User" | "System"
    QString text;
    QString callId;
    QString toolName;
    QString argsSummary;
    bool ok = false;
    QString summary;
    QString detailKind;    // ToolResult structured detail kind (D1; empty when absent)
    QByteArray detailBody; // ToolResult structured detail payload (JSON bytes)
    quint32 requestId = 0;
    QString hostKind;
    QString contentKind;
    qint64 updatedAtMs = 0;
};

class DaemonCacheStore : public QObject {
public:
    explicit DaemonCacheStore(const QString& dbPath = QString(), QObject* parent = nullptr);
    ~DaemonCacheStore() override;

    [[nodiscard]] QString databasePath() const { return m_dbPath; }
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] QString lastError() const { return m_lastError; }

    // Switch the cache to a per-user namespace so switching users cannot surface the prior user's
    // cached roster/sessions/transcripts. `userKey` is the authenticated principal's opaque user_id
    // (hashed into the filename); empty resets to the shared/default db (pre-auth / local-trust).
    // Reopening is cheap and safe: the cache is non-authoritative and re-baselines from the daemon
    // on the next ready. No-op if the resulting path is already open.
    void setUserNamespace(const QString& userKey);

    // Persisted schema version, read from daemon_cache_meta (0 if absent/unopened).
    [[nodiscard]] int schemaVersion() const;

    bool upsertSession(const CachedSessionRow& row);
    [[nodiscard]] QList<CachedSessionRow> sessions() const;
    // Remove a session row (L4 delta prune: a `removed` id or a session that left the queried
    // scope). Best-effort; a missing row is not an error.
    bool deleteSession(const QString& sessionId);

    bool setCursor(const QString& scope, const QString& cursor, qint64 updatedAtMs);
    [[nodiscard]] QString cursor(const QString& scope) const;

    bool upsertFsEntry(const CachedFsEntryRow& row);
    [[nodiscard]] QList<CachedFsEntryRow> fsEntries(const QString& rootId) const;

    bool upsertProfile(const CachedProfileRow& row);
    [[nodiscard]] QList<CachedProfileRow> profiles() const;
    // Remove a cached profile (a live ProfileList no longer lists it). Best-effort.
    bool deleteProfile(const QString& profileRef);

    // Offline-first fleet tree (daemon_fleet_units). fleetUnits() returns pre-order (ORDER BY
    // ordinal).
    bool upsertFleetUnit(const CachedFleetUnitRow& row);
    [[nodiscard]] QList<CachedFleetUnitRow> fleetUnits() const;
    bool deleteFleetUnit(const QString& unitId);

    // Offline-first Channels read surface (Phase 6a). Instances are accounts (ORDER BY family,
    // display_name); conversations are scoped per transport.
    bool upsertTransportInstance(const CachedTransportInstanceRow& row);
    [[nodiscard]] QList<CachedTransportInstanceRow> transportInstances() const;
    bool deleteTransportInstance(const QString& transport);
    bool upsertConversation(const CachedConversationRow& row);
    [[nodiscard]] QList<CachedConversationRow> conversations(const QString& transport) const;
    bool deleteConversation(const QString& transport, const QString& convId);

    // Durable transcript blocks (schema v2 render-from-cache). upsert is idempotent per (session,
    // seq); transcriptBlocks reads in seq order past `afterSeq`; clearTranscript wipes a session's
    // blocks for a re-baseline (epoch change / Reset).
    bool upsertTranscriptBlock(const CachedTranscriptBlockRow& row);
    [[nodiscard]] QList<CachedTranscriptBlockRow> transcriptBlocks(const QString& sessionId,
                                                                   quint64 afterSeq = 0) const;
    bool clearTranscript(const QString& sessionId);
    // The latest non-empty Message text per session (one grouped query), for an offline roster
    // snippet/preview. Maps sessionId -> last message text.
    [[nodiscard]] QHash<QString, QString> latestTranscriptSnippets() const;

    // Generic typed metadata stored in daemon_cache_meta (schema version, etc).
    bool setMeta(const QString& key, const QString& value);
    [[nodiscard]] QString meta(const QString& key) const;

    // Wipe every cached row (drop + recreate the data tables) for the "Clear local data" action;
    // the cache re-baselines from the daemon on the next ready. Returns true on success. Consumed
    // by Application::clearLocalData() (W4). TODO(W2): implement the drop/recreate + sibling-file
    // cleanup.
    bool clearAll();

    // Approximate total cached row count across the data tables, for the boot cache-rows sentinel
    // (kSentinelCacheRowsPrefix). Cheap/best-effort; exact accuracy is not required. TODO: return
    // the real aggregate count (W2 populates it; Stream B's sentinel consumes it).
    [[nodiscard]] int approxRowCount() const;

private:
    [[nodiscard]] static QString defaultDatabasePath();
    // The per-user db path derived from the base path + a hashed userKey (base path when empty).
    [[nodiscard]] QString namespacedPath(const QString& userKey) const;
    // (Re)open the SQLite connection at `path` and (re)create the schema. Replaces any open db. On
    // an open/integrity/schema failure the (torn/corrupt) file is discarded and rebuilt fresh - the
    // cache is non-authoritative, so the daemon re-baselines it on reconnect.
    void openAt(const QString& path);
    // Add + open the QSQLITE connection at m_dbPath, gate it on PRAGMA integrity_check, then build
    // the schema. Returns false (leaving the connection closed) on any of those failing.
    bool openConnection();
    // Close + remove the QSQLITE connection bound to m_connectionName, if any.
    void dropConnection();
    // Quick yes/no verdict from PRAGMA integrity_check(1) on the current connection (torn-write /
    // corruption gate; IDBFS can persist an interrupted db/journal pair).
    [[nodiscard]] bool integrityCheckOk() const;
    // Delete a db file and its SQLite sidecars (-journal / -wal / -shm).
    static void removeDatabaseFiles(const QString& path);
    // Create the meta table, reconcile the persisted schema version (rebuilding the
    // non-authoritative cache on a version mismatch), then create the data tables.
    bool ensureSchema();
    bool createDataTables();
    bool dropDataTables();
    bool execSql(const char* sql);
    // Run a prepared mutating query. On success arms the debounced IDBFS write-back (no-op off
    // wasm) so the change reaches IndexedDB; on failure records lastError().
    bool execWrite(QSqlQuery& query);
    void setLastError(const QString& message) const;

    QString m_dbPath;
    QString m_basePath; // the shared/default path; per-user paths derive from its directory
    QString m_connectionName;
    mutable QString m_lastError;
};

} // namespace daemonapp::daemon
