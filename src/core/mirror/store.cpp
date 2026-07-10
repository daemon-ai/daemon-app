#include "mirror/store.h"

#include <QDateTime>
#include <utility>

namespace mirror {

Batch::Batch(Store& store) : store_(store), working_(store.current_) {}

void Batch::addConversationToIndex(const ConversationKey& key) {
    const QString transport = key.transport;
    immer::flex_vector<ConversationKey> vec = working_.conversationsByTransport.count(transport)
                                                  ? working_.conversationsByTransport[transport]
                                                  : immer::flex_vector<ConversationKey>{};
    for (const ConversationKey& existing : vec) {
        if (existing == key) {
            return; // already indexed
        }
    }
    vec = std::move(vec).push_back(key);
    working_.conversationsByTransport =
        std::move(working_.conversationsByTransport).set(transport, std::move(vec));
}

void Batch::removeConversationFromIndex(const ConversationKey& key) {
    const QString transport = key.transport;
    if (!working_.conversationsByTransport.count(transport)) {
        return;
    }
    immer::flex_vector<ConversationKey> vec = working_.conversationsByTransport[transport];
    immer::flex_vector<ConversationKey> next;
    for (const ConversationKey& existing : vec) {
        if (!(existing == key)) {
            next = std::move(next).push_back(existing);
        }
    }
    if (next.empty()) {
        working_.conversationsByTransport =
            std::move(working_.conversationsByTransport).erase(transport);
    } else {
        working_.conversationsByTransport =
            std::move(working_.conversationsByTransport).set(transport, std::move(next));
    }
}

quint64 Batch::commit() {
    Q_ASSERT_X(!committed_, "mirror::Batch::commit", "a batch may only be committed once");
    committed_ = true;
    if (staged_.empty()) {
        // No real change (all diff-before-write no-ops): no snapshot swap, no notification.
        return store_.journal_.headRev();
    }
    return store_.commitBatch(std::move(working_), staged_);
}

Store::Store(Observe& observe, QObject* parent)
    : QObject(parent), observe_(observe),
      clock_([] { return QDateTime::currentMSecsSinceEpoch(); }) {
    // §4.6 default per-scope item budgets (tunable). fs windows are dir-complete (no item cap).
    window_max_.insert(static_cast<int>(EntityKind::ChatMessage), 500);
    window_max_.insert(static_cast<int>(EntityKind::TranscriptBlock), 2000);
    window_max_.insert(static_cast<int>(EntityKind::FsEntry), 0);
}

qint64 Store::nowMs() const {
    return clock_ ? clock_() : QDateTime::currentMSecsSinceEpoch();
}

void Store::adoptLoaded(MirrorModel model, quint64 persistedJournalHead) {
    current_ = std::move(model);
    journal_.seedHead(persistedJournalHead);
    // Publish the loaded snapshot so any early observers render from cache (offline-first).
    CommitInfo info;
    info.revFrom = persistedJournalHead;
    info.revTo = persistedJournalHead;
    observe_.publish(current_, info);
}

void Store::setWindowMaxItems(EntityKind kind, int maxItems) {
    window_max_.insert(static_cast<int>(kind), maxItems);
}

int Store::windowMaxItems(EntityKind kind) const {
    return window_max_.value(static_cast<int>(kind), 0);
}

quint64 Store::commitBatch(MirrorModel&& working, const std::vector<Batch::StagedChange>& staged) {
    const quint64 revFrom = journal_.headRev();
    const qint64 at = nowMs();
    QSet<EntityKind> kinds;
    for (const Batch::StagedChange& c : staged) {
        journal_.stamp(c.kind, c.key, c.op, c.origin, c.originOp, at);
        kinds.insert(c.kind);
    }
    // Single atomic snapshot swap, then one notification wave (§4.2).
    current_ = std::move(working);
    const quint64 revTo = journal_.headRev();

    CommitInfo info;
    info.revFrom = revFrom;
    info.revTo = revTo;
    info.kinds = std::move(kinds);
    observe_.publish(current_, info);
    Q_EMIT committed(revFrom, revTo);
    return revTo;
}

} // namespace mirror
