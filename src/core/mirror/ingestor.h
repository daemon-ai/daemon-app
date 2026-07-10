#pragma once
// mirror::Ingestor — the single consumer of the node event stream and the single event-policy
// authority (spec 09 §5). It absorbs the SubscriptionManager's routing role (07§5.1): one place
// decides debounce, patch-vs-refetch, and skip-gates, all read from the §5.2 policy table (DATA).
//
// It is the single Writer of mirrored state (§5.1): every apply funnels through mirror::Store's
// one stamp/commit path. It drives a FetchScheduler (§5.4) over a transport-agnostic
// FetchExecutor seam; the daemon bridge wraps NodeApiClient + the vendored codec + the mappers,
// decodes replies, and feeds the deliver*() methods (already-mapped entities), keeping the codec
// confined to the bridge (07§10). Reconnect (§5.6) runs in both modes; the FULL (wire_delta) path
// is seamed for BR (api/39). ConvHistory is cursor-paged (§13 M1 fix). Chat is a class-W window
// (§4.6). Visibility declarations (§5.8) — never lens callsites — drive freshness.

#include "mirror/fetch_scheduler.h"
#include "mirror/node_event.h"
#include "mirror/store.h"
#include "mirror/sync_state.h"

#include <functional>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QTimer>
#include <vector>

namespace mirror {

class Ingestor : public QObject {
    Q_OBJECT

public:
    Ingestor(Store& store, FetchScheduler& scheduler, QObject* parent = nullptr);

    // The single event entry point (§5.1). Looks up policyFor(kind) and dispatches.
    void ingest(const NodeEvent& event);

    // --- reconnect (§5.6) ---
    // Per-connection mode select + resume. Degraded: scan currently-stale collections. Full:
    // callers use onBootstrap() with the probe result. `hasRevFields` is the BR activation gate.
    void onConnected(int apiVersion, bool hasRevFields);
    void onDisconnected();
    // Suspected node restart at the transport level (v38 cannot distinguish restart from overflow,
    // 06G1 accepted interim): mark ALL collections stale, then scan (§5.6 step 4).
    void onSuspectedNodeRestart();
    // FULL-mode Bootstrap probe result (§5.6 full step 1-2; §10.3). Epoch mismatch ⇒ all node revs
    // unservable (degrade to full reads); else per-collection rev==stored skip, else since_rev.
    void onBootstrap(quint64 cursor, quint64 epoch, const QHash<QString, quint64>& revs);

    // --- visibility declarations (§5.8) — the ingestor decides, lenses only declare ---
    void beginObserving(const QString& collection, const QString& scope = QString());
    void endObserving(const QString& collection, const QString& scope = QString());
    void setObservationMaxAgeMs(const QString& collection, qint64 maxAgeMs);

    // --- demand paging (§4.6): fulfils the WindowModel olderRequested seam ---
    // Returns true when a fill was scheduled: a COLD chat window forward-fills from cursor 0 (the
    // v38 posture — backward windows are not expressible until BR's before_cursor, §10.2). Returns
    // false when no older history is reachable on this connection: the persisted/filled tail IS
    // the window, and the mediator should surface end-of-history (setHasMoreOlder(false)).
    bool requestOlder(const QString& scopeKey, int count);

    // --- delivery of decoded (already-mapped) fetch results (called by the bridge / tests) ---
    // ConvList page for a transport: full-list replace-and-prune (PageLoop semantics, §5.3). The
    // final page passes isFinalPage=true so the prune runs over the accumulated union.
    void deliverConversations(const QString& transport, const std::vector<Conversation>& items,
                              bool isFinalPage, const QString& originOp = QString());
    // ConvGet single conversation (keyed patch): upsert only.
    void deliverConversation(const Conversation& conv, const QString& originOp = QString());
    // ConvHistory forward page (the cursor fix, §13 M1 + windows v1, §4.6): append the tail,
    // advance the per-conv cursor. nextCursor/headCursor drive the page loop (continue until
    // next>=head).
    void deliverChatPage(const QString& transport, const QString& conv,
                         const std::vector<ChatMessage>& page, quint64 nextCursor,
                         quint64 headCursor);
    void deliverContacts(const QString& transport, const std::vector<Contact>& items,
                         bool isFinalPage);
    void deliverPersons(const std::vector<Person>& items, bool isFinalPage);

    // Report a fetch job finished so the scheduler frees the slot (the executor also may call the
    // scheduler directly; this is the ingestor-side convenience used by the bridge on decode).
    void fetchCompleted(const FetchJob& job);
    void fetchFailed(const FetchJob& job, const QString& error);

    // --- test / E5 accessors ---
    [[nodiscard]] SyncState& syncState() noexcept { return state_; }
    [[nodiscard]] const SyncState& syncState() const noexcept { return state_; }
    [[nodiscard]] int unknownArmCount() const noexcept { return unknown_arm_count_; }
    void setClock(std::function<qint64()> clock) { clock_ = std::move(clock); }
    [[nodiscard]] qint64 nowMs() const;

Q_SIGNALS:
    // SessionAdvanced: nudge the focused engine for this session (never a fetch, §5.2). The bridge
    // connects this to the per-tab turn engine (the old registerFocus routing).
    void nudgeFocused(const QString& session);
    // A ResyncNeeded arrived and the scan was kicked (§5.7) — observable for tests / E5.
    void resyncScanStarted(const QStringList& collections);

private:
    // policy dispatch
    void dispatchFetch(const NodeEvent& e, const PolicyRow& p);
    void dispatchPatch(const NodeEvent& e, const PolicyRow& p);
    void dispatchKeyed(const NodeEvent& e, const PolicyRow& p);
    void handleResync(const NodeEvent& e);

    // enqueue helpers
    void enqueueFetch(FetchOp op, const QString& scope, Priority prio, quint64 afterCursor = 0);
    void enqueueLaneFetch(CoalesceLane lane, const QString& laneScope, const FetchJob& job);
    void flushLane(const QString& laneId);
    [[nodiscard]] QTimer* laneTimer(const QString& laneId, int intervalMs);
    [[nodiscard]] QString fetchScopeFor(const NodeEvent& e) const;
    [[nodiscard]] bool observing(const QString& collection, const QString& scope) const;

    // reconnect / scan
    void runStalenessScan(Priority prio);
    [[nodiscard]] FetchOp baselineOpFor(const QString& collection) const;
    [[nodiscard]] QStringList knownTransports() const;

    // apply helpers (single-writer path)
    void applyConversationFullList(const QString& transport, const std::vector<Conversation>& items,
                                   const QString& originOp);
    void applyContactFullList(const QString& transport, const std::vector<Contact>& items);
    void applyPersonFullList(const std::vector<Person>& items);
    void patchTransportAccount(const NodeEvent& e);
    void patchDownload(const NodeEvent& e);

    [[nodiscard]] quint64 nextReason() noexcept { return ++reason_counter_; }
    [[nodiscard]] JournalOrigin originFor(const QString& collection) const;

    Store& store_;
    FetchScheduler& scheduler_;
    SyncState state_;

    // debounce lanes (§5.2): each lane id (name, or name␟transport for per-transport lanes) holds
    // pending distinct fetches, flushed once per burst window (burst compression, §5.4).
    QHash<QString, QTimer*> lane_timers_;
    QHash<QString, QHash<QString, FetchJob>> lane_pending_; // laneId -> coalesceKey -> job

    // visibility (§5.8)
    QSet<QString> observing_;               // "collection␟scope"
    QHash<QString, qint64> obs_max_age_ms_; // collection -> max age while observed

    int apiVersion_ = 38;
    bool hasRevFields_ = false;
    int unknown_arm_count_ = 0;
    quint64 reason_counter_ = 0;
    std::function<qint64()> clock_;
};

} // namespace mirror
