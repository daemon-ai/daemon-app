#pragma once
// mirror::NodeEvent — the ingestor's decoupled event input (spec 09 §5.1/§5.2). A value copy of
// the node-wide feed notification, independent of the wire codec so the ingestor's policy table,
// scheduler, reconnect, and window logic are unit-testable with synthetic E4 streams (§12) and no
// live transport. The daemon bridge translates the codec's `DecodedNodeEvent` into this shape
// (a pure, tested `translate()`), keeping the codec confined to the bridge (07§10 boundary).
//
// NodeEventKind enumerates the complete §5.2 arm census (16 arms). NotificationsChanged is in the
// census (spec §5.2 / §10.1 rung-1) even though the v38 app codec cannot yet deliver it: a
// declared-but-not-yet-wired arm proves the forward-compatibility property (a new wire arm must
// fail the policy-table exhaustiveness gate until §5.2 declares a row). `Unknown` is the
// no-op + counter sink for a future arm the bridge cannot translate yet.

#include <QString>

namespace mirror {

enum class NodeEventKind {
    Unknown = 0,
    SessionAdvanced,
    SessionMetaChanged,
    RosterChanged,
    FleetChanged,
    ProfilesChanged,
    ApprovalPending,
    DownloadProgress,
    CatalogChanged,
    TransportChanged,
    ConversationsChanged,
    MembershipChanged,
    ContactsChanged,
    NotificationsChanged,
    PersonsChanged,
    MessagesChanged,
    ResyncNeeded,
};

// One decoupled feed event. Only the fields a given arm needs are populated; the policy table
// (policy_table.h) and the ingestor read them per arm.
struct NodeEvent {
    NodeEventKind kind = NodeEventKind::Unknown;

    // Session-scoped arms (SessionAdvanced / SessionMetaChanged / ApprovalPending).
    QString session;
    quint64 epoch = 0;
    quint64 headSeq = 0;
    QString requestId;

    // Coalescing-rev arms (RosterChanged / FleetChanged / ProfilesChanged / *Changed at rung 1).
    quint64 rev = 0;
    bool hasRev = false; // rung-1 rev field present (BR activation seam, §10.1)

    // DownloadProgress (payload-carrying).
    quint64 downloadId = 0;
    quint32 pct = 0;
    QString downloadState;
    quint64 downloadedBytes = 0;
    quint64 totalBytes = 0;

    // Transport / conversation / membership arms.
    QString transport;
    QString connection;
    QString presence;
    bool hasPresence = false;
    QString reason;
    bool hasReason = false;
    QString message;
    bool hasMessage = false;
    bool fatal = false;
    QString conv;
    QString convChange; // "added" | "removed"
    QString member;
    QString membershipChange; // "joined" | "left" | "invited" | "kicked" | "banned"
    bool isSelf = false;

    // ResyncNeeded.
    QString scope;

    // Rung-3 uniform provenance (§10.3): the causing op's client op-id, null (empty) until api/39.
    QString originOp;
};

} // namespace mirror
