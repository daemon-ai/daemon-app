#pragma once
// mirror::Observe — the swappable observation seam (spec 09 §4.2; ADR-008, the M0 spike THIS
// package decides). How a commit reaches scalar Q_PROPERTY reads and derived values. Collection row
// deltas do NOT go through here — they consume journal deltas either way (§8.1); the seam
// confines the choice to scalar/derived plumbing.
//
// Two conforming implementations exist behind this interface for the spike (deleted at M1):
//   (a) CoarseObserve  — handwritten per-commit signal (mirrorCommitted), per-VM value reads.
//   (b) LagerObserve   — lager::state<MirrorModel, transactional_tag> + lager::commit: a
//                        glitch-free, equality-gated two-phase observer graph.
// The Store publishes each committed snapshot through whichever seam it was constructed with;
// ADR-008's gates (compile cost + WASM smoke + code weight) pick the default.

#include "mirror/generated/entities_gen.h"

#include <cstdint>
#include <QSet>

namespace mirror {

struct MirrorModel;

// Delta metadata for one atomic commit (§4.2 "one notification wave per batch"). Kinds is the
// set of entity kinds a coarse consumer would re-read; the rev range bounds the journal deltas
// a collection adapter drains (§8.1).
struct CommitInfo {
    quint64 revFrom = 0; // journal head BEFORE the batch
    quint64 revTo = 0;   // journal head AFTER the batch
    QSet<EntityKind> kinds;

    [[nodiscard]] bool isEmpty() const noexcept { return revTo == revFrom; }
};

class Observe {
public:
    Observe() = default;
    Observe(const Observe&) = delete;
    Observe& operator=(const Observe&) = delete;
    Observe(Observe&&) = delete;
    Observe& operator=(Observe&&) = delete;
    virtual ~Observe() = default;

    // Publish one atomic snapshot + its delta info. Called by mirror::Store once per commit,
    // after the root value has been swapped (so a callback reading the seam sees the new truth).
    virtual void publish(const MirrorModel& snapshot, const CommitInfo& info) = 0;

    // Seam identity, recorded in the ADR-008 spike evidence.
    [[nodiscard]] virtual const char* seamName() const = 0;
};

} // namespace mirror
