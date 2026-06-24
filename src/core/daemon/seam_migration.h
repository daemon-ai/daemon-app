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
    "Align ISessionSettings with SessionOverlay and ApprovalMode; keep effort/fast/verbose local or "
    "remove them where they have no daemon backing.";

inline constexpr const char* kAccountsMigration =
    "Reshape IAccountsService around profile-keyed CredentialApi entries and AuthApi flows rather "
    "than provider-primary mock account rows.";

inline constexpr const char* kFleetMigration =
    "Single-source dashboard, fleet tree, and session roster from SessionRepository/ControlApi "
    "snapshots instead of parallel mock datasets.";

inline constexpr const char* kConfigMigration =
    "Replace flat IDaemonConfig writes with typed facades over profiles, session overlays, node "
    "capabilities, and client-local QSettings.";

inline constexpr SeamMigrationTarget kTargets[] = {
    { "ISessionStore", "SessionsQuery / Subscribe / SessionLogEntry", kSessionIdMigration,
      SeamMigrationStatus::AdditiveIdReady },
    { "ISessionSettings", "SessionOverlay / ApprovalMode", kSessionSettingsMigration,
      SeamMigrationStatus::MockOnly },
    { "IAccountsService", "CredentialApi / AuthApi", kAccountsMigration,
      SeamMigrationStatus::MockOnly },
    { "IFleetTree + ISessionRoster + IDashboard + IApprovalsInbox",
      "Tree / SessionsQuery / ApprovalsPending", kFleetMigration,
      SeamMigrationStatus::MockOnly },
    { "IDaemonConfig", "ProfileApi / SessionOverlay / node capabilities / ISettingsStore",
      kConfigMigration, SeamMigrationStatus::MockOnly },
    { "IFsService", "FsRoots / FsList / FsRead / FsWrite / FsWatchPoll",
      "Implement daemon-backed IFsService using NodeApi fs calls with DaemonCacheStore as offline fallback.",
      SeamMigrationStatus::MockOnly },
};

} // namespace daemonapp::daemon::migration
