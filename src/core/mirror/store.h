#pragma once
// mirror::Store — the single main-thread holder of the MirrorModel root and the apply/commit
// pipeline (spec 09 §4.1/§4.2). Rebuilt only through Batches; every commit is one atomic
// snapshot swap + one journal-record burst (revs minted here, §4.3) + one Observe notification
// (§4.2). Exactly one writer per mode (Ingestor xor Seeder, §5.1) — the Store exposes the write
// API; readers hold `const MirrorModel&` snapshots + the journal delta feed.
//
// Diff-before-write (§5.3): an upsert whose value equals the existing row produces no journal
// record, no signal, no persistence write — nearly free under immer value equality (§3.5/§8.4.7).
// Page ingest uses transients for O(1)-amortized bulk insert then ONE commit (§4.2).

#include "mirror/journal.h"
#include "mirror/model.h"
#include "mirror/observe.h"

#include <functional>
#include <QHash>
#include <QObject>
#include <QSet>
#include <type_traits>
#include <utility>
#include <vector>

namespace mirror {

// The typed key of a table entity (M/L). Windowed (W) entities use ScopeOf<T> instead.
template <typename T>
using KeyOf = std::decay_t<decltype(std::declval<T>().key())>;

// Window item ordering key -> journal key tail. One overload per class-W kind.
[[nodiscard]] inline QString window_item_key(const ChatMessage& m) {
    return QString::number(m.cursor);
}
[[nodiscard]] inline QString window_item_key(const TranscriptBlock& b) {
    return QString::number(b.seq);
}
[[nodiscard]] inline QString window_item_key(const FsEntry& e) {
    return e.path;
}

// Numeric ordering value used for window_meta oldest/newest_cursor bookkeeping (§4.6). The fs
// window is directory-keyed rather than cursor-ordered, so it reports 0.
[[nodiscard]] inline quint64 window_order(const ChatMessage& m) noexcept {
    return m.cursor;
}
[[nodiscard]] inline quint64 window_order(const TranscriptBlock& b) noexcept {
    return b.seq;
}
[[nodiscard]] inline quint64 window_order(const FsEntry&) noexcept {
    return 0;
}

class Store;

// A single applied batch (a decoded page, coalesced event burst, or seeder batch). Mutations
// accumulate on a cheap working copy of the root; commit() mints revs, swaps the snapshot, and
// notifies once. Move-only; commit() is single-shot.
class Batch {
public:
    Batch(Batch&&) = default;
    Batch& operator=(Batch&&) = delete;
    Batch(const Batch&) = delete;
    Batch& operator=(const Batch&) = delete;
    ~Batch() = default;

    // Upsert one table entity (M/L). No-op (unstamped) when the value is unchanged (§5.3).
    template <typename T>
    Batch& upsert(const T& value, JournalOrigin origin = JournalOrigin::RefetchDiff,
                  const QString& originOp = QString()) {
        auto& tbl = table_for(working_, TypeTag<T>{});
        const auto key = value.key();
        const T* existing = tbl.find(key);
        if (existing != nullptr && *existing == value) {
            return *this; // diff-before-write: unchanged, no record
        }
        tbl = tbl.insert(value); // upsert
        maintainIndexOnUpsert(value);
        stage(T::entity_kind, key.serialize(), JournalOp::Upsert, origin, originOp);
        return *this;
    }

    // Bulk page ingest via a transient (§4.2), diff-before-write per item, ONE commit at the end.
    template <typename T>
    Batch& upsertPage(const std::vector<T>& items,
                      JournalOrigin origin = JournalOrigin::RefetchDiff,
                      const QString& originOp = QString()) {
        auto& tbl = table_for(working_, TypeTag<T>{});
        auto transient = tbl.transient();
        for (const T& value : items) {
            const auto key = value.key();
            const T* existing = transient.find(key);
            if (existing != nullptr && *existing == value) {
                continue;
            }
            transient.insert(value);
            maintainIndexOnUpsert(value);
            stage(T::entity_kind, key.serialize(), JournalOp::Upsert, origin, originOp);
        }
        tbl = transient.persistent();
        return *this;
    }

    // Tombstone a table entity. No-op when the key is absent.
    template <typename T>
    Batch& tombstone(const KeyOf<T>& key, JournalOrigin origin = JournalOrigin::RefetchDiff,
                     const QString& originOp = QString()) {
        auto& tbl = table_for(working_, TypeTag<T>{});
        if (tbl.find(key) == nullptr) {
            return *this;
        }
        tbl = tbl.erase(key);
        maintainIndexOnTombstone<T>(key);
        stage(T::entity_kind, key.serialize(), JournalOp::Tombstone, origin, originOp);
        return *this;
    }

    // Append/replace a windowed (W) item in its per-scope window (§4.6). Cursor-ordered; a
    // duplicate item equal to the one already at that ordering key is a no-op (§5.3). Eviction
    // trims oldest-first past the per-kind max-items policy (never a journal tombstone — the
    // node remains the archive, §4.6).
    template <typename T>
    Batch& appendWindow(const T& item, JournalOrigin origin = JournalOrigin::RefetchDiff,
                        const QString& originOp = QString());

    // Publish: mint revs, swap the root, notify once. Returns the new head rev (== revFrom when
    // the batch was empty / all no-ops). Single-shot.
    quint64 commit();

    [[nodiscard]] int stagedCount() const noexcept { return static_cast<int>(staged_.size()); }

private:
    friend class Store;
    explicit Batch(Store& store);

    struct StagedChange {
        EntityKind kind{};
        QString key;
        JournalOp op = JournalOp::Upsert;
        JournalOrigin origin = JournalOrigin::RefetchDiff;
        QString originOp;
    };

    void stage(EntityKind kind, const QString& key, JournalOp op, JournalOrigin origin,
               const QString& originOp) {
        staged_.push_back(StagedChange{kind, key, op, origin, originOp});
    }

    // conversationsByTransport index maintenance (§3.5), apply-only. Generic for other kinds is
    // a no-op until those indexes gain consumers (A3/A4).
    template <typename T>
    void maintainIndexOnUpsert(const T& value) {
        if constexpr (std::is_same_v<T, Conversation>) {
            addConversationToIndex(value.key());
        }
    }
    template <typename T>
    void maintainIndexOnTombstone(const KeyOf<T>& key) {
        if constexpr (std::is_same_v<T, Conversation>) {
            removeConversationFromIndex(key);
        }
    }
    void addConversationToIndex(const ConversationKey& key);
    void removeConversationFromIndex(const ConversationKey& key);

    Store& store_;
    MirrorModel working_;
    std::vector<StagedChange> staged_;
    bool committed_ = false;
};

class Store : public QObject {
    Q_OBJECT

public:
    explicit Store(Observe& observe, QObject* parent = nullptr);

    [[nodiscard]] const MirrorModel& snapshot() const noexcept { return current_; }
    [[nodiscard]] Journal& journal() noexcept { return journal_; }
    [[nodiscard]] const Journal& journal() const noexcept { return journal_; }

    // Open a batch on the current snapshot (single writer; §5.1).
    [[nodiscard]] Batch beginBatch() { return Batch{*this}; }

    // Deterministic clock injection for tests; defaults to wall-clock ms.
    void setClock(std::function<qint64()> clock) { clock_ = std::move(clock); }
    [[nodiscard]] qint64 nowMs() const;

    // Boot: adopt a snapshot loaded from persistence + seed the rev counter (§4.5 boot path).
    void adoptLoaded(MirrorModel model, quint64 persistedJournalHead);

    // Per-kind window max-items policy (§4.6 defaults, tunable). chat = 500.
    void setWindowMaxItems(EntityKind kind, int maxItems);
    [[nodiscard]] int windowMaxItems(EntityKind kind) const;

Q_SIGNALS:
    // Emitted once per non-empty commit (mirrors the coarse seam signal; §4.2).
    void committed(quint64 revFrom, quint64 revTo);

private:
    friend class Batch;
    quint64 commitBatch(MirrorModel&& working, const std::vector<Batch::StagedChange>& staged);

    MirrorModel current_;
    Journal journal_;
    Observe& observe_;
    std::function<qint64()> clock_;
    QHash<int, int> window_max_;
};

// ---- Batch::appendWindow (templated; needs Store's policy accessor) --------------------------
template <typename T>
Batch& Batch::appendWindow(const T& item, JournalOrigin origin, const QString& originOp) {
    auto& windows = window_for(working_, TypeTag<T>{});
    const auto scope = item.scope();
    Window<T> win = windows.count(scope) ? windows[scope] : Window<T>{};

    // Diff-before-write: identical item already newest at head is a no-op.
    if (!win.items.empty() && win.items[win.items.size() - 1].get() == item) {
        return *this;
    }

    win.items = std::move(win.items).push_back(immer::box<T>{item});
    win.meta.item_count = static_cast<int>(win.items.size());
    win.meta.byte_count += static_cast<qint64>(::mirror::reflect::gadget_dump(item).size());
    win.meta.newest_cursor = window_order(item);
    if (win.items.size() == 1) {
        win.meta.oldest_cursor = win.meta.newest_cursor;
    }

    // Eviction (§4.6): trim oldest-first past the policy. Node remains the archive; no tombstone.
    const int maxItems = store_.windowMaxItems(T::entity_kind);
    while (maxItems > 0 && static_cast<int>(win.items.size()) > maxItems) {
        win.meta.byte_count -=
            static_cast<qint64>(::mirror::reflect::gadget_dump(win.items[0].get()).size());
        win.items = std::move(win.items).erase(0);
        win.meta.oldest_cursor = win.items.empty() ? 0 : window_order(win.items[0].get());
    }
    win.meta.item_count = static_cast<int>(win.items.size());

    windows = std::move(windows).set(scope, std::move(win));
    stage(T::entity_kind, scope.serialize() + QChar(0x1f) + window_item_key(item),
          JournalOp::Upsert, origin, originOp);
    return *this;
}

} // namespace mirror
