// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

namespace daemonapp::daemon::migration {

enum class SeamMigrationStatus {
    MockOnly,
    AdditiveIdReady,
    RepositoryBacked,
    DaemonAligned,
};

struct SeamMigrationTarget {
    const char* seam;
    const char* daemonSource;
    const char* target;
    SeamMigrationStatus status;
};

// These constants intentionally live near the daemon integration code so GUI/TUI work can migrate
// seams in small, reviewable steps before real daemon data is enabled.

inline constexpr const char* kSessionIdMigration =
    "Replace daemon session int ids in service seams with domain::SessionId; keep ints only for "
    "local UI handles such as tab ids.";

inline constexpr const char* kSessionSettingsMigration =
    "Align ISessionSettings with SessionOverlay and ApprovalMode; keep effort/fast/verbose local "
    "or "
    "remove them where they have no daemon backing.";

inline constexpr const char* kAccountsMigration =
    "Reshape IAccountsService around profile-keyed CredentialApi entries and AuthApi flows rather "
    "than provider-primary mock account rows.";

inline constexpr const char* kFleetMigration =
    "Single-source dashboard, fleet tree, and session roster from SessionRepository/ControlApi "
    "snapshots instead of parallel mock datasets.";

inline constexpr const char* kConfigMigration =
    "Replace flat IDaemonConfig writes with typed facades over profiles, session overlays, node "
    "capabilities, and client-local QSettings. Phase 4 DECISION: do NOT wire the deprecated "
    "ConfigGet/ConfigSet ops (wire v9 collapsed runtime config into "
    "ProfileUpdate/SetSessionOverlay; "
    "the daemon trait still returns Unsupported). IDaemonConfig stays MockOnly until its keys "
    "migrate "
    "to those typed homes; it is not a NodeApi-config seam.";

// Phase 5 DECISION (node-blocked), REVISED at wire v28: cron still has NO NodeApi wire op in the
// codec subset the client speaks, so it stays MockOnly + EMPTY-SEEDED in daemon mode
// (createAppServiceGraph passes seedDemo=false), rendering an empty/unavailable surface rather
// than passing the mock's illustrative demo rows off as node-backed data. Checkpoints and routing
// left this bucket when their wire ops landed (CheckpointList/CheckpointRewind, Routing*); see
// their entries below. The legacy intent->model IRoutingStore seam (never node-backed) was
// DELETED with the routing landing — the routing surface is the origin->session pin table.
inline constexpr const char* kCronMigration =
    "Node-blocked: no cron/schedule wire op exists. MockOnly + empty-seeded in daemon mode (the "
    "on-disk cache still restores any user-created local jobs). Promote only when daemon-node "
    "exposes a cron ApiRequest.";

inline constexpr SeamMigrationTarget kTargets[] = {
    {"ISessionStore", "SessionsQuery / Subscribe / SessionLogEntry", kSessionIdMigration,
     SeamMigrationStatus::AdditiveIdReady},
    {"ISessionSettings", "SessionOverlay / ApprovalMode", kSessionSettingsMigration,
     SeamMigrationStatus::MockOnly},
    {"IAccountsService", "CredentialApi / AuthApi", kAccountsMigration,
     SeamMigrationStatus::MockOnly},
    {"IFleetTree", "Tree / Unit / Pause / Resume / Scale",
     "MIRRORED (G2, M5): MirrorFleetTree projects the ONE generated mirror FleetUnit entity "
     "(child_ids edge -> pre-order depth; engine/lifetime/role off the wire unit-node) into the "
     "fleet rows, re-derived on FleetUnit journal deltas (connect-gated like the mirror roster - "
     "live supervision state is not persisted); pause/resume/scale issue the wire control op via "
     "FleetRepository (PRO-10 control is meaningful only on orchestrator units - engine leaves "
     "return Unsupported).",
     SeamMigrationStatus::DaemonAligned},
    {"ISessionRoster + IDashboard + IApprovalsInbox", "SessionsQuery / Tree / ApprovalsPending",
     "LANDED (Phase 5): DaemonApprovalsInbox + DaemonSessionRoster (offline-first over the "
     "CachedSessionStore projection) + DaemonDashboard (counters derived from roster/fleet/"
     "approvals, health from the connection). Wired in daemon mode. Degraded (presentation only, "
     "not domain re-derivation): suspend/resume is a client-local cosmetic overlay and "
     "tokens/rewindable are placeholders - no session-lifecycle / per-session-token wire op yet.",
     SeamMigrationStatus::DaemonAligned},
    {"IDaemonConfig", "ProfileApi / SessionOverlay / node capabilities / ISettingsStore",
     kConfigMigration, SeamMigrationStatus::MockOnly},
    {"Routing (routing slice)",
     "RoutingListChats / RoutingBindChat / RoutingUnbindChat / RoutingSet / TransportRooms",
     "MIRRORED (M3, wire v28): the routing pin table lives on the mirror store (RoutingListChats "
     "-> "
     "route_pins; TransportRooms -> rooms), read by RoutingManagerController; RoutingRepository is "
     "the node-authoritative DIRECT mutation seam (RoutingBindChat/Unbind). Honest residuals: "
     "resolve() answers Pin (a pinned origin) else Default (no read op for the lower precedence "
     "rungs), binding rules are empty (config-time, no wire read), and the delivery/handover panel "
     "stays inert until the SessionGet delivery seam / handover re-point land.",
     SeamMigrationStatus::DaemonAligned},
    {"ICronStore", kCronMigration, kCronMigration, SeamMigrationStatus::MockOnly},
    {"ICheckpointTimeline", "CheckpointList / CheckpointRewind",
     "LANDED (E4/TOOL-9, wire v28): DaemonCheckpointTimeline projects CheckpointRepository's "
     "CheckpointList pages into the popover + timeline-strip rows; restore() issues "
     "CheckpointRewind (UI confirms first). Client-initiated creates stay unsupported (checkpoints "
     "are node-created on tool events); foreign/ACP sessions hide rewind (C4 honesty).",
     SeamMigrationStatus::DaemonAligned},
    {"IFsService", "FsRoots / FsList / FsRead / FsWrite / FsWatchPoll",
     "LANDED (Phase 4): DaemonFsService implements IFsService over the NodeApi fs_* ops + the "
     "NodeApiCodec facade, with DaemonCacheStore (daemon_fs_entries) as the offline fallback and a "
     "FsWatchPoll cursor loop (reset -> re-list). Wired in daemon mode; mock keeps "
     "LocalDiskFsService.",
     SeamMigrationStatus::DaemonAligned},
    {"ITransportRegistry + IPresenceService", "TransportAdapters / TransportInstances / ConvList",
     "LANDED (Phase 6a, story 04): DaemonTransportRegistry projects the node's TransportAdapters "
     "(the 'Add channel' picker) + the offline-first daemon_transport_instances cache; "
     "DaemonPresenceService derives the EIO-9 status dots from the same instances (connection is "
     "coarse + poll/refresh today - real live presence is a deferred backend follow-up). ConvList "
     "(EIO-8) enumerates live rooms per account. Read-only: connect (EIO-2)/disconnect (EIO-7) "
     "deferred.",
     SeamMigrationStatus::DaemonAligned},
};

} // namespace daemonapp::daemon::migration
