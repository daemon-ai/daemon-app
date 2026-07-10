// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "outbox_composer_queue_storage.h"

#include "outbox.h"
#include "verb_class.h"

#include <QCborMap>
#include <QCborValue>
#include <utility>

namespace mirror {
namespace {
constexpr int kDisplayMax = 80;

QString summarize(const QString& text) {
    const QString trimmed = text.trimmed();
    return trimmed.size() <= kDisplayMax ? trimmed : trimmed.left(kDisplayMax - 1) + QChar(0x2026);
}
} // namespace

OutboxComposerQueueStorage::OutboxComposerQueueStorage(Outbox* outbox, QString sessionScope)
    : m_outbox(outbox), m_scope(std::move(sessionScope)),
      m_lane(buildLane(VerbClass::TurnPrompt, m_scope)) {}

QByteArray OutboxComposerQueueStorage::encodeEntry(const ComposerQueueModel::Entry& entry) {
    QCborMap map;
    map[QStringLiteral("t")] = entry.text;
    map[QStringLiteral("r")] = entry.refs;
    return map.toCborValue().toCbor();
}

ComposerQueueModel::Entry OutboxComposerQueueStorage::decodeEntry(const QByteArray& payload) {
    const QCborMap map = QCborValue::fromCbor(payload).toMap();
    ComposerQueueModel::Entry e;
    e.text = map.value(QStringLiteral("t")).toString();
    e.refs = map.value(QStringLiteral("r")).toString();
    return e;
}

QList<ComposerQueueModel::Entry> OutboxComposerQueueStorage::loadEntries() const {
    QList<ComposerQueueModel::Entry> out;
    if (m_outbox == nullptr) {
        return out;
    }
    // ops() is FIFO (enqueued_ms) order, so the turn-prompt lane's entries come back in queue
    // order.
    for (const PendingOp& op : m_outbox->ops()) {
        if (op.lane == m_lane) {
            out.append(decodeEntry(op.payload));
        }
    }
    return out;
}

void OutboxComposerQueueStorage::persistEntries(const QList<ComposerQueueModel::Entry>& entries) {
    if (m_outbox == nullptr) {
        return;
    }
    // Replace this lane's durable rows with `entries` (FIFO preserved). The turn-prompt lane is a
    // dispatch lane, so the entries are simply its durable home — a full re-serialize per model
    // mutation is cheap at composer-queue sizes and keeps the durable copy an exact mirror.
    QStringList existing;
    for (const PendingOp& op : m_outbox->ops()) {
        if (op.lane == m_lane) {
            existing.append(op.opId);
        }
    }
    for (const QString& opId : existing) {
        m_outbox->discard(opId);
    }
    for (const ComposerQueueModel::Entry& e : entries) {
        m_outbox->enqueue(QStringLiteral("TurnPrompt"), m_scope, encodeEntry(e), summarize(e.text));
    }
}

} // namespace mirror
