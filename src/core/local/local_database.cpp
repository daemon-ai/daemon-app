// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "local_database.h"

#include "platform/wasm_persistence.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace mirror {
namespace {

// --- current (kSchemaVersion) DDL. All CREATE ... IF NOT EXISTS so opening an already-current db
// (or one just migrated) is idempotent, and NOTHING is ever dropped (§4.5: the precious db).
constexpr const char* kCreateMeta =
    "CREATE TABLE IF NOT EXISTS local_meta(k TEXT PRIMARY KEY, v TEXT)";

// outbox_ops — spec 09 §6.1, verbatim column set.
constexpr const char* kCreateOutbox = "CREATE TABLE IF NOT EXISTS outbox_ops("
                                      "op_id TEXT PRIMARY KEY,"
                                      "lane TEXT NOT NULL,"
                                      "verb TEXT NOT NULL,"
                                      "payload BLOB NOT NULL,"
                                      "display TEXT NOT NULL,"
                                      "state INTEGER NOT NULL,"
                                      "attempts INTEGER NOT NULL DEFAULT 0,"
                                      "enqueued_ms INTEGER NOT NULL,"
                                      "last_error TEXT,"
                                      "schema_v INTEGER NOT NULL)";
constexpr const char* kCreateOutboxIndex =
    "CREATE INDEX IF NOT EXISTS outbox_lane ON outbox_ops(lane, enqueued_ms)";

// sidecar_conversation — spec 09 §3.4. `collapsed` is the v2 column (see the migration ladder).
constexpr const char* kCreateSidecarConversation =
    "CREATE TABLE IF NOT EXISTS sidecar_conversation("
    "key TEXT PRIMARY KEY,"
    "unread_count INTEGER NOT NULL DEFAULT 0,"
    "last_read_cursor INTEGER NOT NULL DEFAULT 0,"
    "muted INTEGER NOT NULL DEFAULT 0,"
    "collapsed INTEGER NOT NULL DEFAULT 0,"
    "draft TEXT,"
    "draft_refs TEXT,"
    "updated_ms INTEGER NOT NULL DEFAULT 0)";

// sidecar_session — spec 09 §3.4.
constexpr const char* kCreateSidecarSession = "CREATE TABLE IF NOT EXISTS sidecar_session("
                                              "key TEXT PRIMARY KEY,"
                                              "draft TEXT,"
                                              "draft_refs TEXT,"
                                              "updated_ms INTEGER NOT NULL DEFAULT 0)";

} // namespace

LocalDatabase::LocalDatabase(const QString& dbPath, QObject* parent)
    : QObject(parent), m_basePath(dbPath.isEmpty() ? defaultDatabasePath() : dbPath),
      m_connectionName(QStringLiteral("local-db-%1").arg(reinterpret_cast<quintptr>(this))) {
    openAt(m_basePath);
    platform::installUnloadFlush();
}

LocalDatabase::~LocalDatabase() {
    dropConnection();
}

QString LocalDatabase::defaultDatabasePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("local.db"));
}

QString LocalDatabase::namespacedPath(const QString& userKey) const {
    if (userKey.isEmpty()) {
        return m_basePath;
    }
    const QByteArray h =
        QCryptographicHash::hash(userKey.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    const QFileInfo base(m_basePath);
    return base.dir().filePath(QStringLiteral("local-%1.db").arg(QString::fromLatin1(h)));
}

void LocalDatabase::setIdentityNamespace(const QString& userKey) {
    const QString target = namespacedPath(userKey);
    if (target == m_dbPath && isOpen()) {
        return;
    }
    openAt(target);
}

void LocalDatabase::openAt(const QString& path) {
    dropConnection();
    m_dbPath = path;
    QDir().mkpath(QFileInfo(m_dbPath).absolutePath());
    if (!openConnection()) {
        // The local db is PRECIOUS (spec 09 §4.5): unlike the disposable cache we do NOT
        // drop-and-rebuild on failure — that would destroy pending ops / drafts. Surface the error
        // and leave the store closed.
        qWarning() << "local-db: failed to open" << m_dbPath << ":" << m_lastError;
    }
}

bool LocalDatabase::openConnection() {
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        setLastError(db.lastError().text());
        return false;
    }
    if (!ensureSchema()) {
        db.close();
        return false;
    }
    return true;
}

void LocalDatabase::dropConnection() {
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

bool LocalDatabase::isOpen() const {
    const QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    return db.isValid() && db.isOpen();
}

bool LocalDatabase::ensureSchema() {
    if (!execSql(kCreateMeta)) {
        return false;
    }
    const QString stored = meta(QStringLiteral("schema_version"));
    const int from = stored.isEmpty() ? 0 : stored.toInt();
    // Forward-only migrations for an existing older db (never a drop). A brand-new db (from == 0)
    // skips straight to createCurrentTables.
    if (from != 0 && from < kSchemaVersion) {
        if (!migrateFrom(from)) {
            return false;
        }
    }
    if (from > kSchemaVersion) {
        // A db written by a newer build: leave its data intact (never drop the precious db) and do
        // not downgrade. Newer columns are simply unread by this build.
        qWarning() << "local-db: db schema_version" << from << "is newer than this build's"
                   << kSchemaVersion << "- opening read-forward without migration";
    }
    // Add any brand-new tables (IF NOT EXISTS: a no-op for tables the migration/older db already
    // had) and stamp the current version.
    if (!createCurrentTables()) {
        return false;
    }
    return setMeta(QStringLiteral("schema_version"), QString::number(kSchemaVersion));
}

bool LocalDatabase::createCurrentTables() {
    return execSql(kCreateOutbox) && execSql(kCreateOutboxIndex) &&
           execSql(kCreateSidecarConversation) && execSql(kCreateSidecarSession);
}

bool LocalDatabase::migrateFrom(int storedVersion) {
    // v1 -> v2: sidecar_conversation gained `collapsed`. ALTER only when the column is absent so a
    // partially-migrated db is safe to re-open. (createCurrentTables adds any wholly-new tables.)
    if (storedVersion <= 1 &&
        !hasColumn(QStringLiteral("sidecar_conversation"), QStringLiteral("collapsed"))) {
        if (!execSql("ALTER TABLE sidecar_conversation ADD COLUMN collapsed INTEGER NOT NULL "
                     "DEFAULT 0")) {
            return false;
        }
    }
    return true;
}

int LocalDatabase::schemaVersion() const {
    return meta(QStringLiteral("schema_version")).toInt();
}

bool LocalDatabase::hasColumn(const QString& table, const QString& column) const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    // `table` is an internal constant (not user input); PRAGMA cannot bind an identifier.
    if (!q.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        setLastError(q.lastError().text());
        return false;
    }
    while (q.next()) {
        if (q.value(1).toString() == column) {
            return true;
        }
    }
    return false;
}

bool LocalDatabase::setMeta(const QString& key, const QString& value) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("INSERT INTO local_meta(k,v) VALUES(?,?) "
                             "ON CONFLICT(k) DO UPDATE SET v=excluded.v"));
    q.addBindValue(key);
    q.addBindValue(value);
    return execWrite(q);
}

QString LocalDatabase::meta(const QString& key) const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT v FROM local_meta WHERE k=?"));
    q.addBindValue(key);
    if (!q.exec()) {
        setLastError(q.lastError().text());
        return {};
    }
    return q.next() ? q.value(0).toString() : QString();
}

bool LocalDatabase::insertOp(const PendingOp& op) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(
        QStringLiteral("INSERT INTO outbox_ops(op_id,lane,verb,payload,display,state,attempts,"
                       "enqueued_ms,last_error,schema_v) VALUES(?,?,?,?,?,?,?,?,?,?)"));
    q.addBindValue(op.opId);
    q.addBindValue(op.lane);
    q.addBindValue(op.verb);
    q.addBindValue(op.payload);
    q.addBindValue(op.display);
    q.addBindValue(static_cast<int>(op.state));
    q.addBindValue(op.attempts);
    q.addBindValue(op.enqueuedMs);
    q.addBindValue(op.lastError);
    q.addBindValue(op.schemaV);
    return execWrite(q);
}

bool LocalDatabase::updateOp(const PendingOp& op) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("UPDATE outbox_ops SET lane=?,verb=?,payload=?,display=?,state=?,"
                             "attempts=?,enqueued_ms=?,last_error=? WHERE op_id=?"));
    q.addBindValue(op.lane);
    q.addBindValue(op.verb);
    q.addBindValue(op.payload);
    q.addBindValue(op.display);
    q.addBindValue(static_cast<int>(op.state));
    q.addBindValue(op.attempts);
    q.addBindValue(op.enqueuedMs);
    q.addBindValue(op.lastError);
    q.addBindValue(op.opId);
    return execWrite(q);
}

bool LocalDatabase::deleteOp(const QString& opId) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("DELETE FROM outbox_ops WHERE op_id=?"));
    q.addBindValue(opId);
    return execWrite(q);
}

QList<PendingOp> LocalDatabase::loadOps() const {
    QList<PendingOp> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral("SELECT op_id,lane,verb,payload,display,state,attempts,enqueued_ms,"
                               "last_error,schema_v FROM outbox_ops "
                               "ORDER BY enqueued_ms ASC, op_id ASC"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        PendingOp op;
        op.opId = q.value(0).toString();
        op.lane = q.value(1).toString();
        op.verb = q.value(2).toString();
        op.payload = q.value(3).toByteArray();
        op.display = q.value(4).toString();
        op.state = static_cast<OpState>(q.value(5).toInt());
        op.attempts = q.value(6).toInt();
        op.enqueuedMs = q.value(7).toLongLong();
        op.lastError = q.value(8).toString();
        op.schemaV = q.value(9).toInt();
        rows.append(op);
    }
    return rows;
}

int LocalDatabase::laneDepth(const QString& lane) const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT COUNT(*) FROM outbox_ops WHERE lane=?"));
    q.addBindValue(lane);
    if (!q.exec() || !q.next()) {
        setLastError(q.lastError().text());
        return 0;
    }
    return q.value(0).toInt();
}

int LocalDatabase::totalDepth() const {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM outbox_ops")) || !q.next()) {
        setLastError(q.lastError().text());
        return 0;
    }
    return q.value(0).toInt();
}

bool LocalDatabase::resetInflightToPending() {
    // Boot transition (spec 09 §6.6): inflight (1) -> pending (0), at-least-once.
    return execSql("UPDATE outbox_ops SET state=0 WHERE state=1");
}

bool LocalDatabase::upsertSidecarConversation(const SidecarConversation& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO sidecar_conversation(key,unread_count,last_read_cursor,muted,collapsed,draft,"
        "draft_refs,updated_ms) VALUES(?,?,?,?,?,?,?,?) "
        "ON CONFLICT(key) DO UPDATE SET unread_count=excluded.unread_count,"
        "last_read_cursor=excluded.last_read_cursor,muted=excluded.muted,"
        "collapsed=excluded.collapsed,draft=excluded.draft,draft_refs=excluded.draft_refs,"
        "updated_ms=excluded.updated_ms"));
    q.addBindValue(row.key);
    q.addBindValue(row.unreadCount);
    q.addBindValue(row.lastReadCursor);
    q.addBindValue(row.muted ? 1 : 0);
    q.addBindValue(row.collapsed ? 1 : 0);
    q.addBindValue(row.draft);
    q.addBindValue(row.draftRefs);
    q.addBindValue(row.updatedMs);
    return execWrite(q);
}

SidecarConversation LocalDatabase::sidecarConversation(const QString& key) const {
    SidecarConversation row;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral("SELECT key,unread_count,last_read_cursor,muted,collapsed,draft,"
                             "draft_refs,updated_ms FROM sidecar_conversation WHERE key=?"));
    q.addBindValue(key);
    if (!q.exec() || !q.next()) {
        return row;
    }
    row.key = q.value(0).toString();
    row.unreadCount = q.value(1).toInt();
    row.lastReadCursor = q.value(2).toLongLong();
    row.muted = q.value(3).toInt() != 0;
    row.collapsed = q.value(4).toInt() != 0;
    row.draft = q.value(5).toString();
    row.draftRefs = q.value(6).toString();
    row.updatedMs = q.value(7).toLongLong();
    return row;
}

QList<SidecarConversation> LocalDatabase::sidecarConversations() const {
    QList<SidecarConversation> rows;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QStringLiteral("SELECT key,unread_count,last_read_cursor,muted,collapsed,draft,"
                               "draft_refs,updated_ms FROM sidecar_conversation ORDER BY key"))) {
        setLastError(q.lastError().text());
        return rows;
    }
    while (q.next()) {
        SidecarConversation row;
        row.key = q.value(0).toString();
        row.unreadCount = q.value(1).toInt();
        row.lastReadCursor = q.value(2).toLongLong();
        row.muted = q.value(3).toInt() != 0;
        row.collapsed = q.value(4).toInt() != 0;
        row.draft = q.value(5).toString();
        row.draftRefs = q.value(6).toString();
        row.updatedMs = q.value(7).toLongLong();
        rows.append(row);
    }
    return rows;
}

bool LocalDatabase::upsertSidecarSession(const SidecarSession& row) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(QStringLiteral(
        "INSERT INTO sidecar_session(key,draft,draft_refs,updated_ms) VALUES(?,?,?,?) "
        "ON CONFLICT(key) DO UPDATE SET draft=excluded.draft,draft_refs=excluded.draft_refs,"
        "updated_ms=excluded.updated_ms"));
    q.addBindValue(row.key);
    q.addBindValue(row.draft);
    q.addBindValue(row.draftRefs);
    q.addBindValue(row.updatedMs);
    return execWrite(q);
}

SidecarSession LocalDatabase::sidecarSession(const QString& key) const {
    SidecarSession row;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(
        QStringLiteral("SELECT key,draft,draft_refs,updated_ms FROM sidecar_session WHERE key=?"));
    q.addBindValue(key);
    if (!q.exec() || !q.next()) {
        return row;
    }
    row.key = q.value(0).toString();
    row.draft = q.value(1).toString();
    row.draftRefs = q.value(2).toString();
    row.updatedMs = q.value(3).toLongLong();
    return row;
}

bool LocalDatabase::execSql(const char* sql) {
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    if (!q.exec(QString::fromUtf8(sql))) {
        setLastError(q.lastError().text());
        return false;
    }
    return true;
}

bool LocalDatabase::execWrite(QSqlQuery& query) {
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return false;
    }
    // Arms the debounced IDBFS write-back on wasm (no-op off wasm) so a durable mutation reaches
    // IndexedDB — the local db is precious, so this matters more than for the cache.
    platform::scheduleIdbfsSync();
    return true;
}

void LocalDatabase::setLastError(const QString& message) const {
    m_lastError = message;
}

} // namespace mirror
