// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

namespace daemonapp::daemon::cache {

enum class PersistenceOwner {
    ClientLocalSettings,
    MockModeOnly,
    DaemonAuthoritativeCache,
    ImportOnlyLegacy,
};

struct PersistencePolicy {
    const char* store;
    PersistenceOwner owner;
    const char* policy;
};

inline constexpr PersistencePolicy kPersistencePolicies[] = {
    {
        "QSettings",
        PersistenceOwner::ClientLocalSettings,
        "Keep for client-local preferences only: theme, layout, setupComplete, last connection.",
    },
    {
        "AppDataLocation/mock/*.json",
        PersistenceOwner::MockModeOnly,
        "Use only when ServiceMode::Mock is active; do not write daemon-authoritative domains here "
        "in daemon mode.",
    },
    {
        "daemon_cache.db",
        PersistenceOwner::DaemonAuthoritativeCache,
        "Primary last-known cache for daemon sessions, logs, cursors, profiles, approvals, and "
        "filesystem metadata.",
    },
    {
        "sessions.db",
        PersistenceOwner::ImportOnlyLegacy,
        "Retain for existing mock/local sessions and future one-time import; do not dual-write "
        "daemon sessions long term.",
    },
};

} // namespace daemonapp::daemon::cache
