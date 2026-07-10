// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "outbox.h"

#include <utility>

// RED stub: real implementation lands in the GREEN commit.
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
void Outbox::load() {}
QString Outbox::enqueue(const QString&, const QString&, const QByteArray&, const QString&) {
    return {};
}
void Outbox::drain() {}
void Outbox::onAck(const QString&) {}
void Outbox::onTransientFailure(const QString&, const QString&) {}
void Outbox::onRejection(const QString&, const QString&, const QString&) {}
void Outbox::onUnauthenticated() {}
void Outbox::resumeWireLanes() {}
void Outbox::onProvenanceStamped(const QString&) {}
void Outbox::reconcileLandedOps(const QSet<QString>&) {}
void Outbox::expireAcceptedOps(qint64, qint64) {}
void Outbox::retry(const QString&) {}
QByteArray Outbox::takeForEdit(const QString&) {
    return {};
}
void Outbox::discard(const QString&) {}
void Outbox::sendRemainingAnyway(const QString&) {}
PendingOp Outbox::op(const QString&) const {
    return {};
}
bool Outbox::contains(const QString&) const {
    return false;
}
bool Outbox::lanePaused(const QString&) const {
    return false;
}
int Outbox::laneDepth(const QString&) const {
    return 0;
}
int Outbox::inFlightCount() const {
    return 0;
}
qint64 Outbox::backoffMs(int) {
    return 0;
}
bool Outbox::autoReplayEnabled(int) {
    return false;
}

int Outbox::indexOf(const QString&) const {
    return -1;
}
void Outbox::persistAndCache(const PendingOp&, bool) {}
void Outbox::removeCached(const QString&) {}
qint64 Outbox::nextEligibleMs(const PendingOp&) const {
    return 0;
}
qint64 Outbox::now() const {
    return 0;
}

} // namespace mirror
