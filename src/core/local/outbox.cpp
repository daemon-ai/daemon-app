// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "outbox.h"

#include "uuidv7.h"
#include "verb_class.h"

#include <algorithm>
#include <QDateTime>
#include <utility>

namespace mirror {

Outbox::Outbox(LocalDatabase* db, QObject* parent) : QObject(parent), m_db(db) {}

void Outbox::setGate(GatePredicate gate) {
    m_gate = std::move(gate);
}

void Outbox::setClock(Clock clock) {
    m_clock = std::move(clock);
}

void Outbox::setProvenanceCapable(bool capable) {
    m_provenanceCapable = capable;
}

qint64 Outbox::now() const {
    return m_clock ? m_clock() : QDateTime::currentMSecsSinceEpoch();
}

int Outbox::indexOf(const QString& opId) const {
    for (int i = 0; i < m_ops.size(); ++i) {
        if (m_ops.at(i).opId == opId) {
            return i;
        }
    }
    return -1;
}

PendingOp Outbox::op(const QString& opId) const {
    const int i = indexOf(opId);
    return i >= 0 ? m_ops.at(i) : PendingOp{};
}

bool Outbox::contains(const QString& opId) const {
    return indexOf(opId) >= 0;
}

bool Outbox::lanePaused(const QString& lane) const {
    return m_pausedLanes.contains(lane);
}

int Outbox::laneDepth(const QString& lane) const {
    int n = 0;
    for (const PendingOp& o : m_ops) {
        if (o.lane == lane) {
            ++n;
        }
    }
    return n;
}

int Outbox::inFlightCount() const {
    int n = 0;
    for (const PendingOp& o : m_ops) {
        if (o.state == OpState::Inflight) {
            ++n;
        }
    }
    return n;
}

qint64 Outbox::nextEligibleMs(const PendingOp& op) const {
    return m_nextEligibleMs.value(op.opId, 0);
}

void Outbox::load() {
    // Boot transition (spec 09 §6.6): every inflight row reverts to pending, durably, before we
    // read them back — at-least-once delivery.
    if (m_db != nullptr) {
        m_db->resetInflightToPending();
        m_ops = m_db->loadOps();
    }
    m_pausedLanes.clear();
    m_nextEligibleMs.clear();
    m_acceptedAtMs.clear();
    m_lastEnqueueMs = 0;
    for (const PendingOp& o : m_ops) {
        m_lastEnqueueMs = std::max(m_lastEnqueueMs, o.enqueuedMs);
        // A persisted rejection re-pauses its lane on boot (§6.5): the user must still act.
        if (o.state == OpState::Rejected) {
            m_pausedLanes.insert(o.lane);
        }
    }
}

QString Outbox::enqueue(const QString& verb, const QString& scope, const QByteArray& payload,
                        const QString& display) {
    const VerbClass cls = verbClass(verb);
    const QString lane = buildLane(cls, scope);

    // Latest-wins coalescing per (verb, target) for conv-meta (§6.4): a second pending edit for the
    // same target replaces the first, in place (FIFO position + op-id kept).
    if (isCoalescing(cls)) {
        for (PendingOp& o : m_ops) {
            if (o.state == OpState::Pending && o.lane == lane && o.verb == verb) {
                o.payload = payload;
                o.display = display;
                if (m_db != nullptr) {
                    m_db->updateOp(o);
                }
                emit opChanged(o.opId);
                return o.opId;
            }
        }
    }

    // Bounds (spec 09 §6.1): fail the verb VISIBLY, never a silent drop.
    if (laneDepth(lane) >= kMaxPerLane) {
        emit enqueueRejected(verb, QStringLiteral("lane full (%1 max)").arg(kMaxPerLane));
        return {};
    }
    if (m_ops.size() >= kMaxGlobal) {
        emit enqueueRejected(verb, QStringLiteral("outbox full (%1 max)").arg(kMaxGlobal));
        return {};
    }

    PendingOp op;
    op.opId = generateUuidV7();
    op.lane = lane;
    op.verb = verb;
    op.payload = payload;
    op.display = display;
    op.state = OpState::Pending;
    op.attempts = 0;
    // Monotonic stamp so FIFO is total even under a constant/colliding clock; the on-disk order key
    // is enqueued_ms, so this also survives reload.
    op.enqueuedMs = std::max(now(), m_lastEnqueueMs + 1);
    m_lastEnqueueMs = op.enqueuedMs;
    op.lastError.clear();
    op.schemaV = LocalDatabase::kSchemaVersion;

    // The DURABILITY POINT (spec 09 §6.6): the insert commits BEFORE any send.
    if (m_db != nullptr && !m_db->insertOp(op)) {
        emit enqueueRejected(verb, QStringLiteral("durable enqueue failed"));
        return {};
    }
    m_ops.append(op);
    emit opAdded(op.opId);
    return op.opId;
}

void Outbox::drain() {
    if (m_db == nullptr) {
        return;
    }
    const qint64 nowMs = now();
    int inflight = inFlightCount();

    // Lanes that already hold an in-flight op are busy (one in-flight per lane). Accepted ops do
    // NOT block: the wire is free after the ack.
    QSet<QString> busyLanes;
    for (const PendingOp& o : m_ops) {
        if (o.state == OpState::Inflight) {
            busyLanes.insert(o.lane);
        }
    }

    // Walk in FIFO order (m_ops is enqueued_ms-ordered): the first pending op encountered for a
    // lane IS that lane's head, so marking a lane busy when its head can't go correctly forbids
    // reordering a later op in the same lane ahead of it.
    for (PendingOp& o : m_ops) {
        if (inflight >= kMaxInFlight) {
            break;
        }
        if (o.state != OpState::Pending || busyLanes.contains(o.lane) ||
            m_pausedLanes.contains(o.lane)) {
            continue;
        }
        const VerbClass cls = verbClass(o.verb);
        if (m_wirePaused && requiresConnected(cls)) {
            continue; // Unauthenticated: wire lanes held until re-auth
        }
        if (nowMs < nextEligibleMs(o)) {
            busyLanes.insert(o.lane); // head in its backoff window: lane holds (FIFO)
            continue;
        }
        if (m_gate && !m_gate(o.lane, cls)) {
            busyLanes.insert(o.lane); // gated (not connected / session busy): lane holds
            continue;
        }
        o.state = OpState::Inflight;
        m_db->updateOp(o);
        busyLanes.insert(o.lane);
        ++inflight;
        const PendingOp snapshot = o;
        emit opChanged(snapshot.opId);
        emit sendRequested(snapshot);
    }
}

void Outbox::onAck(const QString& opId) {
    const int i = indexOf(opId);
    if (i < 0) {
        return;
    }
    m_nextEligibleMs.remove(opId);
    if (!m_provenanceCapable) {
        // Pre-provenance node (api < 39): an ack is terminal success — delete on ack (§6.6).
        const QString lane = m_ops.at(i).lane;
        if (m_db != nullptr) {
            m_db->deleteOp(opId);
        }
        m_ops.removeAt(i);
        emit opRemoved(opId);
        Q_UNUSED(lane)
        return;
    }
    // Provenance node (api >= 39): admitted, awaiting a provenance-stamped delta.
    m_ops[i].state = OpState::Accepted;
    m_acceptedAtMs.insert(opId, now());
    if (m_db != nullptr) {
        m_db->updateOp(m_ops[i]);
    }
    emit opChanged(opId);
}

void Outbox::onTransientFailure(const QString& opId, const QString& message) {
    const int i = indexOf(opId);
    if (i < 0) {
        return;
    }
    PendingOp& o = m_ops[i];
    o.state = OpState::Pending;
    o.attempts += 1;
    o.lastError = message;
    if (m_db != nullptr) {
        m_db->updateOp(o);
    }
    // Exponential backoff 1s * 2^n capped 60s (spec 09 §6.3); n = retry index (attempts-1), so the
    // first retry waits 1s. Order is kept: the op stays the lane head, just not yet eligible.
    m_nextEligibleMs.insert(opId, now() + backoffMs(o.attempts - 1));
    emit opChanged(opId);
}

void Outbox::onRejection(const QString& opId, const QString& apiErrorKind, const QString& message) {
    const ErrorClass ec = classifyError(apiErrorKind);
    if (ec == ErrorClass::Unauthenticated) {
        onUnauthenticated(); // pause all wire lanes; NOT a per-op rejection (§6.5)
        return;
    }
    if (ec == ErrorClass::Transient) {
        onTransientFailure(opId, message);
        return;
    }
    const int i = indexOf(opId);
    if (i < 0) {
        return;
    }
    PendingOp& o = m_ops[i];
    o.state = OpState::Rejected;
    o.lastError = apiErrorKind + QStringLiteral(": ") + message;
    if (m_db != nullptr) {
        m_db->updateOp(o);
    }
    // Pause the lane (§6.5): later ops must not reorder around the failure.
    if (!m_pausedLanes.contains(o.lane)) {
        m_pausedLanes.insert(o.lane);
        emit lanePausedChanged(o.lane, true);
    }
    emit opChanged(opId);
}

void Outbox::onUnauthenticated() {
    m_wirePaused = true;
}

void Outbox::resumeWireLanes() {
    m_wirePaused = false;
}

void Outbox::onProvenanceStamped(const QString& opId) {
    if (opId.isEmpty()) {
        return; // null/unattributed origin: heuristic attribution is A4/A5 (§6.6, ADR-006)
    }
    const int i = indexOf(opId);
    if (i < 0) {
        return;
    }
    if (m_db != nullptr) {
        m_db->deleteOp(opId);
    }
    m_ops.removeAt(i);
    m_nextEligibleMs.remove(opId);
    m_acceptedAtMs.remove(opId);
    emit opRemoved(opId);
}

void Outbox::reconcileLandedOps(const QSet<QString>& persistedOpIds) {
    // Two-phase landing, local half (spec 09 §6.6): an op whose op-id already appears in persisted
    // provenance was landed before the crash — delete it. The journal-tail scan that SUPPLIES the
    // set is A1's surface (seam).
    const QList<PendingOp> snapshot = m_ops;
    for (const PendingOp& o : snapshot) {
        if (persistedOpIds.contains(o.opId)) {
            onProvenanceStamped(o.opId);
        }
    }
}

void Outbox::expireAcceptedOps(qint64 nowMs, qint64 graceMs) {
    // Accepted-state disposition (spec 09 §6.6): dedup makes a lingering accepted op harmless, so
    // dispose one left accepted past the grace window.
    const QList<PendingOp> snapshot = m_ops;
    for (const PendingOp& o : snapshot) {
        if (o.state != OpState::Accepted) {
            continue;
        }
        const qint64 acceptedAt = m_acceptedAtMs.value(o.opId, o.enqueuedMs);
        if (nowMs - acceptedAt >= graceMs) {
            onProvenanceStamped(o.opId); // idempotent delete + signal
        }
    }
}

void Outbox::retry(const QString& opId) {
    const int i = indexOf(opId);
    if (i < 0) {
        return;
    }
    PendingOp& o = m_ops[i];
    // Same op-id -> pending (dedup-safe on a provenance node). Unpause the lane.
    o.state = OpState::Pending;
    o.lastError.clear();
    if (m_db != nullptr) {
        m_db->updateOp(o);
    }
    m_nextEligibleMs.remove(opId);
    if (m_pausedLanes.remove(o.lane)) {
        emit lanePausedChanged(o.lane, false);
    }
    emit opChanged(opId);
}

QByteArray Outbox::takeForEdit(const QString& opId) {
    const int i = indexOf(opId);
    if (i < 0) {
        return {};
    }
    const PendingOp o = m_ops.at(i);
    const QByteArray payload = o.payload;
    // The old op is discarded — the caller re-opens it in the initiating surface and enqueues a NEW
    // op-id (§6.5). Removing the offending op unpauses its lane.
    if (m_db != nullptr) {
        m_db->deleteOp(opId);
    }
    m_ops.removeAt(i);
    m_nextEligibleMs.remove(opId);
    m_acceptedAtMs.remove(opId);
    if (m_pausedLanes.remove(o.lane)) {
        emit lanePausedChanged(o.lane, false);
    }
    emit opRemoved(opId);
    return payload;
}

void Outbox::discard(const QString& opId) {
    const int i = indexOf(opId);
    if (i < 0) {
        return;
    }
    const PendingOp o = m_ops.at(i);
    if (m_db != nullptr) {
        m_db->deleteOp(opId);
    }
    m_ops.removeAt(i);
    m_nextEligibleMs.remove(opId);
    m_acceptedAtMs.remove(opId);
    // If that op was the reason the lane paused, and no other rejected op remains in the lane,
    // resume it.
    if (o.state == OpState::Rejected && m_pausedLanes.contains(o.lane)) {
        bool stillRejected = false;
        for (const PendingOp& r : m_ops) {
            if (r.lane == o.lane && r.state == OpState::Rejected) {
                stillRejected = true;
                break;
            }
        }
        if (!stillRejected) {
            m_pausedLanes.remove(o.lane);
            emit lanePausedChanged(o.lane, false);
        }
    }
    emit opRemoved(opId);
}

void Outbox::sendRemainingAnyway(const QString& lane) {
    // Per-lane "send remaining anyway" (§6.5): drop the paused rejected op(s) and resume so the
    // remaining ops drain. Order beyond the dropped rejection is preserved.
    const QList<PendingOp> snapshot = m_ops;
    for (const PendingOp& o : snapshot) {
        if (o.lane == lane && o.state == OpState::Rejected) {
            if (m_db != nullptr) {
                m_db->deleteOp(o.opId);
            }
            const int i = indexOf(o.opId);
            if (i >= 0) {
                m_ops.removeAt(i);
            }
            m_nextEligibleMs.remove(o.opId);
            m_acceptedAtMs.remove(o.opId);
            emit opRemoved(o.opId);
        }
    }
    if (m_pausedLanes.remove(lane)) {
        emit lanePausedChanged(lane, false);
    }
}

qint64 Outbox::backoffMs(int attempts) {
    attempts = std::max(attempts, 0);
    if (attempts >= 30) {
        return 60000;
    }
    const qint64 v = static_cast<qint64>(1000) << attempts; // 1s * 2^attempts
    return std::min<qint64>(v, 60000);
}

bool Outbox::autoReplayEnabled(int /*apiVersion*/) {
    // HARD-DISABLED in A2 for EVERY api version (spec 09 §6.8). Auto-replay is enabled per
    // connection later by the BR bridge once api >= 39; here the outbox only ever drains on an
    // explicit manual tap. The apiVersion parameter documents the seam a later package relaxes.
    return false;
}

} // namespace mirror
