#pragma once

#include <QByteArray>
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

struct CachedApprovalRow {
    QString sessionId;
    QString requestId;
    QString prompt;
    QString path;
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

    bool appendSessionLog(const CachedLogRow& row);
    [[nodiscard]] QList<CachedLogRow> sessionLog(const QString& sessionId, quint64 afterSeq = 0,
                                                 int limit = 0) const;

    bool setCursor(const QString& scope, const QString& cursor, qint64 updatedAtMs);
    [[nodiscard]] QString cursor(const QString& scope) const;

    bool upsertApproval(const CachedApprovalRow& row);
    [[nodiscard]] QList<CachedApprovalRow> approvals() const;

    bool upsertFsEntry(const CachedFsEntryRow& row);
    [[nodiscard]] QList<CachedFsEntryRow> fsEntries(const QString& rootId) const;

    bool upsertProfile(const CachedProfileRow& row);
    [[nodiscard]] QList<CachedProfileRow> profiles() const;

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
