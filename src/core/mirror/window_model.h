#pragma once
// mirror::WindowModel<Entity> — the class-W (windowed collection) adapter (spec 09 §4.6/§8.1).
//
// A scope-bound specialization of TableModel<Entity>: its row source is one per-scope Window
// (A1's immer::flex_vector<immer::box<Entity>> + WindowMeta), cursor-ordered, and its journal
// filter is this kind AND the scope prefix. It adds the demand-paging FULFILMENT side of the
// seam: canFetchMore()/fetchMore() emit olderRequested(scope, page) so the Ingestor (A4) issues
// a backward-window fetch (before_cursor, §10.2) or forward-fills on a v38 node (§5.6). A3 owns
// the seam; A4 owns the fetch and calls setHasMoreOlder(false) when the scope reaches cursor 0.
//
// Head eviction (§4.6) is deliberately NOT journaled (the node stays the archive), so WindowModel
// enables the present-set sweep (sweepsAbsentRows) to drop evicted rows that ride along with the
// journaled append in the same commit.

#include "mirror/model.h"
#include "mirror/table_model.h"

#include <QChar>
#include <QString>
#include <utility>
#include <vector>

namespace mirror {

template <typename Entity>
class WindowModel : public TableModel<Entity> {
public:
    using Scope = ScopeOf<Entity>;

    WindowModel(Store& store, Scope scope, QObject* parent = nullptr)
        : TableModel<Entity>(store, parent), scope_(std::move(scope)) {
        prefix_ = scope_.serialize() + QChar(0x1f);
        this->setScopeKey(scope_.serialize());
        // Re-derive with the window source + scope now set (silent, construction-time).
        this->reprime();
    }

    [[nodiscard]] bool canFetchMore(const QModelIndex& parent) const override {
        return !parent.isValid() && has_more_older_;
    }

    void fetchMore(const QModelIndex& parent) override {
        if (parent.isValid() || !has_more_older_) {
            return;
        }
        Q_EMIT this->olderRequested(this->scopeKey(), this->pageSize());
    }

    // A4 sets this false once the scope's window abuts cursor 0 (no older history to fetch).
    void setHasMoreOlder(bool value) {
        has_more_older_ = value;
        Q_EMIT this->olderRequested(this->scopeKey(), 0); // wake any canFetchMore re-evaluation
    }
    [[nodiscard]] bool hasMoreOlder() const { return has_more_older_; }

    [[nodiscard]] const Scope& scope() const { return scope_; }

protected:
    [[nodiscard]] std::vector<Entity> presentRows(const MirrorModel& m) const override {
        std::vector<Entity> out;
        const auto& windows = window_for(m, TypeTag<Entity>{});
        if (windows.count(scope_) != 0U) {
            const Window<Entity>& win = windows[scope_];
            for (const auto& boxed : win.items) {
                out.push_back(boxed.get());
            }
        }
        return out;
    }

    // Identity mirrors the journal key the Store stamps for a window item (store.h appendWindow):
    // scope.serialize() ␟ window_item_key(item).
    [[nodiscard]] QString rowIdentity(const Entity& e) const override {
        return e.scope().serialize() + QChar(0x1f) + window_item_key(e);
    }

    // Cursor order (node-supplied ordering key, §8.1). TableModel's identity tiebreak keeps it
    // total.
    [[nodiscard]] bool lessThan(const Entity& a, const Entity& b) const override {
        return window_order(a) < window_order(b);
    }

    [[nodiscard]] bool matchesFilter(const JournalRecord& r) const override {
        return r.kind == this->kind() && r.key.startsWith(prefix_);
    }

    [[nodiscard]] bool sweepsAbsentRows() const override { return true; }

private:
    Scope scope_;
    QString prefix_;
    bool has_more_older_ = true;
};

} // namespace mirror
