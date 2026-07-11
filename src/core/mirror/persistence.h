#pragma once
// mirror::Persistence — the mirror-<id>.db disposable cache (spec 09 §4.5). Opens/creates the DB
// from the generated DDL (mirror_schema_gen.sql, embedded as a Qt resource so the drift-gated
// generated file is read, never copied); write-behind is one transaction carrying entity rows +
// mirror_journal rows + a journal watermark + a sync cursor together (B7: cursor advance and
// state apply are atomic — a crash mid-apply never skips or double-applies). Boot loads M tables
// via immer transients and swaps the root; schema-version mismatch/corruption drops and rebuilds
// (the disposable-cache posture, 07§4.1).
//
// A1 owns mirror-<id>.db ONLY. The precious local-<id>.db (outbox + sidecar, §3.4/§6) is A2's;
// this class takes a DbPathProvider so the parent can wire A2's local-db beside the mirror db
// without A1 implementing it.

#include "mirror/journal.h"
#include "mirror/model.h"

#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <vector>

namespace mirror {

// Where the per-identity DB files live (§4.5 per-user filename-hash convention). A1 uses only
// mirrorDbPath(); localDbPath() is declared for A2's module and is not consumed here.
class DbPathProvider {
public:
    virtual ~DbPathProvider() = default;
    [[nodiscard]] virtual QString mirrorDbPath() const = 0;
    [[nodiscard]] virtual QString localDbPath() const = 0; // A2 integration point
};

// A trivial fixed-path provider (tests + single-identity boot). Use ":memory:" for the mirror
// path in unit tests.
class FixedDbPaths final : public DbPathProvider {
public:
    FixedDbPaths(QString mirrorDb, QString localDb)
        : mirror_db_(std::move(mirrorDb)), local_db_(std::move(localDb)) {}
    [[nodiscard]] QString mirrorDbPath() const override { return mirror_db_; }
    [[nodiscard]] QString localDbPath() const override { return local_db_; }

private:
    QString mirror_db_;
    QString local_db_;
};

// One per-scope windowed item to persist (class-W, §4.5): the canonical entity payload + its
// window scope/cursor + provenance. A4's ingestion arms drive the chat window.
struct WindowRowWrite {
    QString kind;  // entity kind name (e.g. "ChatMessage")
    QString scope; // canonical scope key (transport␟conv)
    quint64 cursor = 0;
    QByteArray payload; // canonical entity payload (JSON stand-in until the CBOR encoder lands, BR)
    QString originOp;
    quint64 lastRev = 0;
};

// Per-scope window bookkeeping row (window_meta, §4.6), written in the same transaction.
struct WindowMetaWrite {
    QString kind;
    QString scope;
    int itemCount = 0;
    qint64 byteCount = 0;
    quint64 oldestCursor = 0;
    quint64 newestCursor = 0;
    bool contiguousToHead = false;
};

// One durable write-behind unit (§4.4/§4.5). The Conversation/Contact/Person M-tables and the
// ChatMessage W-window are the entities A4's arms touch (other M tables are wired identically —
// mechanical, codegen-friendly — as their consumers land in A5+). Journal rows + the watermark
// advance + the sync-cursor advance + node-rev state ride the SAME transaction as the rows (B7:
// cursor advance and state apply are atomic — a crash mid-apply never skips or double-applies).
struct WriteBatch {
    std::vector<Conversation> conversationUpserts;
    std::vector<ConversationKey> conversationTombstones;
    std::vector<Contact> contactUpserts;
    std::vector<ContactKey> contactTombstones;
    std::vector<Person> personUpserts;
    std::vector<PersonKey> personTombstones;
    std::vector<WindowRowWrite> windowUpserts;
    std::vector<WindowMetaWrite> windowMeta;
    // AD (1b.3): a clearWindow scope tombstone (the engine's journal-rebaseline transcript wipe)
    // drops the scope's persisted rows + meta BEFORE this batch's upserts apply, so a wiped
    // generation never survives a reboot. Scope keys of w_transcript_blocks.
    std::vector<QString> transcriptWindowClears;
    std::vector<JournalRecord> journalRecords;

    bool advanceWatermark = false;
    QString watermarkConsumer;
    quint64 watermarkRev = 0;

    bool advanceCursor = false;
    QString cursorName;
    quint64 cursorValue = 0;
    quint64 cursorEpoch = 0;
};

class Persistence {
public:
    // schema_version the generated DDL targets (mirror_schema_gen.sql / spec §4.5).
    // v12 (G2/M5): m_fleet_units gained engine/engine_agent/child_ids (the tree edge + engine
    // identity); a mismatch drops-and-rebuilds (disposable cache), no migration.
    static constexpr int kSchemaVersion = 12;

    Persistence() = default;
    ~Persistence();
    Persistence(const Persistence&) = delete;
    Persistence& operator=(const Persistence&) = delete;
    Persistence(Persistence&&) = delete;
    Persistence& operator=(Persistence&&) = delete;

    // Open/create the mirror DB at the provider's path. Applies the generated schema on a fresh
    // DB; on a schema-version mismatch drops and rebuilds (disposable cache). Returns false on a
    // hard sql error (with lastError() set).
    bool open(const DbPathProvider& paths);
    [[nodiscard]] bool isOpen() const;
    void close();
    [[nodiscard]] QString lastError() const { return last_error_; }

    [[nodiscard]] int schemaVersion() const;

    // The persisted journal head (MAX(rev)) — seeds the in-memory counter at boot (§4.3).
    [[nodiscard]] quint64 persistedJournalHead() const;

    // The distinct non-null provenance op-ids in the persisted mirror journal (§6.6 two-phase
    // boot reconciliation): the retention-tail scan the outbox's idempotent local cleanup
    // (Outbox::reconcileLandedOps) consumes so a crash between "delta persisted" and "op row
    // deleted" resolves to the op being deleted. A1's surface; A2 does the cleanup.
    [[nodiscard]] QSet<QString> persistedOriginOps() const;

    // One transaction: rows + journal + watermark + cursor (B7). Rolls back on any failure.
    bool writeBehind(const WriteBatch& batch);

    // Boot load: rebuild the M tables via transients + read watermarks/cursors, return the model.
    // (Conversation is loaded; other M tables are the same mechanical shape.) Class-W chat windows
    // are rebuilt from w_chat_messages/window_meta as the persisted contiguous tail, bounded per
    // scope by `chatWindowLimit` (the §4.6 chat policy max; <= 0 = unbounded) so a cold boot
    // renders the last-known timeline offline (E1); forward-fill-on-demand remains the reconnect
    // top-up path.
    [[nodiscard]] bool loadInto(MirrorModel& model, int chatWindowLimit = 500);

    [[nodiscard]] quint64 loadWatermark(const QString& consumer) const;
    [[nodiscard]] quint64 loadCursor(const QString& name, quint64* epochOut = nullptr) const;

    // Compaction sweep (§4.4): delete journal rows the in-memory Journal dropped below its floor
    // (the durable retention tail is kept by passing the Journal's floor/retention decision as a
    // concrete rev threshold). Returns rows deleted.
    std::size_t compactJournal(quint64 deleteBelowRev);

private:
    bool applySchema();
    bool dropAllTables();
    [[nodiscard]] bool hasMetaTable() const;

    QSqlDatabase db_;
    QString connection_name_;
    QString last_error_;
};

} // namespace mirror
