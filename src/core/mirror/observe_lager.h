#pragma once
// LagerObserve — the lager reactive-core implementation of the observe seam (ADR-008 candidate
// (a), the study's recommended default). A lager::state<MirrorModel, transactional_tag> is the
// root cursor; publish() stages the new snapshot and lager::commit()s it, so every watcher sees
// one consistent snapshot and exactly one notification per batch (glitch-free two-phase,
// equality-gated; 01§2.1/§3.2). Scalar/derived Q_PROPERTY reads would zoom readers off state() and
// wire NOTIFY via LAGER_QT_READER; here we expose the root reader + a watch hook, enough for the
// spike's glitch-freedom / single-notify measurements.
//
// Only the severable header subset is pulled in (state/commit/cursor/reader/watch) — never
// lager::store/effects/debugger (ADR-002). This TU is one of the ADR-008 compile-cost samples.

#include "mirror/model.h"
#include "mirror/observe.h"

#include <functional>
#include <lager/commit.hpp>
#include <lager/cursor.hpp>
#include <lager/reader.hpp>
#include <lager/state.hpp>
#include <lager/watch.hpp>

namespace mirror {

class LagerObserve : public Observe {
public:
    LagerObserve() : state_(lager::make_state(MirrorModel{}, lager::transactional_tag{})) {
        // A single root watch proves the two-phase guarantee: it fires once per commit, after
        // the whole graph is consistent. Derived scalar readers (zoomed off state_) would hang
        // off the same graph without ever observing a half-propagated update.
        lager::watch(state_, [this](const MirrorModel&) { ++notify_count_; });
    }

    void publish(const MirrorModel& snapshot, const CommitInfo& info) override {
        last_ = info;
        state_.set(snapshot);  // transactional: staged, no notify yet
        lager::commit(state_); // send_down across the graph, THEN notify — once
    }

    [[nodiscard]] const char* seamName() const override { return "lager-headers"; }

    // The committed root snapshot (reader semantics: last committed value, 01§3.2).
    [[nodiscard]] const MirrorModel& current() const { return state_.get(); }

    // A shared reader onto the root; VMs would zoom scalar/derived readers off this.
    [[nodiscard]] lager::reader<MirrorModel> reader() { return state_; }

    [[nodiscard]] int notifyCount() const noexcept { return notify_count_; }
    [[nodiscard]] const CommitInfo& lastCommit() const noexcept { return last_; }

private:
    lager::state<MirrorModel, lager::transactional_tag> state_;
    CommitInfo last_;
    int notify_count_ = 0;
};

} // namespace mirror
