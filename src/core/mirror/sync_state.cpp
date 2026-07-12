#include "mirror/sync_state.h"

#include <algorithm>

namespace mirror {

void SyncState::markStale(const QString& name) {
    Collection& c = collections_[name];
    c.state = CollectionState::Stale;
}

void SyncState::markSyncing(const QString& name) {
    Collection& c = collections_[name];
    c.state = CollectionState::Syncing;
}

void SyncState::markFresh(const QString& name, quint64 nodeRev, qint64 atMs) {
    Collection& c = collections_[name];
    c.state = CollectionState::Fresh;
    c.fetchedAtMs = atMs;
    c.nodeRev = std::max(c.nodeRev, nodeRev);
    c.lastError.clear();
}

void SyncState::markError(const QString& name, const QString& err) {
    Collection& c = collections_[name];
    c.state = CollectionState::Error;
    c.lastError = err;
}

void SyncState::setMode(const QString& name, StampingMode mode) {
    collections_[name].mode = mode;
}

QStringList SyncState::staleCollections() const {
    QStringList out;
    for (auto it = collections_.constBegin(); it != collections_.constEnd(); ++it) {
        if (it.value().state == CollectionState::Stale) {
            out << it.key();
        }
    }
    out.sort(); // deterministic scan order (tests + reproducible reconnect fan-out)
    return out;
}

bool SyncState::revUnchanged(const QString& name, quint64 eventRev, bool hasRev) const {
    if (!hasRev || eventRev == 0) {
        return false; // degraded: no wire rev ⇒ never skip (always refetch+diff)
    }
    return collections_.value(name).nodeRev == eventRev;
}

bool SyncState::setFeedEpoch(quint64 e) {
    const bool changed = (feed_epoch_ != 0 && e != feed_epoch_);
    feed_epoch_ = e;
    return changed;
}

const QStringList& SyncState::allCollections() {
    static const QStringList all = {
        QStringLiteral("sessions"), QStringLiteral("fleet"),      QStringLiteral("approvals"),
        QStringLiteral("models"),   QStringLiteral("profiles"),   QStringLiteral("conversations"),
        QStringLiteral("contacts"), QStringLiteral("persons"),    QStringLiteral("notifications"),
        QStringLiteral("chat"),     QStringLiteral("transports"), QStringLiteral("routing"),
        QStringLiteral("adapters"), // AD (1a): the adapter catalog (connect-refresh read)
    };
    return all;
}

QStringList SyncState::scopeToCollections(const QString& scope) {
    if (scope.isEmpty() || scope == QStringLiteral("all")) {
        return allCollections();
    }
    if (scope == QStringLiteral("roster")) {
        // The session inbox + the fleet tree that rides it (07§5.1 roster→sessions+fleet).
        return {QStringLiteral("sessions"), QStringLiteral("fleet")};
    }
    if (scope == QStringLiteral("profiles")) {
        return {QStringLiteral("profiles")};
    }
    // Rung 1 widens scope naming to collection names: honor a known collection, else nothing.
    if (allCollections().contains(scope)) {
        return {scope};
    }
    return {}; // unknown scope ⇒ no blind re-baseline (§5.7)
}

} // namespace mirror
