#include "persistence/sqlite_session_store.h"

#include <QDateTime>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <QtGlobal>

namespace persistence {

using domain::Session;
using domain::Tag;
using domain::UnitId;
using domain::UnitKind;
using domain::UnitNode;
using domain::UnitState;

namespace {

// Serialize a tag-id list as a compact comma-separated string for storage.
QString joinTagIds(const QList<int>& ids)
{
    QStringList parts;
    parts.reserve(ids.size());
    for (int id : ids) {
        parts << QString::number(id);
    }
    return parts.join(QLatin1Char(','));
}

QList<int> splitTagIds(const QString& csv)
{
    QList<int> out;
    if (csv.isEmpty()) {
        return out;
    }
    const QStringList parts = csv.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString& p : parts) {
        bool ok = false;
        const int v = p.toInt(&ok);
        if (ok) {
            out.push_back(v);
        }
    }
    return out;
}

} // namespace

SqliteSessionStore::SqliteSessionStore(const QString& dbPath, QObject* parent, bool seedDemoData)
    : InMemorySessionStore(parent, /*seed=*/false)
    , m_dbPath(dbPath.isEmpty() ? defaultDatabasePath() : dbPath)
{
    // A unique connection name per instance so multiple stores (GUI + a test, or
    // GUI + TUI in one process) never collide in Qt's global connection registry.
    m_connectionName = QStringLiteral("daemon-app-sessions-%1")
                           .arg(reinterpret_cast<quintptr>(this));

    openDatabase();
    createSchema();

    // Hydrate the in-memory model from disk. Demo data is opt-in so a normal
    // first run does not persist fake conversations into the user's real cache.
    if (!loadAll() && seedDemoData) {
        seedSampleData();
        saveAll();
    }

    // Write-through: the base emits changed() after every mutation (create /
    // rename / archive / pin / move / delete / setContent). Persist a fresh
    // snapshot each time. saveAll() never emits changed(), so there is no loop.
    connect(this, &ISessionStore::changed, this, &SqliteSessionStore::saveAll);
}

SqliteSessionStore::~SqliteSessionStore()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        if (db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

QString SqliteSessionStore::defaultDatabasePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::tempPath();
    }
    QDir().mkpath(dir);
    return dir + QStringLiteral("/sessions.db");
}

void SqliteSessionStore::openDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qWarning("SqliteSessionStore: failed to open %s: %s", qPrintable(m_dbPath),
                 qPrintable(db.lastError().text()));
    }
}

void SqliteSessionStore::createSchema()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS meta("
                          "key TEXT PRIMARY KEY, value TEXT)"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS nodes("
                          "id TEXT PRIMARY KEY, parent_id TEXT, name TEXT, "
                          "kind INTEGER, state INTEGER, work TEXT, ord INTEGER)"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS tags("
                          "id INTEGER PRIMARY KEY, name TEXT, color TEXT, ord INTEGER)"));
    q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS sessions("
                          "id INTEGER PRIMARY KEY, agent_id TEXT, tag_ids TEXT, "
                          "title TEXT, content TEXT, archived INTEGER, pinned INTEGER, "
                          "created TEXT, modified TEXT, ord INTEGER)"));
}

bool SqliteSessionStore::loadAll()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    m_units.clear();
    m_tags.clear();
    m_sessions.clear();

    QSqlQuery nq(db);
    nq.exec(QStringLiteral("SELECT id, parent_id, name, kind, state, work "
                           "FROM nodes ORDER BY ord ASC"));
    while (nq.next()) {
        UnitNode n;
        n.id = UnitId(nq.value(0).toString());
        n.parentId = UnitId(nq.value(1).toString());
        n.name = nq.value(2).toString();
        n.kind = static_cast<UnitKind>(nq.value(3).toInt());
        n.state = static_cast<UnitState>(nq.value(4).toInt());
        n.work = nq.value(5).toString();
        // profile/session/role are derived (not persisted; the schema predates
        // them), so a loaded unit carries the same agent identity as a seeded one.
        applyUnitMeta(n);
        m_units.push_back(n);
    }

    QSqlQuery tq(db);
    tq.exec(QStringLiteral("SELECT id, name, color FROM tags ORDER BY ord ASC"));
    while (tq.next()) {
        Tag t;
        t.id = tq.value(0).toInt();
        t.name = tq.value(1).toString();
        t.color = tq.value(2).toString();
        m_tags.push_back(t);
    }

    QSqlQuery cq(db);
    cq.exec(QStringLiteral("SELECT id, agent_id, tag_ids, title, content, archived, "
                           "pinned, created, modified FROM sessions ORDER BY ord ASC"));
    while (cq.next()) {
        Session c;
        c.id = cq.value(0).toInt();
        c.unitId = UnitId(cq.value(1).toString());
        c.tagIds = splitTagIds(cq.value(2).toString());
        c.title = cq.value(3).toString();
        c.content = cq.value(4).toString();
        c.isArchived = cq.value(5).toInt() != 0;
        c.isPinned = cq.value(6).toInt() != 0;
        c.created = QDateTime::fromString(cq.value(7).toString(), Qt::ISODate);
        c.modified = QDateTime::fromString(cq.value(8).toString(), Qt::ISODate);
        m_sessions.push_back(c);
    }

    // Restore the id counters from meta, falling back to max+1 over loaded rows.
    auto metaInt = [&](const QString& key, int fallback) {
        QSqlQuery mq(db);
        mq.prepare(QStringLiteral("SELECT value FROM meta WHERE key = ?"));
        mq.addBindValue(key);
        if (mq.exec() && mq.next()) {
            bool ok = false;
            const int v = mq.value(0).toString().toInt(&ok);
            if (ok) {
                return v;
            }
        }
        return fallback;
    };

    int maxConv = 0;
    for (const Session& c : m_sessions) {
        maxConv = std::max(maxConv, c.id);
    }
    int maxTag = 0;
    for (const Tag& t : m_tags) {
        maxTag = std::max(maxTag, t.id);
    }
    m_nextId = metaInt(QStringLiteral("next_conv_id"), maxConv + 1);
    m_nextTagId = metaInt(QStringLiteral("next_tag_id"), maxTag + 1);
    m_nextUnitSeq = metaInt(QStringLiteral("next_node_seq"), static_cast<int>(m_units.size()) + 1);

    return !m_sessions.isEmpty() || !m_units.isEmpty();
}

void SqliteSessionStore::saveAll()
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.isOpen()) {
        return;
    }

    db.transaction();

    QSqlQuery del(db);
    del.exec(QStringLiteral("DELETE FROM nodes"));
    del.exec(QStringLiteral("DELETE FROM tags"));
    del.exec(QStringLiteral("DELETE FROM sessions"));

    QSqlQuery nq(db);
    nq.prepare(QStringLiteral("INSERT INTO nodes(id, parent_id, name, kind, state, work, ord) "
                              "VALUES(?, ?, ?, ?, ?, ?, ?)"));
    for (int i = 0; i < m_units.size(); ++i) {
        const UnitNode& n = m_units.at(i);
        nq.addBindValue(n.id.toString());
        nq.addBindValue(n.parentId.toString());
        nq.addBindValue(n.name);
        nq.addBindValue(static_cast<int>(n.kind));
        nq.addBindValue(static_cast<int>(n.state));
        nq.addBindValue(n.work);
        nq.addBindValue(i);
        nq.exec();
    }

    QSqlQuery tq(db);
    tq.prepare(QStringLiteral("INSERT INTO tags(id, name, color, ord) VALUES(?, ?, ?, ?)"));
    for (int i = 0; i < m_tags.size(); ++i) {
        const Tag& t = m_tags.at(i);
        tq.addBindValue(t.id);
        tq.addBindValue(t.name);
        tq.addBindValue(t.color);
        tq.addBindValue(i);
        tq.exec();
    }

    QSqlQuery cq(db);
    cq.prepare(QStringLiteral("INSERT INTO sessions(id, agent_id, tag_ids, title, content, "
                              "archived, pinned, created, modified, ord) "
                              "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    for (int i = 0; i < m_sessions.size(); ++i) {
        const Session& c = m_sessions.at(i);
        cq.addBindValue(c.id);
        cq.addBindValue(c.unitId.toString());
        cq.addBindValue(joinTagIds(c.tagIds));
        cq.addBindValue(c.title);
        cq.addBindValue(c.content);
        cq.addBindValue(c.isArchived ? 1 : 0);
        cq.addBindValue(c.isPinned ? 1 : 0);
        cq.addBindValue(c.created.toString(Qt::ISODate));
        cq.addBindValue(c.modified.toString(Qt::ISODate));
        cq.addBindValue(i);
        cq.exec();
    }

    auto setMeta = [&](const QString& key, int value) {
        QSqlQuery mq(db);
        mq.prepare(QStringLiteral("INSERT OR REPLACE INTO meta(key, value) VALUES(?, ?)"));
        mq.addBindValue(key);
        mq.addBindValue(QString::number(value));
        mq.exec();
    };
    setMeta(QStringLiteral("next_conv_id"), m_nextId);
    setMeta(QStringLiteral("next_tag_id"), m_nextTagId);
    setMeta(QStringLiteral("next_node_seq"), m_nextUnitSeq);

    db.commit();
}

} // namespace persistence
