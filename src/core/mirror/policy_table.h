#pragma once
// mirror::policy — the event→action policy table (spec 09 §5.2). The COMPLETE v38 arm census
// (16 NodeEvent arms) with its declared policy expressed as DATA: the interim/full action, the
// fetch op, the coalesce lane + debounce, and skip-gates are columns, not callsite folklore
// (07§9.9 "per-arm policy folklore" deleted). This table IS the policy (E3).
//
// Exhaustiveness is enforced at COMPILE time: `policyFor()` is a switch over the whole
// NodeEventKind enum with NO default, so adding an arm without declaring a row fails
// -Werror=switch (spec §12 "a new wire arm fails compilation until §5.2 declares a row"). The
// daemon bridge's DecodedNodeEvent→NodeEventKind switch is the second compile-time layer tying
// the app's generated wire enum to this table. A runtime test also asserts a row for every arm.

#include "mirror/node_event.h"

#include <QString>
#include <QVector>

namespace mirror {

// Wire read op a fetch arm issues. Degraded (api/38) and full (api/39) modes issue the same op;
// full mode adds `since_rev`/`removed` (the BR activation seam, §5.6/§10.2) — recorded here so a
// collection can be switched per §5.5 without touching the table.
enum class FetchOp : quint8 {
    None = 0,
    SessionsQuery,
    SessionGet,
    Tree,
    ProfileList,
    ApprovalsPending,
    ModelCatalog,
    ModelDownloads,
    ConvList,
    ConvGet,
    ConvHistory,
    RosterList,
    PersonList,
    NotificationList,
    RoutingListChats,
    TransportRooms,
    Bootstrap,
};

// The primary action classification for an arm (§5.2 "Action (interim)" column, coarsened to the
// mechanism the ingestor executes; runtime event fields drive the small conditional branches —
// e.g. TransportChanged's connected→ConvList, MembershipChanged's self-removal,
// ConversationsChanged added-vs-removed — which are data-in-the-event, not per-callsite policy).
enum class Action : quint8 {
    NudgeOnly,     // never a fetch by itself (SessionAdvanced)
    Fetch,         // enqueue `fetchOp` for the arm's scope
    PatchInPlace,  // payload-carrying event patches a row directly (DownloadProgress,
                   // TransportChanged)
    KeyedPatch,    // keyed add/remove (ConversationsChanged: Added→ConvGet, Removed→tombstone)
    MarkStaleScan, // ResyncNeeded: scope→collections→mark stale→staleness scan
};

// The coalesce lane an arm debounces on (§5.2 "Coalesce" column). Per-transport lanes are
// parameterized by the event's transport; the ingestor forms the concrete debounce key.
enum class CoalesceLane : quint8 {
    None,
    Roster,             // 300 ms
    Fleet,              // 300 ms
    PerTransportConv,   // 200 ms per transport
    PerTransportContact // 200 ms per transport
};

// One declared policy row. Value type; one per NodeEventKind.
struct PolicyRow {
    NodeEventKind kind = NodeEventKind::Unknown;
    Action action = Action::Fetch;
    FetchOp fetchOp = FetchOp::None;     // interim (api/38) fetch op
    FetchOp fullFetchOp = FetchOp::None; // full (api/39) — same op, delta-capable (BR seam)
    CoalesceLane lane = CoalesceLane::None;
    int debounceMs = 0;
    bool skipIfRevUnchanged = false; // rung-1 skip-gate: rev == stored ⇒ no fetch (§5.2 "full")
    const char* collection = "";     // the sync-state / staleness collection this arm concerns
};

// The debounce interval for a lane (§5.2 SPEC-DECISION defaults; tunable, §15).
[[nodiscard]] constexpr int laneDebounceMs(CoalesceLane lane) noexcept {
    switch (lane) {
    case CoalesceLane::None:
        return 0;
    case CoalesceLane::Roster:
    case CoalesceLane::Fleet:
        return 300;
    case CoalesceLane::PerTransportConv:
    case CoalesceLane::PerTransportContact:
        return 200;
    }
    return 0;
}

// The declared policy for an arm. Switch with NO default ⇒ compile-time exhaustiveness (§12).
[[nodiscard]] constexpr PolicyRow policyFor(NodeEventKind kind) noexcept {
    switch (kind) {
    case NodeEventKind::Unknown:
        // Forward compatibility: no-op + counter (the bridge cannot translate the arm yet).
        return {kind, Action::NudgeOnly, FetchOp::None, FetchOp::None, CoalesceLane::None, 0, false,
                ""};
    case NodeEventKind::SessionAdvanced:
        // Never a fetch by itself: nudge the focused engine + staleness/unread candidate.
        return {kind,  Action::NudgeOnly, FetchOp::None, FetchOp::None, CoalesceLane::None, 0,
                false, "sessions"};
    case NodeEventKind::SessionMetaChanged:
        // SessionGet(session) if hydrated; roster lane. Debounced with the roster (300 ms).
        return {kind, Action::Fetch, FetchOp::SessionGet, FetchOp::SessionGet, CoalesceLane::Roster,
                300,  true,          "sessions"};
    case NodeEventKind::RosterChanged:
        // SessionsQuery(since_rev) — native delta at full; roster lane 300 ms; rev → node_revs.
        return {kind,
                Action::Fetch,
                FetchOp::SessionsQuery,
                FetchOp::SessionsQuery,
                CoalesceLane::Roster,
                300,
                false,
                "sessions"};
    case NodeEventKind::FleetChanged:
        // Tree page loop; skip if rev == stored (full); fleet lane 300 ms.
        return {kind, Action::Fetch, FetchOp::Tree, FetchOp::Tree, CoalesceLane::Fleet,
                300,  true,          "fleet"};
    case NodeEventKind::ProfilesChanged:
        // ProfileList; rev-gated at full; small list, no coalesce.
        return {
            kind, Action::Fetch, FetchOp::ProfileList, FetchOp::ProfileList, CoalesceLane::None, 0,
            true, "profiles"};
    case NodeEventKind::ApprovalPending:
        // ApprovalsPending refetch; class L, freshness-critical; immediate.
        return {kind,
                Action::Fetch,
                FetchOp::ApprovalsPending,
                FetchOp::ApprovalsPending,
                CoalesceLane::None,
                0,
                false,
                "approvals"};
    case NodeEventKind::DownloadProgress:
        // Patch-in-place ModelDownload from the payload; never a fetch.
        return {kind,
                Action::PatchInPlace,
                FetchOp::None,
                FetchOp::None,
                CoalesceLane::None,
                0,
                false,
                "models"};
    case NodeEventKind::CatalogChanged:
        // ModelCatalog refetch; rev-gated at full.
        return {kind,
                Action::Fetch,
                FetchOp::ModelCatalog,
                FetchOp::ModelCatalog,
                CoalesceLane::None,
                0,
                true,
                "models"};
    case NodeEventKind::TransportChanged:
        // Patch-in-place TransportAccount; if →Connected also ConvList(transport) (runtime branch).
        return {kind,
                Action::PatchInPlace,
                FetchOp::ConvList,
                FetchOp::ConvList,
                CoalesceLane::None,
                0,
                false,
                "transports"};
    case NodeEventKind::ConversationsChanged:
        // Added→ConvGet(conv) fetch; Removed→tombstone locally. 200 ms per transport.
        return {kind,
                Action::KeyedPatch,
                FetchOp::ConvGet,
                FetchOp::ConvGet,
                CoalesceLane::PerTransportConv,
                200,
                false,
                "conversations"};
    case NodeEventKind::MembershipChanged:
        // ConvGet(conv); if self Left/Kicked/Banned: RoutingListChats + TransportRooms + roster.
        return {kind, Action::Fetch, FetchOp::ConvGet, FetchOp::ConvGet, CoalesceLane::None,
                0,    false,         "conversations"};
    case NodeEventKind::ContactsChanged:
        // RosterList(transport); since_rev at full; 200 ms per transport.
        return {kind,
                Action::Fetch,
                FetchOp::RosterList,
                FetchOp::RosterList,
                CoalesceLane::PerTransportContact,
                200,
                false,
                "contacts"};
    case NodeEventKind::NotificationsChanged:
        // NotificationList; rev-gated at full; small list (census/forward-compat arm, §10.1).
        return {kind,
                Action::Fetch,
                FetchOp::NotificationList,
                FetchOp::NotificationList,
                CoalesceLane::None,
                0,
                true,
                "notifications"};
    case NodeEventKind::PersonsChanged:
        // PersonList; since_rev at full; 200 ms.
        return {
            kind, Action::Fetch, FetchOp::PersonList, FetchOp::PersonList, CoalesceLane::None, 200,
            true, "persons"};
    case NodeEventKind::MessagesChanged:
        // ConvHistory(after_cursor = stored per-conv cursor) — the rung-0 cursor fix (§13 M1).
        return {
            kind,  Action::Fetch, FetchOp::ConvHistory, FetchOp::ConvHistory, CoalesceLane::None, 0,
            false, "chat"};
    case NodeEventKind::ResyncNeeded:
        // Mark scope stale → staleness scan (never a blind storm, §5.7).
        return {
            kind, Action::MarkStaleScan, FetchOp::None, FetchOp::None, CoalesceLane::None, 0, false,
            ""};
    }
    // Unreachable: the switch is exhaustive over NodeEventKind (compile-time gate, §12).
    return {kind, Action::NudgeOnly, FetchOp::None, FetchOp::None, CoalesceLane::None, 0, false,
            ""};
}

// Iterable arm census for the runtime exhaustiveness test (§12) and reconnect scans. Kept in
// sync with NodeEventKind by the same compile-time switch (a new arm without a row won't compile).
inline const QVector<NodeEventKind>& allNodeEventKinds() {
    static const QVector<NodeEventKind> kinds = {
        NodeEventKind::Unknown,
        NodeEventKind::SessionAdvanced,
        NodeEventKind::SessionMetaChanged,
        NodeEventKind::RosterChanged,
        NodeEventKind::FleetChanged,
        NodeEventKind::ProfilesChanged,
        NodeEventKind::ApprovalPending,
        NodeEventKind::DownloadProgress,
        NodeEventKind::CatalogChanged,
        NodeEventKind::TransportChanged,
        NodeEventKind::ConversationsChanged,
        NodeEventKind::MembershipChanged,
        NodeEventKind::ContactsChanged,
        NodeEventKind::NotificationsChanged,
        NodeEventKind::PersonsChanged,
        NodeEventKind::MessagesChanged,
        NodeEventKind::ResyncNeeded,
    };
    return kinds;
}

} // namespace mirror
