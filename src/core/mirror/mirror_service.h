// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror::MirrorService — the app-graph composition root of the mirror substrate (spec 09 §2,
// §4, §5, wave M2). A4 shipped the ingestor DECOUPLED (a FetchExecutor seam + deliver*(),
// unit-tested with a recording mock); A5 wires it LIVE. This one QObject owns and connects:
//
//   Observe (coarse) → Store (+ Journal) → WriteBehind → mirror-<id>.db
//   FetchScheduler → (forwarded) FetchExecutor delegate
//   Ingestor (the single Writer, §5.1)
//
// It is MODE-AGNOSTIC: the only mode-specific piece is the FetchExecutor delegate (daemon:
// DaemonFetchExecutor over NodeApiClient + the codec's decode-to-mirror bridge; mock:
// SeededFetchExecutor fed from the mock services — A8 replaces it with mirror::Seeder). Events
// arrive via ingest(); the daemon bridge translates the wire feed into mirror::NodeEvent
// (translateNodeEvent) and calls ingest(). The write-behind consumer flushes on every commit so
// a cold boot renders migrated surfaces from mirror.db with no connection (offline render, §12).

#include "mirror/fetch_scheduler.h"
#include "mirror/ingestor.h"
#include "mirror/node_event.h"
#include "mirror/observe_coarse.h"
#include "mirror/persistence.h"
#include "mirror/store.h"
#include "mirror/write_behind.h"

#include <memory>
#include <QObject>
#include <QString>

namespace mirror {

class MirrorService : public QObject {
    Q_OBJECT

public:
    explicit MirrorService(QObject* parent = nullptr);
    ~MirrorService() override;

    // Boot AND per-identity reopen (§4.5): open mirror-<id>.db at the provider's path, load the M
    // tables + the chat windows' persisted tails via transients, adopt the snapshot (journal
    // rebased to the new persisted head; per-conv cursors re-seeded), then start the write-behind
    // consumer flushing on every commit. Returns false on a hard sql error (the in-memory store
    // still works; the app degrades to a non-persisted cache and logs). Calling it again with a
    // different identity's paths closes the current file and rebases onto the new one — the prior
    // identity's rows/deltas never leak. Live lens models do NOT re-prime on a mid-run switch (the
    // A6 seam); the graph guards reopen to actual identity CHANGES.
    bool open(const DbPathProvider& paths);

    // Run entirely in memory (no persistence) — used by mock mode until A8's seeder + the WASM/
    // IDBFS path decide the mock persistence story, and by headless tests.
    void openInMemory();

    // Install the mode-specific fetch executor delegate (daemon or mock/seeder). The scheduler
    // forwards ready jobs here; null (the default) drops jobs (a store with no feeder still renders
    // from mirror.db). Ownership stays with the caller.
    void setFetchExecutor(FetchExecutor* executor);

    // The single event entry point (§5.1). The daemon bridge calls this per translated feed event.
    void ingest(const NodeEvent& event);

    [[nodiscard]] Store& store() noexcept { return store_; }
    [[nodiscard]] Ingestor& ingestor() noexcept { return ingestor_; }
    [[nodiscard]] FetchScheduler& scheduler() noexcept { return scheduler_; }
    [[nodiscard]] Persistence& persistence() noexcept { return persistence_; }
    [[nodiscard]] WriteBehind& writeBehind() noexcept { return write_behind_; }
    [[nodiscard]] bool persistent() const noexcept { return persistent_; }

private:
    // The scheduler holds a FetchExecutor& for its lifetime; this trivial forwarder lets the
    // mode-specific delegate be installed AFTER construction (breaking the service↔executor
    // construction cycle — the daemon executor needs the ingestor + scheduler this service owns).
    class ForwardingExecutor final : public FetchExecutor {
    public:
        void setDelegate(FetchExecutor* d) noexcept { delegate_ = d; }
        void execute(const FetchJob& job) override {
            if (delegate_ != nullptr) {
                delegate_->execute(job);
            }
        }

    private:
        FetchExecutor* delegate_ = nullptr;
    };

    CoarseObserve observe_;
    Store store_;
    Persistence persistence_;
    WriteBehind write_behind_;
    ForwardingExecutor forwarding_;
    FetchScheduler scheduler_;
    Ingestor ingestor_;
    bool persistent_ = false;
    bool wb_started_ = false; // write-behind starts once; reopen only rebases its watermark
};

} // namespace mirror
