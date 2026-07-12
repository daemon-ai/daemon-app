#pragma once
// mirror::WriteBehind — the durable journal consumer (spec 09 §4.4/§5.1 "write-behind consumer";
// B7). It holds a durable watermark and drains the store's commit stream into ONE sqlite
// transaction per batch: entity-row upserts/deletes + window rows + window_meta + mirror_journal
// rows + the watermark advance + the wire-cursor advance, all atomic. A crash mid-apply never
// skips or double-applies (the cursor and the state it covers commit together, or neither).
//
// It is a journal consumer, not a writer of mirrored state (§5.1): it reads deltas above its
// watermark and the current snapshot; it never mutates the root. The ingestor is the sole Writer.

#include "mirror/persistence.h"
#include "mirror/store.h"

#include <QObject>
#include <QString>

namespace mirror {

class WriteBehind : public QObject {
    Q_OBJECT

public:
    WriteBehind(Store& store, Persistence& persistence,
                QString consumer = QStringLiteral("write-behind"), QObject* parent = nullptr);

    // Register the durable watermark consumer, seeded from the persisted watermark (boot path).
    void start();

    // Stash the wire feed cursor/epoch to persist atomically with the next flush (B7). The
    // ingestor/bridge calls this as it advances the feed (§5.5).
    void setPendingCursor(const QString& name, quint64 cursor, quint64 epoch);

    // Drain deltas above the watermark into one transaction. Returns false on a sql error (the
    // transaction rolled back; the watermark did NOT advance — the batch is retried next commit).
    bool flush();

    // Auto-flush on every store commit (the normal wiring). Off by default so tests drive flush().
    void setFlushOnCommit(bool on);

    [[nodiscard]] quint64 watermark() const;
    [[nodiscard]] QString lastError() const { return last_error_; }

private:
    Store& store_;
    Persistence& persistence_;
    QString consumer_;
    QString last_error_;

    bool has_pending_cursor_ = false;
    QString pending_cursor_name_;
    quint64 pending_cursor_ = 0;
    quint64 pending_epoch_ = 0;
    bool flush_on_commit_ = false;
};

} // namespace mirror
