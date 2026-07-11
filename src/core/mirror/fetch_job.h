#pragma once
// mirror::FetchJob — a scheduled wire fetch (spec 09 §5.4). A job = wire op + scope args +
// correlation id + priority band + coalesce key. Jobs wrap the existing correlation-id calls
// (the useful core of the 20 repositories, 07§3); the scheduler bounds and dedups them.

#include "mirror/policy_table.h"

#include <QString>

namespace mirror {

// Priority bands (§5.4): visible-surface > prefetch > reconcile. Lower value = higher priority
// (band-ordered FIFO in the scheduler).
enum class Priority : quint8 {
    VisibleSurface = 0,
    Prefetch = 1,
    Reconcile = 2,
};

struct FetchJob {
    FetchOp op = FetchOp::None;
    QString scope; // scope-tail: "", transport, or transport␟conv (unit-sep)
    Priority priority = Priority::Reconcile;
    quint64 reason = 0;      // monotonic reason id; a newer reason re-queues an in-flight job once
    quint64 afterCursor = 0; // ConvHistory forward-page anchor (the cursor fix, §13 M1)
    quint64 sinceRev = 0;    // full-mode delta read (§5.6, BR seam)
    bool fullMode = false;   // wire_delta vs refetch_diff stamping intent (§4.3)
    // [api/39 §10.3 carrier 3] the triggering event's origin_op, threaded so the fetched apply
    // stamps provenance and the outbox lands the op (§6.6). Lane coalescing keeps the NEWEST
    // event's origin (a coalesced-away op resolves via the §6.6 accepted-state disposition).
    QString originOp;

    // Coalesce key: one in-flight job per (op, scope) (§5.4 e.g. "ConvList␟matrix-1").
    [[nodiscard]] QString coalesceKey() const {
        return QString::number(static_cast<int>(op)) + QChar(0x1f) + scope;
    }

    // Correlation id: `mir/<op>/<scope-tail>` replacing `repo/...` (§5.4; unit-sep tails kept).
    [[nodiscard]] QString correlation() const {
        return QStringLiteral("mir/") + QString::number(static_cast<int>(op)) + QChar('/') + scope;
    }
};

} // namespace mirror
