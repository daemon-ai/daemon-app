#include "core/persistence.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace be {

bool ChangedBlockStore::open(const QString &path)
{
    m_connectionName = QStringLiteral("be_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        m_lastError = db.lastError().text();
        return false;
    }
    QSqlQuery pragma(db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    return initialize();
}

bool ChangedBlockStore::isOpen() const
{
    return !m_connectionName.isEmpty() && QSqlDatabase::database(m_connectionName).isOpen();
}

bool ChangedBlockStore::initialize()
{
    if (!isOpen()) {
        m_lastError = QStringLiteral("database is not open");
        return false;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS block ("
        "block_id INTEGER PRIMARY KEY,"
        "ordinal INTEGER NOT NULL,"
        "type INTEGER NOT NULL,"
        "heading_level INTEGER NOT NULL,"
        "markdown_utf8 BLOB NOT NULL,"
        "updated_at INTEGER NOT NULL DEFAULT (unixepoch())"
        ")"));
    if (!ok) {
        m_lastError = query.lastError().text();
    }
    return ok;
}

bool ChangedBlockStore::saveBlocks(const QVector<BlockRecord> &blocks)
{
    if (!isOpen()) {
        m_lastError = QStringLiteral("database is not open");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        m_lastError = db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO block(block_id, ordinal, type, heading_level, markdown_utf8, updated_at) "
        "VALUES(?, ?, ?, ?, ?, unixepoch()) "
        "ON CONFLICT(block_id) DO UPDATE SET "
        "ordinal=excluded.ordinal, type=excluded.type, heading_level=excluded.heading_level, "
        "markdown_utf8=excluded.markdown_utf8, updated_at=excluded.updated_at"));

    for (qsizetype row = 0; row < blocks.size(); ++row) {
        const BlockRecord &block = blocks[row];
        if (!block.dirtyPersistence) {
            continue;
        }
        query.addBindValue(QVariant::fromValue<qulonglong>(block.id));
        query.addBindValue(row);
        query.addBindValue(static_cast<int>(block.type));
        query.addBindValue(block.headingLevel);
        query.addBindValue(block.markdownUtf8);
        if (!query.exec()) {
            m_lastError = query.lastError().text();
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        m_lastError = db.lastError().text();
        return false;
    }
    return true;
}

QVector<BlockRecord> ChangedBlockStore::loadBlocks() const
{
    QVector<BlockRecord> blocks;
    if (!isOpen()) {
        m_lastError = QStringLiteral("database is not open");
        return blocks;
    }

    QSqlQuery query(QSqlDatabase::database(m_connectionName));
    if (!query.exec(QStringLiteral("SELECT block_id, type, heading_level, markdown_utf8 FROM block ORDER BY ordinal"))) {
        m_lastError = query.lastError().text();
        return blocks;
    }

    while (query.next()) {
        BlockRecord block;
        block.id = query.value(0).toULongLong();
        block.type = static_cast<BlockType>(query.value(1).toInt());
        block.headingLevel = static_cast<quint16>(query.value(2).toUInt());
        block.markdownUtf8 = query.value(3).toByteArray();
        block.source = {0, block.markdownUtf8.size()};
        blocks.push_back(block);
    }
    return blocks;
}

QString ChangedBlockStore::lastError() const
{
    return m_lastError;
}

} // namespace be
