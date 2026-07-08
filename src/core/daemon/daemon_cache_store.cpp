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
    // daemon_session_log + daemon_approvals are retired (Phase 4 closeout): the transcript caches
    // in daemon_transcript_blocks and pending approvals are transient/live. dropDataTables still
    // drops them so a stale v2 DB is cleaned on the version bump.
    return execSql(cache::kCreateSessionsSql) && execSql(cache::kCreateSyncCursorsSql) &&
           execSql(cache::kCreateProfilesSql) && execSql(cache::kCreateFsEntriesSql) &&
           execSql(cache::kCreateTranscriptBlocksSql) && execSql(cache::kCreateFleetUnitsSql) &&
           execSql(cache::kCreateTransportInstancesSql) && execSql(cache::kCreateConversationsSql);
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
    if (!q.exec(QStringLiteral("SELECT (SELECT COUNT(*) FROM daemon_sessions)"
                               " + (SELECT COUNT(*) FROM daemon_sync_cursors)"
                               " + (SELECT COUNT(*) FROM daemon_profiles)"
                               " + (SELECT COUNT(*) FROM daemon_fs_entries)"
                               " + (SELECT COUNT(*) FROM daemon_transcript_blocks)"
                               " + (SELECT COUNT(*) FROM daemon_fleet_units)"
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

bool DaemonCacheStore::upsertSession(const CachedSessionRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_sessions(session_id,title,state,profile_ref,lifecycle,role,"
        "parent_session_id,pinned,archived,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(session_id) DO UPDATE SET "
        "title=excluded.title,state=excluded.state,profile_ref=excluded.profile_ref,"
        "lifecycle=excluded.lifecycle,role=excluded.role,parent_session_id=excluded.parent_session_"
        "id,"
        "pinned=excluded.pinned,archived=excluded.archived,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.sessionId);
    q.addBindValue(row.title);
    q.addBindValue(row.state);
    q.addBindValue(row.profileRef);
    q.addBindValue(row.lifecycle);
    q.addBindValue(row.role);
    q.addBindValue(row.parentSessionId);
    q.addBindValue(row.pinned ? 1 : 0);
    q.addBindValue(row.archived ? 1 : 0);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedSessionRow> DaemonCacheStore::sessions() const {
    QList<CachedSessionRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral(
            "SELECT session_id,title,state,profile_ref,lifecycle,role,parent_session_id,"
            "pinned,archived,updated_at_ms FROM daemon_sessions ORDER BY updated_at_ms DESC"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedSessionRow row;
        row.sessionId = q.value(0).toString();
        row.title = q.value(1).toString();
        row.state = q.value(2).toString();
        row.profileRef = q.value(3).toString();
        row.lifecycle = q.value(4).toString();
        row.role = q.value(5).toString();
        row.parentSessionId = q.value(6).toString();
        row.pinned = q.value(7).toInt() != 0;
        row.archived = q.value(8).toInt() != 0;
        row.updatedAtMs = q.value(9).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::deleteSession(const QString& sessionId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_sessions WHERE session_id=?"));
    q.addBindValue(sessionId);
    return execWrite(q);
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

bool DaemonCacheStore::upsertFleetUnit(const CachedFleetUnitRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // [wave2:app-delegation] v7 (F3): +lifetime,engine_kind,engine_agent.
    // [waveB:app-v30] v8 (stretch): +end_reason.
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_fleet_units(unit_id,parent_id,depth,ordinal,name,kind,state,role,"
        "profile_ref,session_id,work,lifetime,engine_kind,engine_agent,end_reason,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(unit_id) DO UPDATE SET parent_id=excluded.parent_id,depth=excluded.depth,"
        "ordinal=excluded.ordinal,name=excluded.name,kind=excluded.kind,state=excluded.state,"
        "role=excluded.role,profile_ref=excluded.profile_ref,session_id=excluded.session_id,"
        "work=excluded.work,lifetime=excluded.lifetime,engine_kind=excluded.engine_kind,"
        "engine_agent=excluded.engine_agent,end_reason=excluded.end_reason,"
        "updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.unitId);
    q.addBindValue(row.parentId);
    q.addBindValue(row.depth);
    q.addBindValue(row.ordinal);
    q.addBindValue(row.name);
    q.addBindValue(row.kind);
    q.addBindValue(row.state);
    q.addBindValue(row.role);
    q.addBindValue(row.profileRef);
    q.addBindValue(row.sessionId);
    q.addBindValue(row.work);
    q.addBindValue(row.lifetime);
    q.addBindValue(row.engineKind);
    q.addBindValue(row.engineAgent);
    q.addBindValue(row.endReason);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedFleetUnitRow> DaemonCacheStore::fleetUnits() const {
    QList<CachedFleetUnitRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // [wave2:app-delegation] v7 (F3): +lifetime,engine_kind,engine_agent.
    // [waveB:app-v30] v8 (stretch): +end_reason.
    if (!q.exec(QStringLiteral(
            "SELECT unit_id,parent_id,depth,ordinal,name,kind,state,role,profile_ref,session_id,"
            "work,lifetime,engine_kind,engine_agent,end_reason,updated_at_ms FROM "
            "daemon_fleet_units "
            "ORDER BY ordinal ASC"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedFleetUnitRow row;
        row.unitId = q.value(0).toString();
        row.parentId = q.value(1).toString();
        row.depth = q.value(2).toInt();
        row.ordinal = q.value(3).toInt();
        row.name = q.value(4).toString();
        row.kind = q.value(5).toString();
        row.state = q.value(6).toString();
        row.role = q.value(7).toString();
        row.profileRef = q.value(8).toString();
        row.sessionId = q.value(9).toString();
        row.work = q.value(10).toString();
        row.lifetime = q.value(11).toString();
        row.engineKind = q.value(12).toString();
        row.engineAgent = q.value(13).toString();
        row.endReason = q.value(14).toString();
        row.updatedAtMs = q.value(15).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::deleteFleetUnit(const QString& unitId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_fleet_units WHERE unit_id=?"));
    q.addBindValue(unitId);
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
        "INSERT INTO daemon_conversations(transport,conv_id,kind,title,topic,updated_at_ms) "
        "VALUES(?,?,?,?,?,?) "
        "ON CONFLICT(transport,conv_id) DO UPDATE SET kind=excluded.kind,title=excluded.title,"
        "topic=excluded.topic,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.transport);
    q.addBindValue(row.convId);
    q.addBindValue(row.kind);
    q.addBindValue(row.title);
    q.addBindValue(row.topic);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedConversationRow> DaemonCacheStore::conversations(const QString& transport) const {
    QList<CachedConversationRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "SELECT transport,conv_id,kind,title,topic,updated_at_ms FROM daemon_conversations "
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
        row.updatedAtMs = q.value(5).toLongLong();
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

bool DaemonCacheStore::upsertTranscriptBlock(const CachedTranscriptBlockRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_transcript_blocks(session_id,seq,kind,role,text,call_id,tool_name,"
        "args_summary,ok,summary,detail_kind,detail_body,request_id,host_kind,content_kind,"
        "updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(session_id,seq) DO UPDATE SET kind=excluded.kind,role=excluded.role,"
        "text=excluded.text,call_id=excluded.call_id,tool_name=excluded.tool_name,"
        "args_summary=excluded.args_summary,ok=excluded.ok,summary=excluded.summary,"
        "detail_kind=excluded.detail_kind,detail_body=excluded.detail_body,"
        "request_id=excluded.request_id,host_kind=excluded.host_kind,"
        "content_kind=excluded.content_kind,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.sessionId);
    q.addBindValue(QVariant::fromValue<qulonglong>(row.seq));
    q.addBindValue(row.kind);
    q.addBindValue(row.role);
    q.addBindValue(row.text);
    q.addBindValue(row.callId);
    q.addBindValue(row.toolName);
    q.addBindValue(row.argsSummary);
    q.addBindValue(row.ok ? 1 : 0);
    q.addBindValue(row.summary);
    q.addBindValue(row.detailKind);
    q.addBindValue(row.detailBody);
    q.addBindValue(row.requestId);
    q.addBindValue(row.hostKind);
    q.addBindValue(row.contentKind);
    q.addBindValue(row.updatedAtMs);
    return execWrite(q);
}

QList<CachedTranscriptBlockRow> DaemonCacheStore::transcriptBlocks(const QString& sessionId,
                                                                   quint64 afterSeq) const {
    QList<CachedTranscriptBlockRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "SELECT session_id,seq,kind,role,text,call_id,tool_name,args_summary,ok,summary,"
        "detail_kind,detail_body,request_id,host_kind,content_kind,updated_at_ms "
        "FROM daemon_transcript_blocks WHERE session_id=? AND seq>? ORDER BY seq ASC"));
    q.addBindValue(sessionId);
    q.addBindValue(QVariant::fromValue<qulonglong>(afterSeq));
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedTranscriptBlockRow row;
        row.sessionId = q.value(0).toString();
        row.seq = q.value(1).toULongLong();
        row.kind = q.value(2).toString();
        row.role = q.value(3).toString();
        row.text = q.value(4).toString();
        row.callId = q.value(5).toString();
        row.toolName = q.value(6).toString();
        row.argsSummary = q.value(7).toString();
        row.ok = q.value(8).toInt() != 0;
        row.summary = q.value(9).toString();
        row.detailKind = q.value(10).toString();
        row.detailBody = q.value(11).toByteArray();
        row.requestId = q.value(12).toUInt();
        row.hostKind = q.value(13).toString();
        row.contentKind = q.value(14).toString();
        row.updatedAtMs = q.value(15).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::clearTranscript(const QString& sessionId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_transcript_blocks WHERE session_id=?"));
    q.addBindValue(sessionId);
    return execWrite(q);
}

QHash<QString, QString> DaemonCacheStore::latestTranscriptSnippets() const {
    QHash<QString, QString> out;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // GROUP BY with MAX(seq) picks the highest-seq row per session (SQLite bare-column rule), so
    // `text` is the latest message's text - one query for the whole roster.
    if (!q.exec(QStringLiteral("SELECT session_id,text,MAX(seq) FROM daemon_transcript_blocks "
                               "WHERE kind='Message' AND text IS NOT NULL AND text<>'' "
                               "GROUP BY session_id"))) {
        setLastError(q.lastError().text());
        return out;
    }
    while (q.next()) {
        out.insert(q.value(0).toString(), q.value(1).toString());
    }
    return out;
}

} // namespace daemonapp::daemon
