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
     "IApprovalsInbox LANDED (DaemonApprovalsInbox); roster + dashboard still MockOnly (the "
     "offline-first DaemonSessionRoster/DaemonDashboard are a follow-up over the live "
     "roster/fleet/approvals).",
     SeamMigrationStatus::MockOnly},
    {"IDaemonConfig", "ProfileApi / SessionOverlay / node capabilities / ISettingsStore",
     kConfigMigration, SeamMigrationStatus::MockOnly},
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
