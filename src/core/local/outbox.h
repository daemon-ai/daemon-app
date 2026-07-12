// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "local_database.h"
#include "outbox_types.h"

#include <functional>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>

namespace mirror {

// The generalized durable write queue (spec 09 §6; ADR-003/ADR-006). Lanes with strict FIFO,
// retry-with-backoff, pause-on-rejection, and a uniform provenance-keyed confirm contract, all
// backed by the precious `local-<id>.db` (§4.5).
//
// A2 ships the outbox fully but with **manual drain only**: `drain()` is invoked by an explicit
// user action, never automatically after reconnect. Auto-replay (§6.8) is a hard-disabled gate
// here; the BR bridge flips it on per connection once the node advertises api >= 39.
//
// The outbox never touches the wire itself: `drain()` marks the lane head inflight and emits
// `sendRequested`; the owning verb seam performs the wire call and reports back via `onAck` /
// `onTransientFailure` / `onRejection`. Provenance-stamped deltas from the ingestor land ops via
// `onProvenanceStamped` (§6.6). This keeps the outbox a pure state machine over durable rows.
class Outbox : public QObject {
    Q_OBJECT

public:
    // Bounds (spec 09 §6.1): enqueue past a bound fails the verb visibly — never a silent drop.
    static constexpr int kMaxPerLane = 64;
    static constexpr int kMaxGlobal = 1024;
    // At most one in-flight op per lane; global in-flight cap round-robin across ready lanes
    // (§6.3).
    static constexpr int kMaxInFlight = 4;

    explicit Outbox(LocalDatabase* db, QObject* parent = nullptr);

    // Drain-gate predicate (spec 09 §6.3): returns true when `lane` (of class `cls`) may consume.
    // Wire lanes require connected+authenticated; the turn-prompt lane additionally requires its
    // session idle. Default gate is permissive for enqueue-only tests to stay explicit — callers
    // install the real predicate. A gated lane simply holds (no attempt consumed).
    using GatePredicate = std::function<bool(const QString& lane, VerbClass cls)>;
    void setGate(GatePredicate gate);

    // Injectable monotonic clock (ms) so backoff eligibility is deterministic in tests.
    using Clock = std::function<qint64()>;
    void setClock(Clock clock);

    // Whether the node advertises the rung-3 provenance contract (api >= 39). Default false (v38):
    // an ack is terminal success (deleted on ack — the v38-era behavior, §6.6). When true, an ack
    // moves the op to `accepted` awaiting a provenance-stamped delta.
    void setProvenanceCapable(bool capable);

    // Boot (spec 09 §6.6): load persisted ops and revert every `inflight` row to `pending`.
    void load();

    // Durable enqueue (spec 09 §6.6 durability point): the INSERT commits BEFORE any send. Returns
    // the minted UUIDv7 op-id, or an empty string on a bound breach (§6.1) with `enqueueRejected`
    // emitted. `scope` builds the lane; `verb` selects the class (§6.4). Conv-meta coalesces
    // latest-wins per (verb, scope).
    QString enqueue(const QString& verb, const QString& scope, const QByteArray& payload,
                    const QString& display);

    // Manual drain (spec 09 §6.3): advance ready lanes' heads to inflight and emit `sendRequested`,
    // honoring the gate, per-lane single in-flight, the global cap, backoff eligibility, and lane
    // pauses. NEVER called automatically in A2 (no auto-replay, §6.8).
    void drain();

    // Wire outcomes reported by the owning verb seam:
    void onAck(const QString& opId);
    void onTransientFailure(const QString& opId, const QString& message);
    void onRejection(const QString& opId, const QString& apiErrorKind, const QString& message);
    // Unauthenticated (spec 09 §6.5): pause all wire (mutation) lanes until re-auth — not a
    // rejection; ops keep their order and re-drain after `resumeWireLanes()`.
    void onUnauthenticated();
    void resumeWireLanes();

    // Confirmation (spec 09 §6.6): a provenance-stamped delta carrying `origin_op == opId` lands
    // the op (terminal success -> row deleted). A null/empty origin is "unattributed" and no-ops
    // here (heuristic attribution is A4/A5). `confirmByOpId` is the same landing reachable when the
    // provenance arrives before the ack.
    void onProvenanceStamped(const QString& opId);
    void confirmByOpId(const QString& opId) { onProvenanceStamped(opId); }

    // Boot reconciliation — local half of the two-phase landing (spec 09 §6.6): delete ops whose
    // op-id already appears in persisted provenance. The mirror-journal retention-tail scan that
    // SUPPLIES this set is A1's surface (seam); A2 does the idempotent local cleanup.
    void reconcileLandedOps(const QSet<QString>& persistedOpIds);

    // Accepted-state expiry/orphan handling (spec 09 §6.6): on a provenance node, an op left
    // `accepted` past the grace window is disposed (dedup makes a lingering accepted op harmless).
    void expireAcceptedOps(qint64 nowMs, qint64 graceMs);

    // Rejection affordances (spec 09 §6.5), one set for every lane:
    void retry(const QString& opId);             // same op-id -> pending, unpause the lane
    QByteArray takeForEdit(const QString& opId); // returns payload + deletes (caller mints new op)
    void discard(const QString& opId);
    void sendRemainingAnyway(const QString& lane); // drop the paused rejected head, unpause

    // Introspection
    [[nodiscard]] QList<PendingOp> ops() const { return m_ops; }
    [[nodiscard]] PendingOp op(const QString& opId) const;
    [[nodiscard]] bool contains(const QString& opId) const;
    [[nodiscard]] bool lanePaused(const QString& lane) const;
    [[nodiscard]] int laneDepth(const QString& lane) const;
    [[nodiscard]] int inFlightCount() const;

    // Exponential backoff (spec 09 §6.3): min(1s * 2^attempts, 60s), in ms. Pure + static so the
    // schedule is unit-testable without a clock.
    [[nodiscard]] static qint64 backoffMs(int attempts);

    // Auto-replay gate (spec 09 §6.8). HARD-DISABLED in A2: returns false for every api version,
    // including >= 39. Auto-replay is enabled per connection later by the BR bridge; here the
    // outbox only ever drains on an explicit manual tap. The `apiVersion` argument documents the
    // seam (and lets a later package relax it without changing call sites).
    [[nodiscard]] static bool autoReplayEnabled(int apiVersion);

signals:
    void opAdded(const QString& opId);
    void opChanged(const QString& opId);
    void opRemoved(const QString& opId);
    void sendRequested(const mirror::PendingOp& op);
    void enqueueRejected(const QString& verb, const QString& reason);
    void lanePausedChanged(const QString& lane, bool paused);

private:
    [[nodiscard]] int indexOf(const QString& opId) const;
    void persistAndCache(const PendingOp& op, bool isNew);
    void removeCached(const QString& opId);
    [[nodiscard]] qint64 nextEligibleMs(const PendingOp& op) const;
    [[nodiscard]] qint64 now() const;

    LocalDatabase* m_db = nullptr;
    QList<PendingOp> m_ops; // in-memory FIFO mirror of the durable rows (by enqueued_ms)
    QSet<QString> m_pausedLanes;
    // Per-op next-eligible time after a transient failure (in-memory: backoff need not survive a
    // restart — a booted op is immediately drain-eligible).
    QHash<QString, qint64> m_nextEligibleMs;
    // Per-op acceptance timestamp, for the accepted-state expiry/orphan sweep (§6.6).
    QHash<QString, qint64> m_acceptedAtMs;
    // Monotonic enqueue stamp so a lane's FIFO order is total even when the wall clock collides or
    // is injected as a constant (the on-disk order key is enqueued_ms).
    qint64 m_lastEnqueueMs = 0;
    bool m_wirePaused = false; // Unauthenticated: pause all mutation lanes
    bool m_provenanceCapable = false;
    GatePredicate m_gate;
    Clock m_clock;
};

} // namespace mirror
