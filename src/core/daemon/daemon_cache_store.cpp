#include "daemon/daemon_cache_store.h"

#include "daemon/client_cache_schema.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace daemonapp::daemon {

DaemonCacheStore::DaemonCacheStore(const QString& dbPath, QObject* parent)
    : QObject(parent), m_dbPath(dbPath.isEmpty() ? defaultDatabasePath() : dbPath),
      m_connectionName(QStringLiteral("daemon-cache-%1").arg(reinterpret_cast<quintptr>(this))) {
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        setLastError(db.lastError().text());
        return;
    }
    if (!ensureSchema()) {
        // m_lastError already carries the failing statement's error; close so isOpen()
        // reports the store as unusable rather than half-initialized.
        db.close();
    }
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
        // daemon_cache.db is a non-authoritative last-known cache; the daemon is the
        // source of truth, so a schema bump just drops and rebuilds the data tables
        // rather than running per-version migrations.
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
           execSql(cache::kCreateTranscriptBlocksSql) && execSql(cache::kCreateFleetUnitsSql);
}

bool DaemonCacheStore::dropDataTables() {
    return execSql("DROP TABLE IF EXISTS daemon_sessions") &&
           execSql("DROP TABLE IF EXISTS daemon_session_log") &&
           execSql("DROP TABLE IF EXISTS daemon_sync_cursors") &&
           execSql("DROP TABLE IF EXISTS daemon_profiles") &&
           execSql("DROP TABLE IF EXISTS daemon_approvals") &&
           execSql("DROP TABLE IF EXISTS daemon_fs_entries") &&
           execSql("DROP TABLE IF EXISTS daemon_transcript_blocks") &&
           execSql("DROP TABLE IF EXISTS daemon_fleet_units");
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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

bool DaemonCacheStore::execSql(const char* sql) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QString::fromUtf8(sql))) {
        setLastError(q.lastError().text());
        return false;
    }
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

bool DaemonCacheStore::upsertFleetUnit(const CachedFleetUnitRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_fleet_units(unit_id,parent_id,depth,ordinal,name,kind,state,role,"
        "profile_ref,session_id,work,updated_at_ms) VALUES(?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(unit_id) DO UPDATE SET parent_id=excluded.parent_id,depth=excluded.depth,"
        "ordinal=excluded.ordinal,name=excluded.name,kind=excluded.kind,state=excluded.state,"
        "role=excluded.role,profile_ref=excluded.profile_ref,session_id=excluded.session_id,"
        "work=excluded.work,updated_at_ms=excluded.updated_at_ms"));
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
    q.addBindValue(row.updatedAtMs);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

QList<CachedFleetUnitRow> DaemonCacheStore::fleetUnits() const {
    QList<CachedFleetUnitRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral(
            "SELECT unit_id,parent_id,depth,ordinal,name,kind,state,role,profile_ref,session_id,"
            "work,updated_at_ms FROM daemon_fleet_units ORDER BY ordinal ASC"))) {
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
        row.updatedAtMs = q.value(11).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::deleteFleetUnit(const QString& unitId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_fleet_units WHERE unit_id=?"));
    q.addBindValue(unitId);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

bool DaemonCacheStore::upsertTranscriptBlock(const CachedTranscriptBlockRow& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_transcript_blocks(session_id,seq,kind,role,text,call_id,tool_name,"
        "args_summary,ok,summary,request_id,host_kind,content_kind,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(session_id,seq) DO UPDATE SET kind=excluded.kind,role=excluded.role,"
        "text=excluded.text,call_id=excluded.call_id,tool_name=excluded.tool_name,"
        "args_summary=excluded.args_summary,ok=excluded.ok,summary=excluded.summary,"
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
    q.addBindValue(row.requestId);
    q.addBindValue(row.hostKind);
    q.addBindValue(row.contentKind);
    q.addBindValue(row.updatedAtMs);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

QList<CachedTranscriptBlockRow> DaemonCacheStore::transcriptBlocks(const QString& sessionId,
                                                                   quint64 afterSeq) const {
    QList<CachedTranscriptBlockRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "SELECT session_id,seq,kind,role,text,call_id,tool_name,args_summary,ok,summary,"
        "request_id,host_kind,content_kind,updated_at_ms FROM daemon_transcript_blocks "
        "WHERE session_id=? AND seq>? ORDER BY seq ASC"));
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
        row.requestId = q.value(10).toUInt();
        row.hostKind = q.value(11).toString();
        row.contentKind = q.value(12).toString();
        row.updatedAtMs = q.value(13).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::clearTranscript(const QString& sessionId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM daemon_transcript_blocks WHERE session_id=?"));
    q.addBindValue(sessionId);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
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
