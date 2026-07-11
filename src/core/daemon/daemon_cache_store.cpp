// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_cache_store.h"

#include "daemon/client_cache_schema.h"
#include "platform/wasm_persistence.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

namespace daemonapp::daemon {

DaemonCacheStore::DaemonCacheStore(const QString& dbPath, QObject* parent)
    : QObject(parent), m_basePath(dbPath.isEmpty() ? defaultDatabasePath() : dbPath),
      m_connectionName(QStringLiteral("daemon-cache-%1").arg(reinterpret_cast<quintptr>(this))) {
    openAt(m_basePath);
    // Register the browser unload/visibility write-back once (no-op off wasm): the debounced
    // FS.syncfs could otherwise lose the last cache mutations when the tab is backgrounded/closed.
    platform::installUnloadFlush();
}

void DaemonCacheStore::openAt(const QString& path) {
    dropConnection();
    m_dbPath = path;
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    if (openConnection()) {
        return;
    }
    // The db failed to open, flunked the integrity gate, or its schema could not be built. On wasm
    // IDBFS snapshots each file independently, so an interrupted syncfs can persist a torn
    // db/journal pair; on any platform a crash mid-write can corrupt the file. The cache is
    // non-authoritative (the daemon re-baselines it on reconnect), so recovery is simply: discard
    // the file(s) and start fresh. This branch is only reached on failure, so it is harmless on a
    // healthy db.
    qWarning() << "daemon-cache: unusable database at" << m_dbPath << "(" << m_lastError
               << "); discarding and rebuilding - the non-authoritative cache re-baselines from "
                  "the daemon on reconnect";
    dropConnection();
    removeDatabaseFiles(m_dbPath);
    if (!openConnection()) {
        qWarning() << "daemon-cache: rebuild after reset still failed:" << m_lastError;
    }
}

bool DaemonCacheStore::openConnection() {
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        setLastError(db.lastError().text());
        return false;
    }
    if (!integrityCheckOk()) {
        setLastError(QStringLiteral("PRAGMA integrity_check failed"));
        db.close();
        return false;
    }
    if (!ensureSchema()) {
        // m_lastError already carries the failing statement's error; close so isOpen() reports the
        // store as unusable rather than half-initialized.
        db.close();
        return false;
    }
    return true;
}

void DaemonCacheStore::dropConnection() {
    {
        QSqlDatabase existing = QSqlDatabase::database(m_connectionName, false);
        if (existing.isValid()) {
            existing.close();
        }
    }
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DaemonCacheStore::integrityCheckOk() const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // integrity_check(1): stop after the first problem - we only need a yes/no verdict on the boot
    // path, not a full scan of a large cache. A healthy db returns a single "ok" row.
    if (!q.exec(QStringLiteral("PRAGMA integrity_check(1)"))) {
        return false;
    }
    if (!q.next()) {
        return false;
    }
    return q.value(0).toString().compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0;
}

void DaemonCacheStore::removeDatabaseFiles(const QString& path) {
    // Remove the db and any SQLite sidecars (rollback journal / WAL / shared-memory) so a torn set
    // cannot resurrect the corruption on the next open.
    QFile::remove(path);
    for (const char* suffix : {"-journal", "-wal", "-shm"}) {
        QFile::remove(path + QString::fromLatin1(suffix));
    }
}

QString DaemonCacheStore::namespacedPath(const QString& userKey) const {
    if (userKey.isEmpty()) {
        return m_basePath; // shared/default db (pre-auth / local-trust)
    }
    const QByteArray h =
        QCryptographicHash::hash(userKey.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    const QFileInfo base(m_basePath);
    return base.dir().filePath(QStringLiteral("daemon_cache-%1.db").arg(QString::fromLatin1(h)));
}

void DaemonCacheStore::setUserNamespace(const QString& userKey) {
    const QString target = namespacedPath(userKey);
    if (target == m_dbPath && isOpen()) {
        return; // already on this namespace
    }
    openAt(target);
}

DaemonCacheStore::~DaemonCacheStore() {
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DaemonCacheStore::isOpen() const {
    const QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    return db.isValid() && db.isOpen();
}

QString DaemonCacheStore::defaultDatabasePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("daemon_cache.db"));
}

bool DaemonCacheStore::ensureSchema() {
    if (!execSql(cache::kCreateMetaSql)) {
        return false;
    }
    const QString stored = meta(QStringLiteral("schema_version"));
    if (!stored.isEmpty() && stored.toInt() != cache::kSchemaVersion) {
        // daemon_cache.db is a non-authoritative last-known cache; the daemon is the source of
        // truth, so a schema bump just drops and rebuilds the data tables rather than running
        // per-version migrations. The trigger is purely the internal `cache::kSchemaVersion`
        // integer (hand-bumped when the cache DDL changes) — it is intentionally decoupled from the
        // app release version and the wire/server version (a wire mismatch fails the handshake; the
        // cache re-baselines from the daemon on reconnect). Log the wipe so schema churn is
        // visible.
        qInfo() << "daemon-cache: schema_version" << stored << "->" << cache::kSchemaVersion
                << "- dropping and rebuilding the (non-authoritative) cache; it re-baselines from "
                   "the daemon on reconnect";
        if (!dropDataTables()) {
            return false;
        }
    }
    if (!createDataTables()) {
        return false;
    }
    return setMeta(QStringLiteral("schema_version"), QString::number(cache::kSchemaVersion));
}

bool DaemonCacheStore::createDataTables() {
    // v11 (AD): daemon_sessions / daemon_fleet_units / daemon_transcript_blocks are retired —
    // those domains live on the mirror (mirror-<id>.db). dropDataTables still drops them (and
    // the older retired tables) so a stale DB is cleaned on the version bump.
    return execSql(cache::kCreateSyncCursorsSql) && execSql(cache::kCreateProfilesSql) &&
           execSql(cache::kCreateFsEntriesSql) && execSql(cache::kCreateTransportInstancesSql) &&
           execSql(cache::kCreateConversationsSql);
}

bool DaemonCacheStore::dropDataTables() {
    return execSql("DROP TABLE IF EXISTS daemon_sessions") &&
           execSql("DROP TABLE IF EXISTS daemon_session_log") &&
           execSql("DROP TABLE IF EXISTS daemon_sync_cursors") &&
           execSql("DROP TABLE IF EXISTS daemon_profiles") &&
           execSql("DROP TABLE IF EXISTS daemon_approvals") &&
           execSql("DROP TABLE IF EXISTS daemon_fs_entries") &&
           execSql("DROP TABLE IF EXISTS daemon_transcript_blocks") &&
           execSql("DROP TABLE IF EXISTS daemon_fleet_units") &&
           execSql("DROP TABLE IF EXISTS daemon_transport_instances") &&
           execSql("DROP TABLE IF EXISTS daemon_conversations");
}

int DaemonCacheStore::schemaVersion() const {
    return meta(QStringLiteral("schema_version")).toInt();
}

bool DaemonCacheStore::setMeta(const QString& key, const QString& value) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("INSERT INTO daemon_cache_meta(key,value) VALUES(?,?) "
                             "ON CONFLICT(key) DO UPDATE SET value=excluded.value"));
    q.addBindValue(key);
    q.addBindValue(value);
    return execWrite(q);
}

QString DaemonCacheStore::meta(const QString& key) const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT value FROM daemon_cache_meta WHERE key=?"));
    q.addBindValue(key);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return {};
    }
    return q.next() ? q.value(0).toString() : QString();
}

bool DaemonCacheStore::clearAll() {
    // Empty the currently-open db (drop + recreate the data tables + restamp schema_version) so it
    // stays a valid, empty cache that re-baselines from the daemon on the next ready.
    bool ok = true;
    if (isOpen()) {
        ok = dropDataTables() && createDataTables() &&
             setMeta(QStringLiteral("schema_version"), QString::number(cache::kSchemaVersion));
    }
    // Then delete every sibling cache file for OTHER user namespaces (daemon_cache*.db* in the same
    // directory) so "Clear local data" leaves no prior user's roster/transcripts on the device. The
    // current db and its sidecars are kept: it is open and already emptied above.
    const QDir dir = QFileInfo(m_dbPath).dir();
    const QStringList siblings = dir.entryList(
        {QStringLiteral("daemon_cache*.db"), QStringLiteral("daemon_cache*.db-*")}, QDir::Files);
    for (const QString& name : siblings) {
        const QString abs = dir.absoluteFilePath(name);
        if (abs == m_dbPath || abs.startsWith(m_dbPath)) {
            continue; // the open, freshly-emptied db (+ its -journal / -wal / -shm sidecars)
        }
        QFile::remove(abs);
    }
    // Best-effort write-back of the emptied mount (no-op off wasm).
    platform::scheduleIdbfsSync();
    return ok;
}

int DaemonCacheStore::approxRowCount() const {
    if (!isOpen()) {
        return 0;
    }
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // One round-trip sum across the cached data tables (daemon_cache_meta is bookkeeping,
    // excluded). Best-effort: any error yields 0. Feeds the boot cache-rows sentinel, which proves
    // the IDBFS mount survived a reload with content before any network fetch.
    if (!q.exec(QStringLiteral("SELECT (SELECT COUNT(*) FROM daemon_sync_cursors)"
                               " + (SELECT COUNT(*) FROM daemon_profiles)"
                               " + (SELECT COUNT(*) FROM daemon_fs_entries)"
                               " + (SELECT COUNT(*) FROM daemon_transport_instances)"
                               " + (SELECT COUNT(*) FROM daemon_conversations)"))) {
        setLastError(q.lastError().text());
        return 0;
    }
    return q.next() ? q.value(0).toInt() : 0;
}

bool DaemonCacheStore::execSql(const char* sql) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QString::fromUtf8(sql))) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

bool DaemonCacheStore::execWrite(QSqlQuery& query) {
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return false;
    }
    // A successful mutation dirties the on-disk db; on wasm this arms the debounced FS.syncfs that
    // writes the IDBFS mount back to IndexedDB (no-op off wasm). Coalesced, so per-write is cheap.
    platform::scheduleIdbfsSync();
    return true;
}

void DaemonCacheStore::setLastError(const QString& message) const {
    m_lastError = message;
}

bool DaemonCacheStore::setCursor(const QString& scope, const QString& cursor, qint64 updatedAtMs) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(
        QStringLiteral("INSERT INTO daemon_sync_cursors(scope,cursor,updated_at_ms) VALUES(?,?,?) "
                       "ON CONFLICT(scope) DO UPDATE SET "
                       "cursor=excluded.cursor,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(scope);
    q.addBindValue(cursor);
    q.addBindValue(updatedAtMs);
    return execWrite(q);
}

QString DaemonCacheStore::cursor(const QString& scope) const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT cursor FROM daemon_sync_cursors WHERE scope=?"));
    q.addBindValue(scope);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return {};
    }
    return q.next() ? q.value(0).toString() : QString();
}

bool DaemonCacheStore::upsertFsEntry(const CachedFsEntryRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO "
        "daemon_fs_entries(root_id,path,kind,size,mtime_ms,revision_cbor,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?) "
        "ON CONFLICT(root_id,path) DO UPDATE SET kind=excluded.kind,size=excluded.size,"
        "mtime_ms=excluded.mtime_ms,revision_cbor=excluded.revision_cbor,"
        "updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.rootId);
    q.addBindValue(row.path);
    q.addBindValue(row.kind);
    q.addBindValue(QVariant::fromValue<qulonglong>(row.size));
    q.addBindValue(row.mtimeMs);
    q.addBindValue(row.revisionCbor);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedFsEntryRow> DaemonCacheStore::fsEntries(const QString& rootId) const {
    QList<CachedFsEntryRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT root_id,path,kind,size,mtime_ms,revision_cbor,updated_at_ms "
                             "FROM daemon_fs_entries WHERE root_id=? ORDER BY path ASC"));
    q.addBindValue(rootId);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedFsEntryRow row;
        row.rootId = q.value(0).toString();
        row.path = q.value(1).toString();
        row.kind = q.value(2).toString();
        row.size = q.value(3).toULongLong();
        row.mtimeMs = q.value(4).toLongLong();
        row.revisionCbor = q.value(5).toByteArray();
        row.updatedAtMs = q.value(6).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::upsertProfile(const CachedProfileRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_profiles(profile_ref,display_name,spec_cbor,active,updated_at_ms) "
        "VALUES(?,?,?,?,?) "
        "ON CONFLICT(profile_ref) DO UPDATE SET display_name=excluded.display_name,"
        "spec_cbor=excluded.spec_cbor,active=excluded.active,updated_at_ms=excluded.updated_at_"
        "ms"));
    q.addBindValue(row.profileRef);
    q.addBindValue(row.displayName);
    q.addBindValue(row.specCbor);
    q.addBindValue(row.active ? 1 : 0);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedProfileRow> DaemonCacheStore::profiles() const {
    QList<CachedProfileRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral(
            "SELECT profile_ref,display_name,spec_cbor,active,updated_at_ms FROM daemon_profiles "
            "ORDER BY profile_ref ASC"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedProfileRow row;
        row.profileRef = q.value(0).toString();
        row.displayName = q.value(1).toString();
        row.specCbor = q.value(2).toByteArray();
        row.active = q.value(3).toInt() != 0;
        row.updatedAtMs = q.value(4).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::deleteProfile(const QString& profileRef) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_profiles WHERE profile_ref=?"));
    q.addBindValue(profileRef);
    return execWrite(q);
}

bool DaemonCacheStore::upsertTransportInstance(const CachedTransportInstanceRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // [waveB:app-v30] v8 (D1): +connection_reason,connection_message,fatal.
    // [acct-mgmt] v9 (wire v35): +enabled,label.
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_transport_instances(transport,family,display_name,connection,presence,"
        "bound_profile,connection_reason,connection_message,fatal,enabled,label,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(transport) DO UPDATE SET family=excluded.family,"
        "display_name=excluded.display_name,connection=excluded.connection,"
        "presence=excluded.presence,bound_profile=excluded.bound_profile,"
        "connection_reason=excluded.connection_reason,"
        "connection_message=excluded.connection_message,fatal=excluded.fatal,"
        "enabled=excluded.enabled,label=excluded.label,"
        "updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.transport);
    q.addBindValue(row.family);
    q.addBindValue(row.displayName);
    q.addBindValue(row.connection);
    q.addBindValue(row.presence);
    q.addBindValue(row.boundProfile);
    q.addBindValue(row.connectionReason);
    q.addBindValue(row.connectionMessage);
    q.addBindValue(row.fatal ? 1 : 0);
    q.addBindValue(row.enabled ? 1 : 0);
    q.addBindValue(row.label);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedTransportInstanceRow> DaemonCacheStore::transportInstances() const {
    QList<CachedTransportInstanceRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // [waveB:app-v30] v8 (D1): +connection_reason,connection_message,fatal.
    // [acct-mgmt] v9 (wire v35): +enabled,label.
    if (!q.exec(QStringLiteral(
            "SELECT transport,family,display_name,connection,presence,bound_profile,"
            "connection_reason,connection_message,fatal,enabled,label,updated_at_ms "
            "FROM daemon_transport_instances ORDER BY family ASC, display_name ASC"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedTransportInstanceRow row;
        row.transport = q.value(0).toString();
        row.family = q.value(1).toString();
        row.displayName = q.value(2).toString();
        row.connection = q.value(3).toString();
        row.presence = q.value(4).toString();
        row.boundProfile = q.value(5).toString();
        row.connectionReason = q.value(6).toString();
        row.connectionMessage = q.value(7).toString();
        row.fatal = q.value(8).toInt() != 0;
        row.enabled = q.value(9).toInt() != 0;
        row.label = q.value(10).toString();
        row.updatedAtMs = q.value(11).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::deleteTransportInstance(const QString& transport) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_transport_instances WHERE transport=?"));
    q.addBindValue(transport);
    return execWrite(q);
}

bool DaemonCacheStore::upsertConversation(const CachedConversationRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_conversations(transport,conv_id,kind,title,topic,parent,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?) "
        "ON CONFLICT(transport,conv_id) DO UPDATE SET kind=excluded.kind,title=excluded.title,"
        "topic=excluded.topic,parent=excluded.parent,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.transport);
    q.addBindValue(row.convId);
    q.addBindValue(row.kind);
    q.addBindValue(row.title);
    q.addBindValue(row.topic);
    q.addBindValue(row.parent);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedConversationRow> DaemonCacheStore::conversations(const QString& transport) const {
    QList<CachedConversationRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "SELECT transport,conv_id,kind,title,topic,parent,updated_at_ms FROM daemon_conversations "
        "WHERE transport=? ORDER BY title ASC, conv_id ASC"));
    q.addBindValue(transport);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedConversationRow row;
        row.transport = q.value(0).toString();
        row.convId = q.value(1).toString();
        row.kind = q.value(2).toString();
        row.title = q.value(3).toString();
        row.topic = q.value(4).toString();
        row.parent = q.value(5).toString();
        row.updatedAtMs = q.value(6).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::deleteConversation(const QString& transport, const QString& convId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_conversations WHERE transport=? AND conv_id=?"));
    q.addBindValue(transport);
    q.addBindValue(convId);
    return execWrite(q);
}

} // namespace daemonapp::daemon
