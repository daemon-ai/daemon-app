#pragma once
// mirror::FetchScheduler — the bounded, deduplicating, band-ordered fetch queue (spec 09 §5.4).
// Invariants (re-expressed from Akonadi's bounded pipeline + burst compression — behaviors only,
// LGPL: references/architecture/akonadi/src/core/monitor_p.cpp:959-971,648-751,856-936):
//   - Dedup: at most one IN-FLIGHT job per coalesce key (op, scope); a re-request while in flight
//     is ABSORBED; a re-request carrying a NEWER reason re-queues the key exactly ONCE.
//   - Priority bands: visible-surface > prefetch > reconcile; band-ordered FIFO within a band.
//   - Cap: at most `maxInflight` concurrent jobs (SPEC-DECISION 4) — bounded reconnect fan-out.
// The scheduler is transport-agnostic: it hands ready jobs to a FetchExecutor seam and is told
// when each completes. QCoro is deliberately NOT used (ADR-009) — the seam is
// shaped so a QCoro-backed executor is a drop-in.

#include "mirror/fetch_job.h"

#include <QHash>
#include <QList>

namespace mirror {

// The transport seam: issues one job (the ingestor decodes the reply and applies it, then calls
// FetchScheduler::complete for the job's coalesce key). A recording mock backs the unit tests;
// the daemon bridge wraps NodeApiClient + the vendored codec.
class FetchExecutor {
public:
    FetchExecutor() = default;
    virtual ~FetchExecutor() = default;
    FetchExecutor(const FetchExecutor&) = delete;
    FetchExecutor& operator=(const FetchExecutor&) = delete;
    virtual void execute(const FetchJob& job) = 0;
};

class FetchScheduler {
public:
    static constexpr int kDefaultMaxInflight = 4; // §5.4 SPEC-DECISION

    explicit FetchScheduler(FetchExecutor& executor, int maxInflight = kDefaultMaxInflight)
        : executor_(executor), max_inflight_(maxInflight) {}

    // Enqueue a job (dedup + band-ordered) and pump ready jobs to the executor.
    void enqueue(const FetchJob& job);

    // The executor calls this when a job (identified by its coalesce key) finishes — success or
    // failure. Frees the in-flight slot, honors a pending re-queue, and pumps the next job.
    void complete(const QString& coalesceKey);

    [[nodiscard]] int inflightCount() const noexcept { return inflight_.size(); }
    [[nodiscard]] int queuedCount() const noexcept { return queue_.size(); }
    [[nodiscard]] bool isInflight(const QString& coalesceKey) const noexcept {
        return inflight_.contains(coalesceKey);
    }
    [[nodiscard]] bool isQueued(const QString& coalesceKey) const noexcept;
    void setMaxInflight(int n) noexcept {
        max_inflight_ = n;
        pump();
    }

private:
    struct Slot {
        FetchJob job;
        quint64 seq = 0; // FIFO tiebreaker within a priority band
    };
    void pump();

    FetchExecutor& executor_;
    int max_inflight_;
    quint64 seq_counter_ = 0;
    QList<Slot> queue_;                // pending jobs (band-ordered pick at pump)
    QHash<QString, quint64> inflight_; // coalesce key -> the reason currently in flight
    QHash<QString, FetchJob> requeue_; // coalesce key -> a newer-reason job to run once on complete
};

} // namespace mirror
