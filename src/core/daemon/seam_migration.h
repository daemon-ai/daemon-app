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
    {"IFleetTree + ISessionRoster + IDashboard + IApprovalsInbox",
     "Tree / SessionsQuery / ApprovalsPending", kFleetMigration, SeamMigrationStatus::MockOnly},
    {"IDaemonConfig", "ProfileApi / SessionOverlay / node capabilities / ISettingsStore",
     kConfigMigration, SeamMigrationStatus::MockOnly},
    {"IFsService", "FsRoots / FsList / FsRead / FsWrite / FsWatchPoll",
     "LANDED (Phase 4): DaemonFsService implements IFsService over the NodeApi fs_* ops + the "
     "NodeApiCodec facade, with DaemonCacheStore (daemon_fs_entries) as the offline fallback and a "
     "FsWatchPoll cursor loop (reset -> re-list). Wired in daemon mode; mock keeps "
     "LocalDiskFsService.",
     SeamMigrationStatus::DaemonAligned},
};

} // namespace daemonapp::daemon::migration
