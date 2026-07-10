#include "mirror/journal.h"

#include <algorithm>
#include <limits>

namespace mirror {

const JournalRecord& Journal::stamp(EntityKind kind, const QString& key, JournalOp op,
                                    JournalOrigin origin, const QString& originOp, qint64 atMs) {
    JournalRecord rec;
    rec.rev = ++head_rev_;
    rec.kind = kind;
    rec.key = key;
    rec.op = op;
    rec.origin = origin;
    rec.origin_op = originOp;
    rec.at_ms = atMs;
    records_.push_back(std::move(rec));
    return records_.back();
}

void Journal::appendPersisted(const JournalRecord& rec) {
    records_.push_back(rec);
    head_rev_ = std::max(head_rev_, rec.rev);
}

void Journal::registerConsumer(const QString& name, quint64 at) {
    watermarks_.insert(name, at);
}

void Journal::unregisterConsumer(const QString& name) {
    watermarks_.remove(name);
}

void Journal::advanceWatermark(const QString& name, quint64 rev) {
    auto it = watermarks_.find(name);
    if (it == watermarks_.end()) {
        watermarks_.insert(name, rev);
        return;
    }
    // Watermarks are monotonic: a consumer never rewinds its cursor (§4.4).
    it.value() = std::max(rev, it.value());
}

bool Journal::hasConsumer(const QString& name) const noexcept {
    return watermarks_.contains(name);
}

quint64 Journal::watermark(const QString& name) const noexcept {
    return watermarks_.value(name, 0);
}

quint64 Journal::compactionFloor() const noexcept {
    if (watermarks_.isEmpty()) {
        // No live consumers => everything up to head is consumed.
        return head_rev_;
    }
    quint64 floor = std::numeric_limits<quint64>::max();
    for (auto it = watermarks_.constBegin(); it != watermarks_.constEnd(); ++it) {
        floor = std::min(floor, it.value());
    }
    return floor;
}

std::vector<JournalRecord> Journal::since(quint64 sinceRev) const {
    std::vector<JournalRecord> out;
    for (const JournalRecord& r : records_) {
        if (r.rev > sinceRev) {
            out.push_back(r);
        }
    }
    return out;
}

std::vector<JournalRecord> Journal::since(quint64 sinceRev, EntityKind kind) const {
    std::vector<JournalRecord> out;
    for (const JournalRecord& r : records_) {
        if (r.rev > sinceRev && r.kind == kind) {
            out.push_back(r);
        }
    }
    return out;
}

std::size_t Journal::compact(qint64 nowMs) {
    const quint64 floor = compactionFloor();
    const qint64 ageCutoff = nowMs - retention_ms_;
    std::size_t dropped = 0;

    // Records are rev-ascending. A row is deletable iff it is below the floor AND past BOTH
    // retention bounds: at least retention_rows_ newer rows exist (head - rev >= rows) AND it is
    // older than the age window. Both must hold => we keep the LARGER of the two tails (§4.3).
    while (!records_.empty()) {
        const JournalRecord& front = records_.front();
        const bool belowFloor = front.rev < floor;
        const bool pastRowTail =
            head_rev_ >= front.rev && (head_rev_ - front.rev) >= retention_rows_;
        const bool pastAgeTail = front.at_ms < ageCutoff;
        if (belowFloor && pastRowTail && pastAgeTail) {
            records_.pop_front();
            ++dropped;
        } else {
            break;
        }
    }
    return dropped;
}

} // namespace mirror
