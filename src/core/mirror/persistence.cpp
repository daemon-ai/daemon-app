#include "mirror/persistence.h"

#include <algorithm>
#include <QDateTime>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>

namespace mirror {
namespace {

// Read + split the embedded generated DDL into individual statements. Line comments (--) are
// stripped; statements are ';'-terminated.
QStringList schemaStatements() {
    QFile f(QStringLiteral(":/mirror/mirror_schema_gen.sql"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    const QString all = QString::fromUtf8(f.readAll());
    QString stripped;
    stripped.reserve(all.size());
    for (const QString& line : all.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1String("--"))) {
            continue;
        }
        stripped += line;
        stripped += QLatin1Char('\n');
    }
    QStringList out;
    for (const QString& stmt : stripped.split(QLatin1Char(';'))) {
        if (!stmt.trimmed().isEmpty()) {
            out << stmt.trimmed();
        }
    }
    return out;
}

qint64 nowMsWall() {
    return QDateTime::currentMSecsSinceEpoch();
}

} // namespace

Persistence::~Persistence() {
    close();
}

bool Persistence::open(const DbPathProvider& paths) {
    close();
    connection_name_ = QStringLiteral("mirror_%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name_);
    db_.setDatabaseName(paths.mirrorDbPath());
    if (!db_.open()) {
        last_error_ = db_.lastError().text();
        return false;
    }
    QSqlQuery pragma(db_);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));

    if (!hasMetaTable()) {
        return applySchema();
    }
    if (schemaVersion() != kSchemaVersion) {
        // Disposable cache: a schema bump drops-and-rebuilds (07§4.1 posture).
        if (!dropAllTables()) {
            return false;
        }
        return applySchema();
    }
    return true;
}

bool Persistence::isOpen() const {
    return db_.isValid() && db_.isOpen();
}

void Persistence::close() {
    if (db_.isValid()) {
        if (db_.isOpen()) {
            db_.close();
        }
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name_);
        connection_name_.clear();
    }
}

bool Persistence::hasMetaTable() const {
    QSqlQuery q(db_);
    q.prepare(
        QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name='mirror_meta'"));
    return q.exec() && q.next();
}

bool Persistence::applySchema() {
    const QStringList stmts = schemaStatements();
    if (stmts.isEmpty()) {
        last_error_ = QStringLiteral("embedded mirror_schema_gen.sql missing or empty");
        return false;
    }
    if (!db_.transaction()) {
        last_error_ = db_.lastError().text();
        return false;
    }
    QSqlQuery q(db_);
    for (const QString& stmt : stmts) {
        if (!q.exec(stmt)) {
            last_error_ = q.lastError().text() + QStringLiteral(" [") + stmt + QLatin1Char(']');
            db_.rollback();
            return false;
        }
    }
    QSqlQuery meta(db_);
    meta.prepare(
        QStringLiteral("INSERT OR REPLACE INTO mirror_meta(k, v) VALUES('schema_version', ?)"));
    meta.addBindValue(QString::number(kSchemaVersion));
    if (!meta.exec()) {
        last_error_ = meta.lastError().text();
        db_.rollback();
        return false;
    }
    if (!db_.commit()) {
        last_error_ = db_.lastError().text();
        return false;
    }
    return true;
}

bool Persistence::dropAllTables() {
    QSqlQuery names(db_);
    if (!names.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table'"))) {
        last_error_ = names.lastError().text();
        return false;
    }
    QStringList tables;
    while (names.next()) {
        const QString name = names.value(0).toString();
        if (!name.startsWith(QLatin1String("sqlite_"))) {
            tables << name;
        }
    }
    for (const QString& t : tables) {
        QSqlQuery drop(db_);
        if (!drop.exec(QStringLiteral("DROP TABLE IF EXISTS \"%1\"").arg(t))) {
            last_error_ = drop.lastError().text();
            return false;
        }
    }
    return true;
}

int Persistence::schemaVersion() const {
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT v FROM mirror_meta WHERE k='schema_version'"));
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return -1;
}

quint64 Persistence::persistedJournalHead() const {
    QSqlQuery q(db_);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(rev), 0) FROM mirror_journal")) && q.next()) {
        return q.value(0).toULongLong();
    }
    return 0;
}

bool Persistence::writeBehind(const WriteBatch& batch) {
    if (!db_.transaction()) {
        last_error_ = db_.lastError().text();
        return false;
    }

    // Per-key last_rev derived from the journal rows in this same batch (0 if none).
    auto lastRevFor = [&batch](const QString& key) -> quint64 {
        quint64 rev = 0;
        for (const JournalRecord& r : batch.journalRecords) {
            if (r.key == key) {
                rev = std::max(rev, r.rev);
            }
        }
        return rev;
    };
    const qint64 at = nowMsWall();

    for (const Conversation& c : batch.conversationUpserts) {
        QSqlQuery up(db_);
        up.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO m_conversations"
            "(key,transport,id,kind,title,topic,description,parent,member_count,last_rev,"
            " fetched_at_ms) VALUES(?,?,?,?,?,?,?,?,?,?,?)"));
        const QString key = c.key().serialize();
        up.addBindValue(key);
        up.addBindValue(c.transport);
        up.addBindValue(c.id);
        up.addBindValue(c.kind);
        up.addBindValue(c.title);
        up.addBindValue(c.topic);
        up.addBindValue(c.description);
        up.addBindValue(c.parent);
        up.addBindValue(c.member_count);
        up.addBindValue(static_cast<qulonglong>(lastRevFor(key)));
        up.addBindValue(static_cast<qlonglong>(at));
        if (!up.exec()) {
            last_error_ = up.lastError().text();
            db_.rollback();
            return false;
        }
    }

    for (const ConversationKey& key : batch.conversationTombstones) {
        QSqlQuery del(db_);
        del.prepare(QStringLiteral("DELETE FROM m_conversations WHERE key=?"));
        del.addBindValue(key.serialize());
        if (!del.exec()) {
            last_error_ = del.lastError().text();
            db_.rollback();
            return false;
        }
    }

    for (const Contact& c : batch.contactUpserts) {
        QSqlQuery up(db_);
        up.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO m_contacts"
            "(key,transport,id,display_name,permission,presence_primitive,last_rev,fetched_at_ms)"
            " VALUES(?,?,?,?,?,?,?,?)"));
        const QString key = c.key().serialize();
        up.addBindValue(key);
        up.addBindValue(c.transport);
        up.addBindValue(c.id);
        up.addBindValue(c.display_name);
        up.addBindValue(c.permission);
        up.addBindValue(c.presence_primitive);
        up.addBindValue(static_cast<qulonglong>(lastRevFor(key)));
        up.addBindValue(static_cast<qlonglong>(at));
        if (!up.exec()) {
            last_error_ = up.lastError().text();
            db_.rollback();
            return false;
        }
    }
    for (const ContactKey& key : batch.contactTombstones) {
        QSqlQuery del(db_);
        del.prepare(QStringLiteral("DELETE FROM m_contacts WHERE key=?"));
        del.addBindValue(key.serialize());
        if (!del.exec()) {
            last_error_ = del.lastError().text();
            db_.rollback();
            return false;
        }
    }

    for (const Person& p : batch.personUpserts) {
        QSqlQuery up(db_);
        up.prepare(QStringLiteral("INSERT OR REPLACE INTO m_persons"
                                  "(key,id,alias,avatar_blob,endpoint_count,last_rev,fetched_at_ms)"
                                  " VALUES(?,?,?,?,?,?,?)"));
        const QString key = p.key().serialize();
        up.addBindValue(key);
        up.addBindValue(p.id);
        up.addBindValue(p.alias);
        up.addBindValue(p.avatar_blob);
        up.addBindValue(p.endpoint_count);
        up.addBindValue(static_cast<qulonglong>(lastRevFor(key)));
        up.addBindValue(static_cast<qlonglong>(at));
        if (!up.exec()) {
            last_error_ = up.lastError().text();
            db_.rollback();
            return false;
        }
    }
    for (const PersonKey& key : batch.personTombstones) {
        QSqlQuery del(db_);
        del.prepare(QStringLiteral("DELETE FROM m_persons WHERE key=?"));
        del.addBindValue(key.serialize());
        if (!del.exec()) {
            last_error_ = del.lastError().text();
            db_.rollback();
            return false;
        }
    }

    // Class-W window rows (chat) + window_meta — same transaction (§4.5/§4.6).
    for (const WindowRowWrite& w : batch.windowUpserts) {
        QSqlQuery up(db_);
        up.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO w_chat_messages(scope,cursor,payload,origin_op,last_rev)"
            " VALUES(?,?,?,?,?)"));
        up.addBindValue(w.scope);
        up.addBindValue(static_cast<qulonglong>(w.cursor));
        up.addBindValue(w.payload);
        up.addBindValue(w.originOp.isEmpty() ? QVariant() : QVariant(w.originOp));
        up.addBindValue(static_cast<qulonglong>(w.lastRev));
        if (!up.exec()) {
            last_error_ = up.lastError().text();
            db_.rollback();
            return false;
        }
    }
    for (const WindowMetaWrite& m : batch.windowMeta) {
        QSqlQuery up(db_);
        up.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO window_meta"
            "(kind,scope,item_count,byte_count,oldest_cursor,newest_cursor,contiguous_to_head)"
            " VALUES(?,?,?,?,?,?,?)"));
        up.addBindValue(m.kind);
        up.addBindValue(m.scope);
        up.addBindValue(m.itemCount);
        up.addBindValue(static_cast<qlonglong>(m.byteCount));
        up.addBindValue(static_cast<qulonglong>(m.oldestCursor));
        up.addBindValue(static_cast<qulonglong>(m.newestCursor));
        up.addBindValue(m.contiguousToHead ? 1 : 0);
        if (!up.exec()) {
            last_error_ = up.lastError().text();
            db_.rollback();
            return false;
        }
    }

    for (const JournalRecord& r : batch.journalRecords) {
        QSqlQuery j(db_);
        j.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO mirror_journal(rev,kind,key,op,origin,origin_op,at_ms)"
            " VALUES(?,?,?,?,?,?,?)"));
        j.addBindValue(static_cast<qulonglong>(r.rev));
        j.addBindValue(QString::fromLatin1(entityKindName(r.kind)));
        j.addBindValue(r.key);
        j.addBindValue(static_cast<int>(r.op));
        j.addBindValue(static_cast<int>(r.origin));
        j.addBindValue(r.origin_op.isEmpty() ? QVariant() : QVariant(r.origin_op));
        j.addBindValue(static_cast<qlonglong>(r.at_ms));
        if (!j.exec()) {
            last_error_ = j.lastError().text();
            db_.rollback();
            return false;
        }
    }

    if (batch.advanceWatermark) {
        QSqlQuery w(db_);
        w.prepare(
            QStringLiteral("INSERT OR REPLACE INTO journal_watermarks(consumer,rev) VALUES(?,?)"));
        w.addBindValue(batch.watermarkConsumer);
        w.addBindValue(static_cast<qulonglong>(batch.watermarkRev));
        if (!w.exec()) {
            last_error_ = w.lastError().text();
            db_.rollback();
            return false;
        }
    }

    if (batch.advanceCursor) {
        QSqlQuery cur(db_);
        cur.prepare(
            QStringLiteral("INSERT OR REPLACE INTO sync_cursors(name,cursor,epoch) VALUES(?,?,?)"));
        cur.addBindValue(batch.cursorName);
        cur.addBindValue(static_cast<qulonglong>(batch.cursorValue));
        cur.addBindValue(static_cast<qulonglong>(batch.cursorEpoch));
        if (!cur.exec()) {
            last_error_ = cur.lastError().text();
            db_.rollback();
            return false;
        }
    }

    if (!db_.commit()) {
        last_error_ = db_.lastError().text();
        db_.rollback();
        return false;
    }
    return true;
}

bool Persistence::loadInto(MirrorModel& model) {
    QSqlQuery q(db_);
    if (!q.exec(
            QStringLiteral("SELECT transport,id,kind,title,topic,description,parent,member_count "
                           "FROM m_conversations"))) {
        last_error_ = q.lastError().text();
        return false;
    }
    auto transient = model.conversations.transient();
    while (q.next()) {
        Conversation c;
        c.transport = q.value(0).toString();
        c.id = q.value(1).toString();
        c.kind = q.value(2).toString();
        c.title = q.value(3).toString();
        c.topic = q.value(4).toString();
        c.description = q.value(5).toString();
        c.parent = q.value(6).toString();
        c.member_count = q.value(7).toInt();
        transient.insert(c);
    }
    model.conversations = transient.persistent();

    // Rebuild the conversationsByTransport index from the loaded rows (§3.5) so the boot
    // snapshot is index-consistent before the first live apply.
    auto idx = model.conversationsByTransport.transient();
    for (const Conversation& c : model.conversations) {
        immer::flex_vector<ConversationKey> vec =
            idx.count(c.transport) ? idx[c.transport] : immer::flex_vector<ConversationKey>{};
        vec = std::move(vec).push_back(c.key());
        idx.set(c.transport, std::move(vec));
    }
    model.conversationsByTransport = idx.persistent();

    // Contacts (columnar round-trip — A4 arm ContactsChanged).
    QSqlQuery cq(db_);
    if (cq.exec(QStringLiteral(
            "SELECT transport,id,display_name,permission,presence_primitive FROM m_contacts"))) {
        auto ct = model.contacts.transient();
        while (cq.next()) {
            Contact c;
            c.transport = cq.value(0).toString();
            c.id = cq.value(1).toString();
            c.display_name = cq.value(2).toString();
            c.permission = cq.value(3).toString();
            c.presence_primitive = cq.value(4).toString();
            ct.insert(c);
        }
        model.contacts = ct.persistent();
    }

    // Persons (columnar round-trip — A4 arm PersonsChanged).
    QSqlQuery pq(db_);
    if (pq.exec(QStringLiteral("SELECT id,alias,avatar_blob,endpoint_count FROM m_persons"))) {
        auto pt = model.persons.transient();
        while (pq.next()) {
            Person p;
            p.id = pq.value(0).toString();
            p.alias = pq.value(1).toString();
            p.avatar_blob = pq.value(2).toString();
            p.endpoint_count = pq.value(3).toInt();
            pt.insert(p);
        }
        model.persons = pt.persistent();
    }
    // Chat windows are the disposable class-W cache; a cold window fills forward on demand
    // (§4.6 interim) rather than reloading rows — deliberately not rebuilt here.
    return true;
}

quint64 Persistence::loadWatermark(const QString& consumer) const {
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT rev FROM journal_watermarks WHERE consumer=?"));
    q.addBindValue(consumer);
    if (q.exec() && q.next()) {
        return q.value(0).toULongLong();
    }
    return 0;
}

quint64 Persistence::loadCursor(const QString& name, quint64* epochOut) const {
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("SELECT cursor,epoch FROM sync_cursors WHERE name=?"));
    q.addBindValue(name);
    if (q.exec() && q.next()) {
        if (epochOut != nullptr) {
            *epochOut = q.value(1).toULongLong();
        }
        return q.value(0).toULongLong();
    }
    if (epochOut != nullptr) {
        *epochOut = 0;
    }
    return 0;
}

std::size_t Persistence::compactJournal(quint64 deleteBelowRev) {
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("DELETE FROM mirror_journal WHERE rev < ?"));
    q.addBindValue(static_cast<qulonglong>(deleteBelowRev));
    if (!q.exec()) {
        last_error_ = q.lastError().text();
        return 0;
    }
    return static_cast<std::size_t>(std::max(0, q.numRowsAffected()));
}

} // namespace mirror
