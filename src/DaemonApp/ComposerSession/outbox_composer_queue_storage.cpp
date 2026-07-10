// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "outbox_composer_queue_storage.h"

// RED stub: real implementation lands in the GREEN commit.
namespace mirror {

OutboxComposerQueueStorage::OutboxComposerQueueStorage(Outbox* outbox, QString sessionScope)
    : m_outbox(outbox), m_scope(std::move(sessionScope)) {}

QList<ComposerQueueModel::Entry> OutboxComposerQueueStorage::loadEntries() const {
    return {};
}
void OutboxComposerQueueStorage::persistEntries(const QList<ComposerQueueModel::Entry>&) {}

QByteArray OutboxComposerQueueStorage::encodeEntry(const ComposerQueueModel::Entry&) {
    return {};
}
ComposerQueueModel::Entry OutboxComposerQueueStorage::decodeEntry(const QByteArray&) {
    return {};
}

} // namespace mirror
