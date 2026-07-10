// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "composer_queue_model.h"

#include <QList>
#include <QString>

namespace mirror {

class Outbox;

// Binds the composer queue's storage to the outbox turn-prompt lane (spec 09 §6.7 step 1). The
// composer's `ComposerQueueModel` keeps its roles/signals; its entries are merely persisted on the
// per-session turn-prompt lane so a queued prompt survives a crash. The turn-prompt lane is a
// DISPATCH lane (§6.6): entries are consumed at drain into Submit, not awaiting read-path
// visibility — this adapter is only their durable home.
//
// One instance per composer session (its `scope` is the session id). A2 provides the mechanism +
// proof; the application-level wiring (one storage per active session, swapped on session change —
// the "stash/restore becomes a lane filter" convergence) is A5/M2.
class OutboxComposerQueueStorage : public ComposerQueueStorage {
public:
    OutboxComposerQueueStorage(Outbox* outbox, QString sessionScope);

    [[nodiscard]] QList<ComposerQueueModel::Entry> loadEntries() const override;
    void persistEntries(const QList<ComposerQueueModel::Entry>& entries) override;

    // Canonical (reversible) encoding of one queue entry as the op payload bytes. Public so the
    // round-trip is unit-testable.
    [[nodiscard]] static QByteArray encodeEntry(const ComposerQueueModel::Entry& entry);
    [[nodiscard]] static ComposerQueueModel::Entry decodeEntry(const QByteArray& payload);

private:
    Outbox* m_outbox = nullptr;
    QString m_scope;
    QString m_lane; // buildLane(TurnPrompt, scope)
};

} // namespace mirror
