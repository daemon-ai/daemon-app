#pragma once
// mirror::TableModel<Entity> — the ONE generic journal-delta collection adapter (spec 09 §8.1).
//
// A QAbstractListModel that consumes its kind's journal deltas ABOVE ITS WATERMARK and
// translates them into EXACT row ops (beginInsertRows / beginRemoveRows / beginMoveRows /
// dataChanged) through a maintained sort-key index. It NEVER calls beginResetModel outside the
// silent construction-time population (§14.8) — a rename/add/remove is always an incremental op.
//
// Design split (moc cannot process templates):
//   - TableModelBase (this file, Q_OBJECT, table_model.cpp): the QObject/QAbstractListModel shell
//     — journal-consumer watermark bookkeeping (§4.4), the Store::committed connection, the
//     64-bit stable-id interner, the demand-paging signal seam, and the stable-id invokables.
//   - TableModel<Entity> (this file, header-only template): the typed sorted-projection reconcile,
//     the auto Q_GADGET role map, and the exact delta->row-op dispatch.
//   - WindowModel<Entity> (window_model.h): the class-W specialization + demand-paging fulfilment.
//
// A concrete VM subclasses TableModel<Entity> and contributes only its curated role map
// (§8.2/§8.3, ~30-60 lines) — see conversation_list_model.h. Projections are GUI-free C++ (no
// QtQuick includes): QML binds them, the TUI consumes them via DisplayRoleAdapter (§8.4).
//
// Reader posture (§4.4/§14.2): the adapter READS the store snapshot + journal deltas and manages
// only its OWN journal-consumer watermark (consumer bookkeeping, not a mirrored-state write).
// The row cache it materializes is a read-only projection of committed node truth refreshed
// atomically per commit — not a private domain fork (§8.5/§14.7): it holds no value the node did
// not confirm and answers no domain question.

#include "mirror/journal.h"
#include "mirror/model.h"
#include "mirror/stable_id.h"
#include "mirror/store.h"

#include <algorithm>
#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QMetaProperty>
#include <QString>
#include <QVariant>
#include <utility>
#include <vector>

namespace mirror {

// ---------------------------------------------------------------------------
// The non-template QObject shell.
// ---------------------------------------------------------------------------
class TableModelBase : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles : int {
        // The 64-bit stable id of the row (QML delegate / persistent-index identity).
        StableIdRole = Qt::UserRole + 1,
        // Auto Q_GADGET property roles are assigned from here (PropertyRoleBase + propertyIndex).
        PropertyRoleBase = Qt::UserRole + 2,
    };

    ~TableModelBase() override;

    // --- demand-paging seam (§4.6). A3 only signals intent; A4 fulfils the fetch. ---
    // Ask for `count` more (older) items in this adapter's scope; 0 uses the default page size.
    Q_INVOKABLE void requestOlder(int count = 0);

    // --- 64-bit stable ids (§8.1). ---
    // The stable id of the row, or 0 if out of range. Stable across value updates and reorders.
    [[nodiscard]] Q_INVOKABLE qint64 stableIdAt(int row) const;
    // The current row of a stable id, or -1 if that id is not presently shown (relocation basis).
    [[nodiscard]] Q_INVOKABLE int rowForStableId(qint64 id) const;

    [[nodiscard]] QString scopeKey() const { return scopeKey_; }

Q_SIGNALS:
    // The store's Writer (A4 Ingestor) connects this to a backward-window fetch (§10.2) /
    // forward-fill (§5.6). scopeKey is empty for unscoped (table) adapters.
    void olderRequested(const QString& scopeKey, int count);

protected:
    TableModelBase(Store& store, EntityKind kind, QObject* parent);

    [[nodiscard]] const MirrorModel& snapshot() const;
    [[nodiscard]] Store& store() const { return *store_; }
    [[nodiscard]] EntityKind kind() const { return kind_; }
    [[nodiscard]] StableIdInterner& interner() { return interner_; }
    [[nodiscard]] const StableIdInterner& interner() const { return interner_; }
    [[nodiscard]] quint64 watermark() const { return watermark_; }
    [[nodiscard]] int pageSize() const { return defaultPageSize_; }

    // Advance the local cursor AND this consumer's journal watermark (§4.4).
    void setWatermark(quint64 rev);
    void setScopeKey(QString key) { scopeKey_ = std::move(key); }
    void setDefaultPageSize(int n) { defaultPageSize_ = n; }

    // Filled by the template (correct vtable — never called from a constructor).
    virtual void applyCommit(quint64 revFrom, quint64 revTo) = 0;
    virtual qint64 stableIdAtImpl(int row) const = 0;
    virtual int rowForStableIdImpl(qint64 id) const = 0;

private Q_SLOTS:
    void onStoreCommitted(quint64 revFrom, quint64 revTo);

private:
    Store* store_;
    EntityKind kind_;
    QString consumerName_;
    QString scopeKey_;
    quint64 watermark_ = 0;
    int defaultPageSize_ = 64;
    StableIdInterner interner_;
    static quint64 s_serial_;
};

// ---------------------------------------------------------------------------
// The generic typed adapter.
// ---------------------------------------------------------------------------
template <typename Entity>
class TableModel : public TableModelBase {
public:
    explicit TableModel(Store& store, QObject* parent = nullptr)
        : TableModelBase(store, Entity::entity_kind, parent) {
        primeFromSnapshot();
    }

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        return parent.isValid() ? 0 : static_cast<int>(rows_.size());
    }

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rows_.size())) {
            return {};
        }
        const Entity& e = rows_[static_cast<std::size_t>(index.row())];
        if (role == StableIdRole) {
            return QVariant::fromValue(internerId(e));
        }
        return roleData(e, role);
    }

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override { return defaultRoleNames(); }

    [[nodiscard]] QModelIndex index(int row, int column,
                                    const QModelIndex& parent = QModelIndex()) const override {
        if (!hasIndex(row, column, parent)) {
            return {};
        }
        // Embed the stable id as internalId (§8.1: the persistent-index basis). 64-bit on desktop/
        // TUI; on a 32-bit wasm build quintptr truncates — the StableIdRole + rowForStableId()
        // remain the portable identity path, so truncation only affects the opaque internalId.
        return createIndex(row, column, static_cast<quintptr>(stableIdAtImpl(row)));
    }

protected:
    // ---- projection hooks (overridable; WindowModel overrides all four) ----

    // The collection's current entities (final committed truth). Default: the immer::table for a
    // class-M/L entity. WindowModel overrides with the per-scope window. The if-constexpr keeps
    // this virtual compilable for class-W entities too (their base default is unused).
    [[nodiscard]] virtual std::vector<Entity> presentRows(const MirrorModel& m) const {
        std::vector<Entity> out;
        if constexpr (requires { table_for(m, TypeTag<Entity>{}); }) {
            const auto& tbl = table_for(m, TypeTag<Entity>{});
            for (const Entity& e : tbl) {
                out.push_back(e);
            }
        }
        return out;
    }

    // The canonical identity used to match journal keys and to intern the stable id. Default: the
    // class-M/L typed key. WindowModel overrides with scope ␟ item-key (matching store.h's stamp).
    [[nodiscard]] virtual QString rowIdentity(const Entity& e) const {
        if constexpr (requires { e.key(); }) {
            return e.key().serialize();
        } else {
            return QString();
        }
    }

    // Presentation order (node-supplied ordering keys only — never a domain derivation, §8.1).
    // Default sorts by identity; a total order is enforced by orderBefore() (identity tiebreak).
    [[nodiscard]] virtual bool lessThan(const Entity& a, const Entity& b) const {
        return rowIdentity(a) < rowIdentity(b);
    }

    // Journal-delta filter. Default: this kind. WindowModel adds the scope-prefix test.
    [[nodiscard]] virtual bool matchesFilter(const JournalRecord& r) const {
        return r.kind == kind();
    }

    // The per-VM role map (§8.3). Default exposes every Q_GADGET property as a role; a concrete
    // VM overrides with a curated, renamed subset (conversation_list_model.h).
    [[nodiscard]] virtual QVariant roleData(const Entity& e, int role) const {
        return propertyRoleData(e, role);
    }

    // Windows observe non-journaled head eviction (§4.6) via a present-set sweep; tables do not.
    [[nodiscard]] virtual bool sweepsAbsentRows() const { return false; }

    // A total order over entities (strict weak ordering) — presentation order, identity tiebreak.
    [[nodiscard]] bool orderBefore(const Entity& a, const Entity& b) const {
        if (lessThan(a, b)) {
            return true;
        }
        if (lessThan(b, a)) {
            return false;
        }
        return rowIdentity(a) < rowIdentity(b);
    }

    // Construction-time (silent) (re)population: no reset signal, valid only before views attach.
    // Leaves that change the sort/source/scope re-derive by calling reprime() at ctor end.
    void primeFromSnapshot() {
        rows_ = presentRows(snapshot());
        std::sort(rows_.begin(), rows_.end(),
                  [this](const Entity& a, const Entity& b) { return orderBefore(a, b); });
        for (const Entity& e : rows_) {
            (void)interner().intern(rowIdentity(e));
        }
        setWatermark(store().journal().headRev());
    }
    void reprime() {
        rows_.clear();
        primeFromSnapshot();
    }

    // ---- the delta -> row-op algorithm (§8.1) ----
    void applyCommit(quint64 /*revFrom*/, quint64 revTo) override {
        const std::vector<JournalRecord> deltas = store().journal().since(watermark(), kind());

        std::vector<QString> touched;
        QSet<QString> seen;
        for (const JournalRecord& r : deltas) {
            if (!matchesFilter(r) || seen.contains(r.key)) {
                continue;
            }
            seen.insert(r.key);
            touched.push_back(r.key);
        }

        if (!touched.empty() || sweepsAbsentRows()) {
            // Final committed state, keyed by identity. Bounded by the collection size; the
            // journal bounds the touched set. Windowed head-eviction is not journaled (§4.6), so
            // WindowModel additionally sweeps rows no longer present (sweepsAbsentRows()).
            QHash<QString, Entity> present;
            for (const Entity& e : presentRows(snapshot())) {
                present.insert(rowIdentity(e), e);
            }
            std::sort(touched.begin(), touched.end());
            for (const QString& id : touched) {
                auto it = present.constFind(id);
                if (it != present.constEnd()) {
                    upsertRow(id, it.value());
                } else {
                    removeRowByIdentity(id);
                }
            }
            if (sweepsAbsentRows()) {
                for (int i = static_cast<int>(rows_.size()) - 1; i >= 0; --i) {
                    if (!present.contains(rowIdentity(rows_[static_cast<std::size_t>(i)]))) {
                        beginRemoveRows(QModelIndex(), i, i);
                        rows_.erase(rows_.begin() + i);
                        endRemoveRows();
                    }
                }
            }
        }
        setWatermark(revTo);
    }

    [[nodiscard]] qint64 stableIdAtImpl(int row) const override {
        if (row < 0 || row >= static_cast<int>(rows_.size())) {
            return StableIdInterner::kInvalidId;
        }
        return const_cast<TableModel*>(this)->interner().intern(
            rowIdentity(rows_[static_cast<std::size_t>(row)]));
    }

    [[nodiscard]] int rowForStableIdImpl(qint64 id) const override {
        for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
            if (interner().lookup(rowIdentity(rows_[static_cast<std::size_t>(i)])) == id) {
                return i;
            }
        }
        return -1;
    }

    [[nodiscard]] const std::vector<Entity>& rows() const { return rows_; }

private:
    [[nodiscard]] qint64 internerId(const Entity& e) const {
        return const_cast<TableModel*>(this)->interner().intern(rowIdentity(e));
    }

    [[nodiscard]] QHash<int, QByteArray> defaultRoleNames() const {
        QHash<int, QByteArray> names;
        names.insert(StableIdRole, QByteArrayLiteral("stableId"));
        const QMetaObject& mo = Entity::staticMetaObject;
        for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
            names.insert(PropertyRoleBase + i, QByteArray(mo.property(i).name()));
        }
        return names;
    }

    [[nodiscard]] QVariant propertyRoleData(const Entity& e, int role) const {
        const QMetaObject& mo = Entity::staticMetaObject;
        const int i = role - PropertyRoleBase;
        if (i < mo.propertyOffset() || i >= mo.propertyCount()) {
            return {};
        }
        return mo.property(i).readOnGadget(&e);
    }

    [[nodiscard]] int indexOfIdentity(const QString& id) const {
        for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
            if (rowIdentity(rows_[static_cast<std::size_t>(i)]) == id) {
                return i;
            }
        }
        return -1;
    }

    // Insertion index in sorted order, optionally excluding the row currently at `skip`.
    [[nodiscard]] int sortedInsertPos(const Entity& e, int skip = -1) const {
        int pos = 0;
        for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
            if (i == skip) {
                continue;
            }
            if (orderBefore(rows_[static_cast<std::size_t>(i)], e)) {
                ++pos;
            }
        }
        return pos;
    }

    void upsertRow(const QString& id, const Entity& e) {
        (void)interner().intern(id);
        const int old = indexOfIdentity(id);
        if (old < 0) {
            const int pos = sortedInsertPos(e);
            beginInsertRows(QModelIndex(), pos, pos);
            rows_.insert(rows_.begin() + pos, e);
            endInsertRows();
            return;
        }
        const int newPos = sortedInsertPos(e, old);
        if (newPos == old) {
            if (!::mirror::reflect::gadget_equal(rows_[static_cast<std::size_t>(old)], e)) {
                rows_[static_cast<std::size_t>(old)] = e;
                Q_EMIT dataChanged(index(old, 0), index(old, 0));
            }
            return;
        }
        // Reorder: express the sort-key change as an exact move (never a reset, §14.8).
        const int dest = (newPos > old) ? newPos + 1 : newPos;
        beginMoveRows(QModelIndex(), old, old, QModelIndex(), dest);
        rows_.erase(rows_.begin() + old);
        rows_.insert(rows_.begin() + newPos, e);
        endMoveRows();
        Q_EMIT dataChanged(index(newPos, 0), index(newPos, 0));
    }

    void removeRowByIdentity(const QString& id) {
        const int old = indexOfIdentity(id);
        if (old < 0) {
            return;
        }
        beginRemoveRows(QModelIndex(), old, old);
        rows_.erase(rows_.begin() + old);
        endRemoveRows();
    }

    std::vector<Entity> rows_;
};

} // namespace mirror
