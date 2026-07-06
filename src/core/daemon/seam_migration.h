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

// Phase 5 DECISION (node-blocked): routing, cron, and checkpoint timelines have NO NodeApi wire op
// in the daemon-api codec subset the client speaks - the node does not yet expose them. So they
// stay MockOnly, BUT in daemon mode they are EMPTY-SEEDED (createAppServiceGraph passes
// seedDemo=false), rendering an empty/unavailable surface rather than passing the mocks'
// illustrative demo rows off as node-backed data. Promoting any of them to DaemonAligned requires a
// daemon-node contract addition (a new ApiRequest + codec regen) - out of the daemon-app track's
// scope. This mirrors the IDaemonConfig Phase-4 DECISION: a node-unsupported seam stays mock but
// never masquerades as authority.
inline constexpr const char* kRoutingMigration =
    "Node-blocked: no routing/model-selection wire op exists. MockOnly + empty-seeded in daemon "
    "mode. Promote only when daemon-node exposes a routing ApiRequest.";

inline constexpr const char* kCronMigration =
    "Node-blocked: no cron/schedule wire op exists. MockOnly + empty-seeded in daemon mode (the "
    "on-disk cache still restores any user-created local jobs). Promote only when daemon-node "
    "exposes a cron ApiRequest.";

inline constexpr const char* kCheckpointMigration =
    "Node-blocked: checkpoint timelines are not modeled in the daemon-api codec subset "
    "(CheckpointRepository is a stub). MockOnly + empty-seeded in daemon mode. Promote only when "
    "daemon-node exposes a checkpoint/journal-rewind ApiRequest.";

inline constexpr SeamMigrationTarget kTargets[] = {
    {"ISessionStore", "SessionsQuery / Subscribe / SessionLogEntry", kSessionIdMigration,
     SeamMigrationStatus::AdditiveIdReady},
    {"ISessionSettings", "SessionOverlay / ApprovalMode", kSessionSettingsMigration,
     SeamMigrationStatus::MockOnly},
    {"IAccountsService", "CredentialApi / AuthApi", kAccountsMigration,
     SeamMigrationStatus::MockOnly},
    {"IFleetTree", "Tree / Unit / Pause / Resume / Scale",
     "LANDED (Phase 5b): DaemonFleetTree projects the cached Tree query (daemon_fleet_units, "
     "offline-first) into the fleet rows; pause/resume/scale issue the wire control op (PRO-10 "
     "control is meaningful only on orchestrator units - engine leaves return Unsupported).",
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
    {"IRoutingStore", kRoutingMigration, kRoutingMigration, SeamMigrationStatus::MockOnly},
    {"ICronStore", kCronMigration, kCronMigration, SeamMigrationStatus::MockOnly},
    {"ICheckpointTimeline", kCheckpointMigration, kCheckpointMigration,
     SeamMigrationStatus::MockOnly},
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
