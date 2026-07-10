#include "mirror/fetch_scheduler.h"

#include <algorithm>

namespace mirror {

bool FetchScheduler::isQueued(const QString& coalesceKey) const noexcept {
    return std::ranges::any_of(queue_,
                               [&](const Slot& s) { return s.job.coalesceKey() == coalesceKey; });
}

void FetchScheduler::enqueue(const FetchJob& job) {
    const QString key = job.coalesceKey();

    // In flight already: absorb, unless a newer reason arrived — then re-queue exactly once
    // (burst compression: the newest reason wins; intermediate ones collapse).
    if (auto it = inflight_.constFind(key); it != inflight_.constEnd()) {
        if (job.reason > it.value()) {
            requeue_.insert(key, job); // overwrite: keep only the newest pending re-request
        }
        return;
    }

    // Already queued: keep a single entry, upgrading to the newer reason and/or higher priority
    // (lower value). Preserve the FIFO seq so the job keeps its position within its band.
    for (Slot& s : queue_) {
        if (s.job.coalesceKey() == key) {
            const bool newerReason = job.reason > s.job.reason;
            const bool higherPriority =
                static_cast<int>(job.priority) < static_cast<int>(s.job.priority);
            if (newerReason || higherPriority) {
                const quint64 keepSeq = s.seq;
                s.job = job;
                s.seq = keepSeq;
            }
            pump();
            return;
        }
    }

    queue_.push_back(Slot{job, seq_counter_++});
    pump();
}

void FetchScheduler::complete(const QString& coalesceKey) {
    inflight_.remove(coalesceKey);
    // Honor a pending re-request (newer reason that arrived while the job was in flight).
    if (auto it = requeue_.constFind(coalesceKey); it != requeue_.constEnd()) {
        queue_.push_back(Slot{it.value(), seq_counter_++});
        requeue_.remove(coalesceKey);
    }
    pump();
}

void FetchScheduler::pump() {
    while (inflight_.size() < max_inflight_ && !queue_.empty()) {
        // Pick the band-ordered FIFO front: lowest priority value, then lowest seq, whose
        // coalesce key is not already in flight.
        int bestIdx = -1;
        for (int i = 0; i < queue_.size(); ++i) {
            if (inflight_.contains(queue_[i].job.coalesceKey())) {
                continue; // never two in-flight jobs for one coalesce key
            }
            if (bestIdx < 0) {
                bestIdx = i;
                continue;
            }
            const Slot& b = queue_[bestIdx];
            const Slot& c = queue_[i];
            const int bp = static_cast<int>(b.job.priority);
            const int cp = static_cast<int>(c.job.priority);
            if (cp < bp || (cp == bp && c.seq < b.seq)) {
                bestIdx = i;
            }
        }
        if (bestIdx < 0) {
            return; // all queued keys are already in flight
        }
        const FetchJob job = queue_[bestIdx].job;
        queue_.removeAt(bestIdx);
        inflight_.insert(job.coalesceKey(), job.reason);
        executor_.execute(job);
    }
}

} // namespace mirror
