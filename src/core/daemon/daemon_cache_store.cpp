#include "daemon/daemon_cache_store.h"

#include "daemon/client_cache_schema.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace daemonapp::daemon {
namespace {

qint64 nowMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

} // namespace

DaemonCacheStore::DaemonCacheStore(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath.isEmpty() ? defaultDatabasePath() : dbPath)
    , m_connectionName(QStringLiteral("daemon-cache-%1").arg(reinterpret_cast<quintptr>(this)))
{
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        setLastError(db.lastError().text());
        return;
    }
    execSchema();
}

DaemonCacheStore::~DaemonCacheStore()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool DaemonCacheStore::isOpen() const
{
    const QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    return db.isValid() && db.isOpen();
}

QString DaemonCacheStore::defaultDatabasePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("daemon_cache.db"));
}

bool DaemonCacheStore::execSchema()
{
    return execSql(cache::kCreateMetaSql)
        && execSql(cache::kCreateSessionsSql)
        && execSql(cache::kCreateSessionLogSql)
        && execSql(cache::kCreateSyncCursorsSql)
        && execSql(cache::kCreateProfilesSql)
        && execSql(cache::kCreateApprovalsSql)
        && execSql(cache::kCreateFsEntriesSql)
        && setCursor(QStringLiteral("schema_version"), QString::number(cache::kSchemaVersion), nowMs());
}

bool DaemonCacheStore::execSql(const char* sql)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QString::fromUtf8(sql))) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

void DaemonCacheStore::setLastError(const QString& message) const
{
    m_lastError = message;
}

bool DaemonCacheStore::upsertSession(const CachedSessionRow& row)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_sessions(session_id,title,state,profile_ref,lifecycle,role,"
        "parent_session_id,pinned,archived,updated_at_ms) "
        "VALUES(?,?,?,?,?,?,?,?,?,?) "
        "ON CONFLICT(session_id) DO UPDATE SET "
        "title=excluded.title,state=excluded.state,profile_ref=excluded.profile_ref,"
        "lifecycle=excluded.lifecycle,role=excluded.role,parent_session_id=excluded.parent_session_id,"
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

QList<CachedSessionRow> DaemonCacheStore::sessions() const
{
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

bool DaemonCacheStore::appendSessionLog(const CachedLogRow& row)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_session_log(session_id,seq,payload_cbor,direction,disposition,updated_at_ms) "
        "VALUES(?,?,?,?,?,?) "
        "ON CONFLICT(session_id,seq) DO UPDATE SET "
        "payload_cbor=excluded.payload_cbor,direction=excluded.direction,"
        "disposition=excluded.disposition,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.sessionId);
    q.addBindValue(QVariant::fromValue<qulonglong>(row.seq));
    q.addBindValue(row.payloadCbor);
    q.addBindValue(row.direction);
    q.addBindValue(row.disposition);
    q.addBindValue(row.updatedAtMs);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

QList<CachedLogRow> DaemonCacheStore::sessionLog(const QString& sessionId, quint64 afterSeq,
                                                int limit) const
{
    QList<CachedLogRow> rows;
    QString sql = QStringLiteral(
        "SELECT session_id,seq,payload_cbor,direction,disposition,updated_at_ms "
        "FROM daemon_session_log WHERE session_id=? AND seq>? ORDER BY seq ASC");
    if (limit > 0) {
        sql += QStringLiteral(" LIMIT ?");
    }
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(sql);
    q.addBindValue(sessionId);
    q.addBindValue(QVariant::fromValue<qulonglong>(afterSeq));
    if (limit > 0) {
        q.addBindValue(limit);
    }
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedLogRow row;
        row.sessionId = q.value(0).toString();
        row.seq = q.value(1).toULongLong();
        row.payloadCbor = q.value(2).toByteArray();
        row.direction = q.value(3).toString();
        row.disposition = q.value(4).toString();
        row.updatedAtMs = q.value(5).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::setCursor(const QString& scope, const QString& cursor, qint64 updatedAtMs)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_sync_cursors(scope,cursor,updated_at_ms) VALUES(?,?,?) "
        "ON CONFLICT(scope) DO UPDATE SET cursor=excluded.cursor,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(scope);
    q.addBindValue(cursor);
    q.addBindValue(updatedAtMs);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

QString DaemonCacheStore::cursor(const QString& scope) const
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT cursor FROM daemon_sync_cursors WHERE scope=?"));
    q.addBindValue(scope);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return {};
    }
    return q.next() ? q.value(0).toString() : QString();
}

bool DaemonCacheStore::upsertApproval(const CachedApprovalRow& row)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_approvals(session_id,request_id,prompt,path,updated_at_ms) "
        "VALUES(?,?,?,?,?) "
        "ON CONFLICT(session_id,request_id) DO UPDATE SET "
        "prompt=excluded.prompt,path=excluded.path,updated_at_ms=excluded.updated_at_ms"));
    q.addBindValue(row.sessionId);
    q.addBindValue(row.requestId);
    q.addBindValue(row.prompt);
    q.addBindValue(row.path);
    q.addBindValue(row.updatedAtMs);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

QList<CachedApprovalRow> DaemonCacheStore::approvals() const
{
    QList<CachedApprovalRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral(
            "SELECT session_id,request_id,prompt,path,updated_at_ms FROM daemon_approvals "
            "ORDER BY updated_at_ms DESC"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        CachedApprovalRow row;
        row.sessionId = q.value(0).toString();
        row.requestId = q.value(1).toString();
        row.prompt = q.value(2).toString();
        row.path = q.value(3).toString();
        row.updatedAtMs = q.value(4).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool DaemonCacheStore::upsertFsEntry(const CachedFsEntryRow& row)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO daemon_fs_entries(root_id,path,kind,size,mtime_ms,revision_cbor,updated_at_ms) "
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

QList<CachedFsEntryRow> DaemonCacheStore::fsEntries(const QString& rootId) const
{
    QList<CachedFsEntryRow> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "SELECT root_id,path,kind,size,mtime_ms,revision_cbor,updated_at_ms "
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

} // namespace daemonapp::daemon
