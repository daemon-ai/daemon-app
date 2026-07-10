#pragma once
// mirror::StableIdInterner — monotonic 64-bit stable ids for adapter rows (spec 09 §8.1).
//
// Each adapter interns the canonical (serialized) entity key into a monotonically assigned
// int64 that is NEVER reused and bijective for the adapter's lifetime. QML delegates and
// persistent indexes key on this id, so a row keeps its identity across value updates, sort
// re-orderings, and even a remove -> re-add of the same key.
//
// This fixes Sink's defect: Sink derived a model id from a 32-bit qHash of (resourceId +
// entityId) (references/architecture/sink/common/modelresult.cpp:32-41), so two distinct
// entities whose hashes collided in 32 bits aliased into one model row. A monotonic counter
// cannot collide by construction — behavior re-expressed, no borrowed text (Sink is LGPL).
//
// Bounded-growth policy (documented, §8.1 "bounded growth"): the table grows by exactly one
// entry per DISTINCT key ever observed by this adapter; ids are never reclaimed while the
// adapter lives, so identity is stable even across a tombstone+reinsert. Growth is therefore
// bounded by the domain cardinality the adapter's kind/scope can present. Table (M/L) adapters
// see a finite key set; window (W) adapters are scope-scoped and torn down with their surface
// (§4.4), so a single interner never accumulates unbounded distinct cursor keys in practice.
// forget() exists for the rare case an adapter is retargeted and wants to drop a permanently
// gone key; the default row pipeline never calls it (stability first).

#include <cstdint>
#include <QHash>
#include <QString>

namespace mirror {

class StableIdInterner {
public:
    // The id assigned to the first interned key. 0 is reserved as "no id" (kInvalidId).
    static constexpr qint64 kFirstId = 1;
    static constexpr qint64 kInvalidId = 0;

    StableIdInterner() = default;

    // Assign-or-return the stable id for a canonical key. Monotonic; never reused.
    [[nodiscard]] qint64 intern(const QString& /*canonicalKey*/) {
        return kInvalidId; // RED: monotonic interning not yet implemented
    }

    // The id for a key, or kInvalidId if this adapter never interned it. Does not assign.
    [[nodiscard]] qint64 lookup(const QString& canonicalKey) const {
        return ids_.value(canonicalKey, kInvalidId);
    }

    [[nodiscard]] bool contains(const QString& canonicalKey) const {
        return ids_.contains(canonicalKey);
    }

    // Live number of interned keys (== distinct keys ever seen unless forget() was used).
    [[nodiscard]] int size() const { return ids_.size(); }

    // The id the NEXT distinct key will receive (== highest assigned + 1). Monotonic proof hook.
    [[nodiscard]] qint64 peekNext() const { return next_; }

    // Drop a key's mapping. NOT called by the row pipeline (see the stability note above); a
    // subsequent intern() of the same key would receive a NEW (higher) id, never an old one.
    void forget(const QString& canonicalKey) { ids_.remove(canonicalKey); }

private:
    QHash<QString, qint64> ids_;
    qint64 next_ = kFirstId;
};

} // namespace mirror
