#include "mirror/write_behind.h"

#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QStringList>

namespace mirror {
namespace {

// Canonical JSON payload of a Q_GADGET entity via its meta-object (declaration-ordered, stable).
// A stand-in for canonical CBOR of the entity (spec §4.5 W payload) until the entity CBOR encoder
// lands with BR; sufficient for the disposable cache, and byte-deterministic for the E4 dump.
template <typename T>
QByteArray gadgetJson(const T& m) {
    QJsonObject obj;
    const QMetaObject& mo = T::staticMetaObject;
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        const QMetaProperty p = mo.property(i);
        obj.insert(QString::fromLatin1(p.name()), QJsonValue::fromVariant(p.readOnGadget(&m)));
    }
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

} // namespace

WriteBehind::WriteBehind(Store& store, Persistence& persistence, QString consumer, QObject* parent)
    : QObject(parent), store_(store), persistence_(persistence), consumer_(std::move(consumer)) {}

void WriteBehind::start() {
    const quint64 wm = persistence_.isOpen() ? persistence_.loadWatermark(consumer_) : 0;
    if (!store_.journal().hasConsumer(consumer_)) {
        store_.journal().registerConsumer(consumer_, wm);
    }
    connect(&store_, &Store::committed, this, [this](quint64, quint64) {
        if (flush_on_commit_) {
            flush();
        }
    });
}

void WriteBehind::setFlushOnCommit(bool on) {
    flush_on_commit_ = on;
}

void WriteBehind::setPendingCursor(const QString& name, quint64 cursor, quint64 epoch) {
    has_pending_cursor_ = true;
    pending_cursor_name_ = name;
    pending_cursor_ = cursor;
    pending_epoch_ = epoch;
}

quint64 WriteBehind::watermark() const {
    return store_.journal().watermark(consumer_);
}

bool WriteBehind::flush() {
    const quint64 wm = store_.journal().watermark(consumer_);
    const quint64 head = store_.journal().headRev();
    const std::vector<JournalRecord> deltas = store_.journal().since(wm);
    if (deltas.empty() && !has_pending_cursor_) {
        return true; // nothing to persist
    }

    const MirrorModel& snap = store_.snapshot();
    WriteBatch batch;
    batch.journalRecords = deltas;

    QSet<QString> chatScopesTouched;
    QSet<QString> transcriptScopesTouched;
    for (const JournalRecord& r : deltas) {
        const QStringList parts = r.key.split(QChar(0x1f));
        switch (r.kind) {
        case EntityKind::Conversation: {
            const ConversationKey key{parts.value(0), parts.value(1)};
            if (r.op == JournalOp::Tombstone) {
                batch.conversationTombstones.push_back(key);
            } else if (const Conversation* v = snap.conversations.find(key)) {
                batch.conversationUpserts.push_back(*v);
            }
            break;
        }
        case EntityKind::Contact: {
            const ContactKey key{parts.value(0), parts.value(1)};
            if (r.op == JournalOp::Tombstone) {
                batch.contactTombstones.push_back(key);
            } else if (const Contact* v = snap.contacts.find(key)) {
                batch.contactUpserts.push_back(*v);
            }
            break;
        }
        case EntityKind::Person: {
            const PersonKey key{parts.value(0)};
            if (r.op == JournalOp::Tombstone) {
                batch.personTombstones.push_back(key);
            } else if (const Person* v = snap.persons.find(key)) {
                batch.personUpserts.push_back(*v);
            }
            break;
        }
        case EntityKind::ChatMessage: {
            // key = transport␟conv␟cursor (§4.6 window item key).
            if (parts.size() >= 3) {
                const ChatMessageScope scope{parts.value(0), parts.value(1)};
                const quint64 cursor = parts.value(2).toULongLong();
                chatScopesTouched.insert(scope.serialize());
                if (snap.chat.count(scope)) {
                    const Window<ChatMessage>& win = snap.chat[scope];
                    for (const auto& boxed : win.items) {
                        if (boxed.get().cursor == cursor) {
                            WindowRowWrite w;
                            w.kind = QStringLiteral("ChatMessage");
                            w.scope = scope.serialize();
                            w.cursor = cursor;
                            w.payload = gadgetJson(boxed.get());
                            w.originOp = r.origin_op;
                            w.lastRev = r.rev;
                            batch.windowUpserts.push_back(std::move(w));
                            break;
                        }
                    }
                }
            }
            break;
        }
        case EntityKind::TranscriptBlock: {
            // AD (1b.3): the transcript window persists like chat, so the mirror path carries the
            // durability the deleted legacy cache leg provided (offline cold-boot transcript
            // render, E1). key = session␟seq for an item upsert; key = session (scope only) for a
            // clearWindow tombstone (the engine's journal-rebaseline wipe).
            if (r.op == JournalOp::Tombstone && parts.size() == 1) {
                batch.transcriptWindowClears.push_back(parts.value(0));
                break;
            }
            if (parts.size() >= 2) {
                const TranscriptBlockScope scope{parts.value(0)};
                const quint64 seq = parts.value(1).toULongLong();
                transcriptScopesTouched.insert(scope.serialize());
                if (snap.transcripts.count(scope)) {
                    const Window<TranscriptBlock>& win = snap.transcripts[scope];
                    for (const auto& boxed : win.items) {
                        if (boxed.get().seq == seq) {
                            WindowRowWrite w;
                            w.kind = QStringLiteral("TranscriptBlock");
                            w.scope = scope.serialize();
                            w.cursor = seq;
                            w.payload = gadgetJson(boxed.get());
                            w.originOp = r.origin_op;
                            w.lastRev = r.rev;
                            batch.windowUpserts.push_back(std::move(w));
                            break;
                        }
                    }
                }
            }
            break;
        }
        default:
            // Other M tables are wired identically as A5's verticals land; the journal row +
            // watermark still advance here (crash forensics + E5), so no delta is ever lost.
            break;
        }
    }

    // One window_meta row per touched chat scope, from the current snapshot bookkeeping (§4.6).
    for (const QString& scopeKey : chatScopesTouched) {
        const QStringList sp = scopeKey.split(QChar(0x1f));
        const ChatMessageScope scope{sp.value(0), sp.value(1)};
        if (snap.chat.count(scope)) {
            const WindowMeta& meta = snap.chat[scope].meta;
            WindowMetaWrite mw;
            mw.kind = QStringLiteral("ChatMessage");
            mw.scope = scopeKey;
            mw.itemCount = meta.item_count;
            mw.byteCount = meta.byte_count;
            mw.oldestCursor = meta.oldest_cursor;
            mw.newestCursor = meta.newest_cursor;
            mw.contiguousToHead = meta.contiguous_to_head;
            batch.windowMeta.push_back(std::move(mw));
        }
    }
    for (const QString& scopeKey : transcriptScopesTouched) {
        const TranscriptBlockScope scope{scopeKey};
        if (snap.transcripts.count(scope)) {
            const WindowMeta& meta = snap.transcripts[scope].meta;
            WindowMetaWrite mw;
            mw.kind = QStringLiteral("TranscriptBlock");
            mw.scope = scopeKey;
            mw.itemCount = meta.item_count;
            mw.byteCount = meta.byte_count;
            mw.oldestCursor = meta.oldest_cursor;
            mw.newestCursor = meta.newest_cursor;
            mw.contiguousToHead = meta.contiguous_to_head;
            batch.windowMeta.push_back(std::move(mw));
        }
    }

    batch.advanceWatermark = true;
    batch.watermarkConsumer = consumer_;
    batch.watermarkRev = head;
    if (has_pending_cursor_) {
        batch.advanceCursor = true;
        batch.cursorName = pending_cursor_name_;
        batch.cursorValue = pending_cursor_;
        batch.cursorEpoch = pending_epoch_;
    }

    if (!persistence_.writeBehind(batch)) {
        last_error_ = persistence_.lastError();
        return false; // rolled back; watermark NOT advanced — retried on the next commit
    }
    store_.journal().advanceWatermark(consumer_, head);
    has_pending_cursor_ = false;
    return true;
}

} // namespace mirror
