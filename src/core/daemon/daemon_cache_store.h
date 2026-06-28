#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

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

private:
    [[nodiscard]] static QString defaultDatabasePath();
    // Create the meta table, reconcile the persisted schema version (rebuilding the
    // non-authoritative cache on a version mismatch), then create the data tables.
    bool ensureSchema();
    bool createDataTables();
    bool dropDataTables();
    bool execSql(const char* sql);
    void setLastError(const QString& message) const;

    QString m_dbPath;
    QString m_connectionName;
    mutable QString m_lastError;
};

} // namespace daemonapp::daemon
