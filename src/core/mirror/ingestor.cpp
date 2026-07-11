#include "mirror/ingestor.h"

#include <algorithm>
#include <QDateTime>
#include <QStringList>

namespace mirror {
namespace {

constexpr QChar kSep = QChar(0x1f);

QString laneName(CoalesceLane lane) {
    switch (lane) {
    case CoalesceLane::None:
        return {};
    case CoalesceLane::Roster:
        return QStringLiteral("roster");
    case CoalesceLane::Fleet:
        return QStringLiteral("fleet");
    case CoalesceLane::PerTransportConv:
        return QStringLiteral("conv");
    case CoalesceLane::PerTransportContact:
        return QStringLiteral("contact");
    }
    return {};
}

QString obsKey(const QString& collection, const QString& scope) {
    return collection + kSep + scope;
}

} // namespace

Ingestor::Ingestor(Store& store, FetchScheduler& scheduler, QObject* parent)
    : QObject(parent), store_(store), scheduler_(scheduler),
      clock_([&store] { return store.nowMs(); }) {}

qint64 Ingestor::nowMs() const {
    return clock_ ? clock_() : QDateTime::currentMSecsSinceEpoch();
}

// ---------------------------------------------------------------------------
// Policy dispatch (§5.2)
// ---------------------------------------------------------------------------
void Ingestor::ingest(const NodeEvent& event) {
    const PolicyRow p = policyFor(event.kind);
    switch (p.action) {
    case Action::NudgeOnly:
        if (event.kind == NodeEventKind::SessionAdvanced) {
            // Never a fetch by itself: nudge the focused engine + staleness/unread candidate.
            Q_EMIT nudgeFocused(event.session);
        } else if (event.kind == NodeEventKind::Unknown) {
            ++unknown_arm_count_; // forward compatibility (§5.2)
        }
        break;
    case Action::Fetch:
        dispatchFetch(event, p);
        break;
    case Action::PatchInPlace:
        dispatchPatch(event, p);
        break;
    case Action::KeyedPatch:
        dispatchKeyed(event, p);
        break;
    case Action::MarkStaleScan:
        handleResync(event);
        break;
    }
}

QString Ingestor::fetchScopeFor(const NodeEvent& e) const {
    switch (e.kind) {
    case NodeEventKind::MessagesChanged:
    case NodeEventKind::ConversationsChanged:
    case NodeEventKind::MembershipChanged:
        return e.transport + kSep + e.conv;
    case NodeEventKind::ContactsChanged:
    case NodeEventKind::TransportChanged:
        return e.transport;
    case NodeEventKind::SessionMetaChanged:
        return e.session;
    default:
        return {}; // global lists
    }
}

bool Ingestor::observing(const QString& collection, const QString& scope) const {
    return observing_.contains(obsKey(collection, scope)) ||
           observing_.contains(obsKey(collection, QString()));
}

void Ingestor::dispatchFetch(const NodeEvent& e, const PolicyRow& p) {
    const QString collection = QString::fromLatin1(p.collection);
    // Rung-1 skip-gate (§5.2 "full"): rev == stored ⇒ no fetch. Degraded never skips.
    if (p.skipIfRevUnchanged && state_.revUnchanged(collection, e.rev, e.hasRev)) {
        return;
    }
    const QString scope = fetchScopeFor(e);
    const Priority prio =
        observing(collection, scope) ? Priority::VisibleSurface : Priority::Prefetch;

    FetchJob job;
    job.op = p.fetchOp;
    job.scope = scope;
    job.priority = prio;
    job.reason = nextReason();
    job.fullMode = (state_.collection(collection).mode == StampingMode::WireDelta);
    if (e.kind == NodeEventKind::MessagesChanged) {
        // The rung-0 cursor fix (§13 M1): page from the stored per-conv cursor, never from 0.
        job.afterCursor = state_.convCursor(scope);
    }

    if (p.lane != CoalesceLane::None) {
        // Per-transport lanes debounce per transport; global lanes debounce together (§5.2).
        const QString laneScope = (p.lane == CoalesceLane::PerTransportConv ||
                                   p.lane == CoalesceLane::PerTransportContact)
                                      ? e.transport
                                      : QString();
        enqueueLaneFetch(p.lane, laneScope, job);
    } else {
        scheduler_.enqueue(job);
    }
    state_.markSyncing(collection);

    // MembershipChanged self-removal (§5.2): the node reconciled routing pins; the client re-lists
    // routing + rooms and nudges the roster (it derives nothing itself).
    if (e.kind == NodeEventKind::MembershipChanged && e.isSelf &&
        (e.membershipChange == QStringLiteral("left") ||
         e.membershipChange == QStringLiteral("kicked") ||
         e.membershipChange == QStringLiteral("banned"))) {
        enqueueFetch(FetchOp::RoutingListChats, QString(), Priority::Prefetch);
        enqueueFetch(FetchOp::TransportRooms, e.transport, Priority::Prefetch);
        FetchJob roster;
        roster.op = FetchOp::SessionsQuery;
        roster.priority = Priority::Prefetch;
        roster.reason = nextReason();
        enqueueLaneFetch(CoalesceLane::Roster, QString(), roster);
    }
}

void Ingestor::dispatchPatch(const NodeEvent& e, const PolicyRow& p) {
    if (e.kind == NodeEventKind::DownloadProgress) {
        patchDownload(e);
    } else if (e.kind == NodeEventKind::TransportChanged) {
        patchTransportAccount(e);
    }
    state_.markSyncing(QString::fromLatin1(p.collection));
}

void Ingestor::dispatchKeyed(const NodeEvent& e, const PolicyRow& p) {
    Q_UNUSED(p);
    if (e.convChange == QStringLiteral("removed")) {
        // Removed → tombstone locally; reconciliation covers drift (§5.2 SPEC-DECISION).
        auto b = store_.beginBatch();
        b.tombstone<Conversation>(ConversationKey{e.transport, e.conv},
                                  originFor(QStringLiteral("conversations")), e.originOp);
        b.commit();
    } else {
        // Added → ConvGet(conv) fetch, per-transport-conv lane (200 ms).
        FetchJob job;
        job.op = FetchOp::ConvGet;
        job.scope = e.transport + kSep + e.conv;
        job.priority = observing(QStringLiteral("conversations"), job.scope)
                           ? Priority::VisibleSurface
                           : Priority::Prefetch;
        job.reason = nextReason();
        enqueueLaneFetch(CoalesceLane::PerTransportConv, e.transport, job);
    }
    state_.markSyncing(QStringLiteral("conversations"));
}

void Ingestor::handleResync(const NodeEvent& e) {
    const QStringList collections = SyncState::scopeToCollections(e.scope);
    for (const QString& c : collections) {
        state_.markStale(c);
    }
    runStalenessScan(Priority::Prefetch);
    Q_EMIT resyncScanStarted(collections);
}

// ---------------------------------------------------------------------------
// Enqueue + lane debounce
// ---------------------------------------------------------------------------
void Ingestor::enqueueFetch(FetchOp op, const QString& scope, Priority prio, quint64 afterCursor) {
    FetchJob job;
    job.op = op;
    job.scope = scope;
    job.priority = prio;
    job.reason = nextReason();
    job.afterCursor = afterCursor;
    scheduler_.enqueue(job);
}

QTimer* Ingestor::laneTimer(const QString& laneId, int intervalMs) {
    if (auto it = lane_timers_.constFind(laneId); it != lane_timers_.constEnd()) {
        return it.value();
    }
    auto* t = new QTimer(this);
    t->setSingleShot(true);
    t->setInterval(intervalMs);
    connect(t, &QTimer::timeout, this, [this, laneId] { flushLane(laneId); });
    lane_timers_.insert(laneId, t);
    return t;
}

void Ingestor::enqueueLaneFetch(CoalesceLane lane, const QString& laneScope, const FetchJob& job) {
    QString laneId = laneName(lane);
    if (!laneScope.isEmpty()) {
        laneId += kSep + laneScope;
    }
    lane_pending_[laneId].insert(job.coalesceKey(), job); // dedup within the burst window
    QTimer* t = laneTimer(laneId, laneDebounceMs(lane));
    if (!t->isActive()) {
        t->start();
    }
}

void Ingestor::flushLane(const QString& laneId) {
    const QHash<QString, FetchJob> pending = lane_pending_.take(laneId);
    for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
        scheduler_.enqueue(it.value());
    }
}

// ---------------------------------------------------------------------------
// Reconnect + staleness scan (§5.6/§5.7)
// ---------------------------------------------------------------------------
FetchOp Ingestor::baselineOpFor(const QString& collection) const {
    if (collection == QStringLiteral("sessions"))
        return FetchOp::SessionsQuery;
    if (collection == QStringLiteral("fleet"))
        return FetchOp::Tree;
    if (collection == QStringLiteral("approvals"))
        return FetchOp::ApprovalsPending;
    if (collection == QStringLiteral("models"))
        return FetchOp::ModelCatalog;
    if (collection == QStringLiteral("profiles"))
        return FetchOp::ProfileList;
    if (collection == QStringLiteral("conversations"))
        return FetchOp::ConvList;
    if (collection == QStringLiteral("contacts"))
        return FetchOp::RosterList;
    if (collection == QStringLiteral("persons"))
        return FetchOp::PersonList;
    if (collection == QStringLiteral("notifications"))
        return FetchOp::NotificationList;
    if (collection == QStringLiteral("routing"))
        return FetchOp::RoutingListChats;
    if (collection == QStringLiteral("rooms"))
        return FetchOp::TransportRooms;
    // chat windows fill on demand (§4.6); transports has no single list op here.
    return FetchOp::None;
}

QStringList Ingestor::knownTransports() const {
    QStringList out;
    for (const TransportAccount& t : store_.snapshot().transport_accounts) {
        out << t.transport;
    }
    if (out.isEmpty()) {
        const auto& idx = store_.snapshot().conversationsByTransport;
        for (auto it = idx.begin(); it != idx.end(); ++it) {
            out << it->first;
        }
    }
    out.removeDuplicates();
    out.sort();
    return out;
}

void Ingestor::runStalenessScan(Priority prio) {
    // The 19-call connect-ready storm (07§5.2) is DELETED — replaced by this bounded, deduped,
    // priority-banded scan over ONLY the stale collections (§5.6 step 3). The FetchScheduler caps
    // in-flight jobs; diff-before-write suppresses no-op churn on delivery.
    const QStringList stale = state_.staleCollections();
    for (const QString& collection : stale) {
        const FetchOp op = baselineOpFor(collection);
        if (op == FetchOp::None) {
            continue;
        }
        if (collection == QStringLiteral("conversations") ||
            collection == QStringLiteral("contacts")) {
            const QStringList transports = knownTransports();
            if (transports.isEmpty()) {
                enqueueFetch(op, QString(), prio);
            } else {
                for (const QString& tr : transports) {
                    enqueueFetch(op, tr, prio);
                }
            }
        } else {
            enqueueFetch(op, QString(), prio);
        }
        state_.markSyncing(collection);
    }
}

void Ingestor::onConnected(int apiVersion, bool hasRevFields) {
    apiVersion_ = apiVersion;
    hasRevFields_ = hasRevFields;
    const StampingMode mode = SyncState::selectMode(apiVersion, hasRevFields);
    for (const QString& c : SyncState::allCollections()) {
        state_.setMode(c, mode);
    }
    if (mode == StampingMode::WireDelta) {
        // FULL (§5.6 full step 1): a single Bootstrap probe returns {cursor, epoch, revs}. The
        // executor decodes it and calls onBootstrap(), which issues the per-collection since_rev
        // delta reads (rev == stored ⇒ skip). The probe supersedes the degraded scan — no blind
        // full re-baseline of every collection.
        FetchJob job;
        job.op = FetchOp::Bootstrap;
        job.priority = Priority::Prefetch;
        job.reason = nextReason();
        scheduler_.enqueue(job);
        return;
    }
    // Degraded resume: scan whatever is currently stale (a warm reconnect re-baselines nothing
    // new).
    runStalenessScan(Priority::Prefetch);
}

void Ingestor::onDisconnected() {
    // The feed cursor is retained for the next resume (§5.6 step 1). Nothing to tear down here;
    // the scheduler's in-flight jobs resolve or are abandoned by the bridge.
}

void Ingestor::onSuspectedNodeRestart() {
    for (const QString& c : SyncState::allCollections()) {
        state_.markStale(c);
    }
    runStalenessScan(Priority::Prefetch);
}

void Ingestor::onBootstrap(quint64 cursor, quint64 epoch, const QHash<QString, quint64>& revs) {
    state_.setFeedCursor(cursor);
    const bool restarted = state_.setFeedEpoch(epoch);
    for (const QString& c : SyncState::allCollections()) {
        state_.setMode(c, StampingMode::WireDelta);
    }
    for (auto it = revs.constBegin(); it != revs.constEnd(); ++it) {
        const QString& collection = it.key();
        const quint64 nodeRev = it.value();
        const FetchOp op = baselineOpFor(collection);
        if (op == FetchOp::None) {
            continue;
        }
        if (!restarted && state_.collection(collection).nodeRev == nodeRev) {
            state_.markFresh(collection, nodeRev,
                             nowMs()); // rev == stored ⇒ skip (§5.6 full step 2)
            continue;
        }
        FetchJob job;
        job.op = op;
        job.priority = Priority::Prefetch;
        job.reason = nextReason();
        job.fullMode = true;
        job.sinceRev = restarted ? 0 : state_.collection(collection).nodeRev; // 0 ⇒ full read
        scheduler_.enqueue(job);
        state_.markSyncing(collection);
    }
}

// ---------------------------------------------------------------------------
// Visibility declarations (§5.8)
// ---------------------------------------------------------------------------
void Ingestor::beginObserving(const QString& collection, const QString& scope) {
    observing_.insert(obsKey(collection, scope));
    // A visible chat scope tops up its window FORWARD from the stored per-conv cursor (cold
    // window ⇒ full fill from 0 — §4.6 interim; otherwise a tail read, the §13 M1 cursor fix).
    // The scheduler's (op,scope) dedup absorbs re-opens; delivery advances the cursor.
    if (collection == QStringLiteral("chat") && !scope.isEmpty()) {
        enqueueFetch(FetchOp::ConvHistory, scope, Priority::VisibleSurface,
                     state_.convCursor(scope));
        state_.markSyncing(collection);
        return;
    }
    // The ingestor decides whether observation triggers a fetch — freshness is store-metadata +
    // declared policy, never a lens callsite (00-B1). Fetch if stale, or stale-by-age.
    const SyncState::Collection cs = state_.collection(collection);
    bool needsFetch = (cs.state == CollectionState::Stale);
    if (!needsFetch) {
        const qint64 maxAge = obs_max_age_ms_.value(collection, -1);
        if (maxAge >= 0 && cs.fetchedAtMs > 0 && (nowMs() - cs.fetchedAtMs) > maxAge) {
            needsFetch = true;
        }
    }
    if (needsFetch) {
        const FetchOp op = baselineOpFor(collection);
        if (op != FetchOp::None) {
            enqueueFetch(op, scope, Priority::VisibleSurface);
            state_.markSyncing(collection);
        }
    }
}

void Ingestor::endObserving(const QString& collection, const QString& scope) {
    observing_.remove(obsKey(collection, scope));
}

bool Ingestor::requestOlder(const QString& scopeKey, int count) {
    Q_UNUSED(count); // v38 pages by the wire page bound; BR's before_cursor honors it (§10.2)
    const QStringList sp = scopeKey.split(kSep);
    const ChatMessageScope scope{sp.value(0), sp.value(1)};
    const auto& windows = store_.snapshot().chat;
    const bool windowEmpty = windows.count(scope) == 0U || windows[scope].items.empty();
    if (windowEmpty && state_.convCursor(scopeKey) == 0) {
        // Cold window: the v38 fill is forward from cursor 0 (§4.6 interim).
        enqueueFetch(FetchOp::ConvHistory, scopeKey, Priority::VisibleSurface, 0);
        state_.markSyncing(QStringLiteral("chat"));
        return true;
    }
    // Older-than-window history is not expressible against v38 (no before_cursor until BR): the
    // persisted/filled contiguous tail IS the reachable window; report end-of-history.
    return false;
}

void Ingestor::setObservationMaxAgeMs(const QString& collection, qint64 maxAgeMs) {
    obs_max_age_ms_.insert(collection, maxAgeMs);
}

// ---------------------------------------------------------------------------
// Apply (single-writer path, §5.1) — diff-before-write handled by mirror::Batch (§5.3)
// ---------------------------------------------------------------------------
JournalOrigin Ingestor::originFor(const QString& collection) const {
    return state_.collection(collection).mode == StampingMode::WireDelta
               ? JournalOrigin::WireDelta
               : JournalOrigin::RefetchDiff;
}

void Ingestor::applyConversationFullList(const QString& transport,
                                         const std::vector<Conversation>& items,
                                         const QString& originOp) {
    const JournalOrigin origin = originFor(QStringLiteral("conversations"));
    // Current keys for this transport (replace-and-prune, PageLoop semantics §5.3).
    QSet<QString> present;
    const auto& idx = store_.snapshot().conversationsByTransport;
    if (idx.count(transport)) {
        for (const ConversationKey& k : idx[transport]) {
            present.insert(k.id);
        }
    }
    QSet<QString> incoming;
    auto b = store_.beginBatch();
    for (const Conversation& c : items) {
        incoming.insert(c.id);
        b.upsert(c, origin, originOp);
    }
    // Prune: keys present-but-absent-from-the-page-union stamp tombstones (§5.3/§3 PageLoop).
    for (const QString& id : present) {
        if (!incoming.contains(id)) {
            b.tombstone<Conversation>(ConversationKey{transport, id}, origin);
        }
    }
    b.commit();
}

void Ingestor::deliverConversations(const QString& transport,
                                    const std::vector<Conversation>& items, bool isFinalPage,
                                    const QString& originOp) {
    if (isFinalPage) {
        applyConversationFullList(transport, items, originOp);
    } else {
        auto b = store_.beginBatch();
        for (const Conversation& c : items) {
            b.upsert(c, originFor(QStringLiteral("conversations")), originOp);
        }
        b.commit();
    }
    state_.markFresh(QStringLiteral("conversations"), 0, nowMs());
}

void Ingestor::deliverConversation(const Conversation& conv, const QString& originOp) {
    auto b = store_.beginBatch();
    b.upsert(conv, originFor(QStringLiteral("conversations")), originOp);
    b.commit();
    state_.markFresh(QStringLiteral("conversations"), 0, nowMs());
}

void Ingestor::deliverChatPage(const QString& transport, const QString& conv,
                               const std::vector<ChatMessage>& page, quint64 nextCursor,
                               quint64 headCursor) {
    const QString scope = transport + kSep + conv;
    quint64 maxCursor = state_.convCursor(scope);
    auto b = store_.beginBatch();
    for (const ChatMessage& m : page) {
        b.appendWindow(m, originFor(QStringLiteral("chat")), m.origin_op);
        maxCursor = std::max(maxCursor, m.cursor);
    }
    b.commit();
    // Advance the per-conv cursor to the newest applied (the fix: the next MessagesChanged pages
    // forward from here, never re-fetching the whole transcript).
    state_.setConvCursor(scope, std::max(maxCursor, nextCursor));
    // window_meta.contiguous_to_head bookkeeping (§4.6): the window abuts the node head once the
    // page loop reaches it.
    if (nextCursor >= headCursor && headCursor > 0) {
        state_.markFresh(QStringLiteral("chat"), 0, nowMs());
    }
}

void Ingestor::applyContactFullList(const QString& transport, const std::vector<Contact>& items) {
    const JournalOrigin origin = originFor(QStringLiteral("contacts"));
    QSet<QString> present;
    for (const Contact& c : store_.snapshot().contacts) {
        if (c.transport == transport) {
            present.insert(c.id);
        }
    }
    QSet<QString> incoming;
    auto b = store_.beginBatch();
    for (const Contact& c : items) {
        incoming.insert(c.id);
        b.upsert(c, origin);
    }
    for (const QString& id : present) {
        if (!incoming.contains(id)) {
            b.tombstone<Contact>(ContactKey{transport, id}, origin);
        }
    }
    b.commit();
}

void Ingestor::deliverContacts(const QString& transport, const std::vector<Contact>& items,
                               bool isFinalPage) {
    if (isFinalPage) {
        applyContactFullList(transport, items);
    } else {
        auto b = store_.beginBatch();
        for (const Contact& c : items) {
            b.upsert(c, originFor(QStringLiteral("contacts")));
        }
        b.commit();
    }
    state_.markFresh(QStringLiteral("contacts"), 0, nowMs());
}

void Ingestor::applyPersonFullList(const std::vector<Person>& items) {
    const JournalOrigin origin = originFor(QStringLiteral("persons"));
    QSet<QString> present;
    for (const Person& p : store_.snapshot().persons) {
        present.insert(p.id);
    }
    QSet<QString> incoming;
    auto b = store_.beginBatch();
    for (const Person& p : items) {
        incoming.insert(p.id);
        b.upsert(p, origin);
    }
    for (const QString& id : present) {
        if (!incoming.contains(id)) {
            b.tombstone<Person>(PersonKey{id}, origin);
        }
    }
    b.commit();
}

void Ingestor::deliverPersons(const std::vector<Person>& items, bool isFinalPage) {
    if (isFinalPage) {
        applyPersonFullList(items);
    } else {
        auto b = store_.beginBatch();
        for (const Person& p : items) {
            b.upsert(p, originFor(QStringLiteral("persons")));
        }
        b.commit();
    }
    state_.markFresh(QStringLiteral("persons"), 0, nowMs());
}

void Ingestor::applyRoutePinFullList(const std::vector<RoutePin>& items) {
    const JournalOrigin origin = originFor(QStringLiteral("routing"));
    QSet<QString> present;
    for (const RoutePin& p : store_.snapshot().route_pins) {
        present.insert(p.origin_key);
    }
    QSet<QString> incoming;
    auto b = store_.beginBatch();
    for (const RoutePin& p : items) {
        incoming.insert(p.origin_key);
        b.upsert(p, origin);
    }
    for (const QString& key : present) {
        if (!incoming.contains(key)) {
            b.tombstone<RoutePin>(RoutePinKey{key}, origin);
        }
    }
    b.commit();
}

void Ingestor::deliverRoutePins(const std::vector<RoutePin>& items, bool isFinalPage) {
    if (isFinalPage) {
        applyRoutePinFullList(items);
    } else {
        auto b = store_.beginBatch();
        for (const RoutePin& p : items) {
            b.upsert(p, originFor(QStringLiteral("routing")));
        }
        b.commit();
    }
    state_.markFresh(QStringLiteral("routing"), 0, nowMs());
}

void Ingestor::applyRoomFullList(const QString& transport, const std::vector<Room>& items) {
    const JournalOrigin origin = originFor(QStringLiteral("rooms"));
    QSet<QString> present;
    for (const Room& r : store_.snapshot().rooms) {
        if (r.transport == transport) {
            present.insert(r.room);
        }
    }
    QSet<QString> incoming;
    auto b = store_.beginBatch();
    for (const Room& r : items) {
        incoming.insert(r.room);
        b.upsert(r, origin);
    }
    for (const QString& room : present) {
        if (!incoming.contains(room)) {
            b.tombstone<Room>(RoomKey{transport, room}, origin);
        }
    }
    b.commit();
}

void Ingestor::deliverRooms(const QString& transport, const std::vector<Room>& items,
                            bool isFinalPage) {
    if (isFinalPage) {
        applyRoomFullList(transport, items);
    } else {
        auto b = store_.beginBatch();
        for (const Room& r : items) {
            b.upsert(r, originFor(QStringLiteral("rooms")));
        }
        b.commit();
    }
    state_.markFresh(QStringLiteral("rooms"), 0, nowMs());
}

void Ingestor::refetchRouting() {
    enqueueFetch(FetchOp::RoutingListChats, QString(),
                 observing(QStringLiteral("routing"), QString()) ? Priority::VisibleSurface
                                                                 : Priority::Prefetch);
    state_.markSyncing(QStringLiteral("routing"));
}

void Ingestor::refetchRooms(const QString& transport) {
    enqueueFetch(FetchOp::TransportRooms, transport,
                 observing(QStringLiteral("rooms"), transport) ? Priority::VisibleSurface
                                                               : Priority::Prefetch);
    state_.markSyncing(QStringLiteral("rooms"));
}

// ---------------------------------------------------------------------------
// FULL (wire_delta) delivery (§5.6 full / §10.2): upsert changed + tombstone removed, no prune.
// ---------------------------------------------------------------------------
void Ingestor::deliverConversationsDelta(const QString& transport,
                                         const std::vector<Conversation>& changed,
                                         const QStringList& removed, quint64 rev,
                                         bool isFinalPage) {
    const JournalOrigin origin = originFor(QStringLiteral("conversations"));
    auto b = store_.beginBatch();
    for (const Conversation& c : changed) {
        b.upsert(c, origin);
    }
    // Delta semantics: only the ids the node reports as `removed` are tombstoned — absent keys are
    // UNCHANGED (not pruned). This is the essential difference from the full-list
    // replace-and-prune.
    for (const QString& id : removed) {
        b.tombstone<Conversation>(ConversationKey{transport, id}, origin);
    }
    b.commit();
    if (isFinalPage) {
        state_.markFresh(QStringLiteral("conversations"), rev, nowMs());
    }
}

void Ingestor::deliverContactsDelta(const QString& transport, const std::vector<Contact>& changed,
                                    const QStringList& removed, quint64 rev, bool isFinalPage) {
    const JournalOrigin origin = originFor(QStringLiteral("contacts"));
    auto b = store_.beginBatch();
    for (const Contact& c : changed) {
        b.upsert(c, origin);
    }
    for (const QString& id : removed) {
        b.tombstone<Contact>(ContactKey{transport, id}, origin);
    }
    b.commit();
    if (isFinalPage) {
        state_.markFresh(QStringLiteral("contacts"), rev, nowMs());
    }
}

void Ingestor::deliverPersonsDelta(const std::vector<Person>& changed, const QStringList& removed,
                                   quint64 rev, bool isFinalPage) {
    const JournalOrigin origin = originFor(QStringLiteral("persons"));
    auto b = store_.beginBatch();
    for (const Person& p : changed) {
        b.upsert(p, origin);
    }
    for (const QString& id : removed) {
        b.tombstone<Person>(PersonKey{id}, origin);
    }
    b.commit();
    if (isFinalPage) {
        state_.markFresh(QStringLiteral("persons"), rev, nowMs());
    }
}

void Ingestor::patchTransportAccount(const NodeEvent& e) {
    // Patch-in-place (§5.2): preserve the row's other fields; update only the carried state.
    TransportAccount acct;
    if (const TransportAccount* existing =
            store_.snapshot().transport_accounts.find(TransportAccountKey{e.transport})) {
        acct = *existing;
    } else {
        acct.transport = e.transport;
    }
    acct.connection = e.connection;
    if (e.hasPresence) {
        acct.presence = e.presence;
    }
    if (e.hasReason) {
        acct.reason = e.reason;
    }
    if (e.hasMessage) {
        acct.message = e.message;
    }
    acct.fatal = e.fatal;
    auto b = store_.beginBatch();
    b.upsert(acct, originFor(QStringLiteral("transports")), e.originOp);
    b.commit();
    state_.markFresh(QStringLiteral("transports"), 0, nowMs());
    // On a →Connected transition also refresh that account's ConvList (§5.2).
    if (e.connection == QStringLiteral("connected")) {
        enqueueFetch(FetchOp::ConvList, e.transport, Priority::Prefetch);
    }
}

void Ingestor::patchDownload(const NodeEvent& e) {
    ModelDownload dl;
    dl.id = QString::number(e.downloadId);
    dl.state = e.downloadState;
    dl.downloaded_bytes = e.downloadedBytes;
    dl.total_bytes = e.totalBytes;
    if (const ModelDownload* existing =
            store_.snapshot().model_downloads.find(ModelDownloadKey{dl.id})) {
        dl.files_done = existing->files_done;
        dl.files_total = existing->files_total;
        if (dl.state.isEmpty()) {
            dl.state = existing->state;
        }
    }
    auto b = store_.beginBatch();
    b.upsert(dl, originFor(QStringLiteral("models")), e.originOp);
    b.commit();
}

void Ingestor::fetchCompleted(const FetchJob& job) {
    scheduler_.complete(job.coalesceKey());
}

void Ingestor::fetchFailed(const FetchJob& job, const QString& error) {
    Q_UNUSED(error);
    // Free the slot so the scheduler pumps the next job; the collection stays non-fresh (E5 error
    // surfacing is the bridge's, which knows the op→collection map and calls markError).
    scheduler_.complete(job.coalesceKey());
}

} // namespace mirror
