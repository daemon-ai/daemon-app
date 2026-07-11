#include "mirror/persistence.h"

#include <algorithm>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
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

// Rebuild one windowed entity from its persisted canonical JSON payload (the write-behind's
// meta-object dump; the entity CBOR encoder replaces the format with BR). Generic over the
// Q_GADGET properties so no per-field code; the scope fields + window key are then overwritten
// from the row columns, which are authoritative.
template <typename T>
T gadgetFromPayload(const QByteArray& payload) {
    T m;
    const QJsonObject obj = QJsonDocument::fromJson(payload).object();
    const QMetaObject& mo = T::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        const auto it = obj.constFind(QString::fromLatin1(p.name()));
        if (it != obj.constEnd()) {
            p.writeOnGadget(&m, it->toVariant());
        }
    }
    return m;
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

QSet<QString> Persistence::persistedOriginOps() const {
    QSet<QString> out;
    if (!isOpen()) {
        return out;
    }
    // The retention-tail scan that supplies the outbox's two-phase boot reconciliation (§6.6): the
    // distinct non-null provenance op-ids that already landed in the persisted mirror journal. A2
    // does the idempotent local cleanup (Outbox::reconcileLandedOps); A1 owns this surface.
    QSqlQuery q(db_);
    if (q.exec(QStringLiteral("SELECT DISTINCT origin_op FROM mirror_journal "
                              "WHERE origin_op IS NOT NULL AND origin_op != ''"))) {
        while (q.next()) {
            const QString op = q.value(0).toString();
            if (!op.isEmpty()) {
                out.insert(op);
            }
        }
    }
    return out;
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

    // Class-W window rows (chat + transcripts) + window_meta — same transaction (§4.5/§4.6).
    // Scope CLEARS run first: a rebaseline wipe precedes its replacement generation's upserts.
    for (const QString& scope : batch.transcriptWindowClears) {
        QSqlQuery del(db_);
        del.prepare(QStringLiteral("DELETE FROM w_transcript_blocks WHERE scope=?"));
        del.addBindValue(scope);
        if (!del.exec()) {
            last_error_ = del.lastError().text();
            db_.rollback();
            return false;
        }
        QSqlQuery delMeta(db_);
        delMeta.prepare(
            QStringLiteral("DELETE FROM window_meta WHERE kind='TranscriptBlock' AND scope=?"));
        delMeta.addBindValue(scope);
        if (!delMeta.exec()) {
            last_error_ = delMeta.lastError().text();
            db_.rollback();
            return false;
        }
    }
    for (const WindowRowWrite& w : batch.windowUpserts) {
        QSqlQuery up(db_);
        up.prepare(w.kind == QLatin1String("TranscriptBlock")
                       ? QStringLiteral("INSERT OR REPLACE INTO "
                                        "w_transcript_blocks(scope,seq,payload,origin_op,last_rev)"
                                        " VALUES(?,?,?,?,?)")
                       : QStringLiteral("INSERT OR REPLACE INTO "
                                        "w_chat_messages(scope,cursor,payload,origin_op,last_rev)"
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

bool Persistence::loadInto(MirrorModel& model, int chatWindowLimit) {
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
    // Class-W chat windows (E1 offline render): rebuild each scope's persisted CONTIGUOUS TAIL —
    // rows are read newest-first bounded by the §4.6 policy max, then reversed into cursor order.
    // The on-disk row set is a superset of the in-memory window (eviction trims memory only), so
    // the newest N rows ARE the contiguous tail. Reconnect tops up forward from the seeded per-conv
    // cursor (MirrorService seeds it from newest_cursor), never re-appending what boot restored.
    QSqlQuery sq(db_);
    if (sq.exec(QStringLiteral("SELECT DISTINCT scope FROM w_chat_messages"))) {
        auto wt = model.chat.transient();
        while (sq.next()) {
            const QString scopeKey = sq.value(0).toString();
            const QStringList sp = scopeKey.split(QChar(0x1f));
            const ChatMessageScope scope{sp.value(0), sp.value(1)};

            QSqlQuery rq(db_);
            rq.prepare(QStringLiteral("SELECT cursor,payload FROM w_chat_messages WHERE scope=? "
                                      "ORDER BY cursor DESC LIMIT ?"));
            rq.addBindValue(scopeKey);
            rq.addBindValue(chatWindowLimit > 0 ? chatWindowLimit : -1); // -1 = no LIMIT (sqlite)
            if (!rq.exec()) {
                continue;
            }
            std::vector<ChatMessage> newestFirst;
            while (rq.next()) {
                auto m = gadgetFromPayload<ChatMessage>(rq.value(1).toByteArray());
                m.transport = scope.transport;
                m.conv = scope.conv;
                m.cursor = rq.value(0).toULongLong();
                newestFirst.push_back(std::move(m));
            }
            if (newestFirst.empty()) {
                continue;
            }

            Window<ChatMessage> win;
            auto items = win.items.transient();
            qint64 bytes = 0;
            for (std::size_t i = newestFirst.size(); i > 0; --i) {
                const ChatMessage& m = newestFirst[i - 1]; // reversed: oldest-first
                bytes += static_cast<qint64>(reflect::gadget_dump(m).size());
                items.push_back(immer::box<ChatMessage>{m});
            }
            win.items = items.persistent();
            win.meta.item_count = static_cast<int>(win.items.size());
            win.meta.byte_count = bytes; // same measure the in-memory append accounting uses
            win.meta.oldest_cursor = newestFirst.back().cursor;
            win.meta.newest_cursor = newestFirst.front().cursor;
            QSqlQuery mq(db_);
            mq.prepare(QStringLiteral(
                "SELECT contiguous_to_head FROM window_meta WHERE kind='ChatMessage' AND scope=?"));
            mq.addBindValue(scopeKey);
            if (mq.exec() && mq.next()) {
                win.meta.contiguous_to_head = mq.value(0).toBool();
            }
            wt.set(scope, std::move(win));
        }
        model.chat = wt.persistent();
    }

    // AD (1b.3): class-W transcript windows (E1 offline transcript render — the durability the
    // deleted legacy cache leg provided). Whole-scope reload in seq order: the window is bounded
    // by the session's replayed generation (the rebaseline clear wipes stale rows), so no LIMIT.
    QSqlQuery tsq(db_);
    if (tsq.exec(QStringLiteral("SELECT DISTINCT scope FROM w_transcript_blocks"))) {
        auto wt = model.transcripts.transient();
        while (tsq.next()) {
            const QString scopeKey = tsq.value(0).toString();
            const TranscriptBlockScope scope{scopeKey};

            QSqlQuery rq(db_);
            rq.prepare(QStringLiteral(
                "SELECT seq,payload FROM w_transcript_blocks WHERE scope=? ORDER BY seq ASC"));
            rq.addBindValue(scopeKey);
            if (!rq.exec()) {
                continue;
            }
            Window<TranscriptBlock> win;
            auto items = win.items.transient();
            qint64 bytes = 0;
            int count = 0;
            quint64 first = 0;
            quint64 last = 0;
            while (rq.next()) {
                auto b = gadgetFromPayload<TranscriptBlock>(rq.value(1).toByteArray());
                b.session = scopeKey;
                b.seq = rq.value(0).toULongLong();
                if (count == 0) {
                    first = b.seq;
                }
                last = b.seq;
                bytes += static_cast<qint64>(reflect::gadget_dump(b).size());
                items.push_back(immer::box<TranscriptBlock>{b});
                ++count;
            }
            if (count == 0) {
                continue;
            }
            win.items = items.persistent();
            win.meta.item_count = count;
            win.meta.byte_count = bytes;
            win.meta.oldest_cursor = first;
            win.meta.newest_cursor = last;
            wt.set(scope, std::move(win));
        }
        model.transcripts = wt.persistent();
    }
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
