#pragma once

namespace daemonapp::daemon::cache {

inline constexpr int kSchemaVersion = 1;

inline constexpr const char* kCreateMetaSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_cache_meta (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
)sql";

inline constexpr const char* kCreateSessionsSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_sessions (
  session_id TEXT PRIMARY KEY,
  title TEXT,
  state TEXT NOT NULL,
  profile_ref TEXT,
  lifecycle TEXT,
  role TEXT,
  parent_session_id TEXT,
  pinned INTEGER NOT NULL DEFAULT 0,
  archived INTEGER NOT NULL DEFAULT 0,
  updated_at_ms INTEGER NOT NULL
);
)sql";

inline constexpr const char* kCreateSessionLogSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_session_log (
  session_id TEXT NOT NULL,
  seq INTEGER NOT NULL,
  payload_cbor BLOB NOT NULL,
  direction TEXT NOT NULL,
  disposition TEXT NOT NULL,
  updated_at_ms INTEGER NOT NULL,
  PRIMARY KEY (session_id, seq)
);
)sql";

inline constexpr const char* kCreateSyncCursorsSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_sync_cursors (
  scope TEXT PRIMARY KEY,
  cursor TEXT NOT NULL,
  updated_at_ms INTEGER NOT NULL
);
)sql";

inline constexpr const char* kCreateProfilesSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_profiles (
  profile_ref TEXT PRIMARY KEY,
  display_name TEXT,
  spec_cbor BLOB NOT NULL,
  active INTEGER NOT NULL DEFAULT 0,
  updated_at_ms INTEGER NOT NULL
);
)sql";

inline constexpr const char* kCreateApprovalsSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_approvals (
  session_id TEXT NOT NULL,
  request_id TEXT NOT NULL,
  prompt TEXT NOT NULL,
  path TEXT,
  updated_at_ms INTEGER NOT NULL,
  PRIMARY KEY (session_id, request_id)
);
)sql";

inline constexpr const char* kCreateFsEntriesSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_fs_entries (
  root_id TEXT NOT NULL,
  path TEXT NOT NULL,
  kind TEXT NOT NULL,
  size INTEGER NOT NULL,
  mtime_ms INTEGER NOT NULL,
  revision_cbor BLOB,
  updated_at_ms INTEGER NOT NULL,
  PRIMARY KEY (root_id, path)
);
)sql";

} // namespace daemonapp::daemon::cache
