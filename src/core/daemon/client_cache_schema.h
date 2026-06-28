#pragma once

#include <QString>

namespace daemonapp::daemon::cache {

// v2 (L3): adds daemon_transcript_blocks (render-from-cache transcript) + the per-session resync
// cursor scopes. The cache is non-authoritative, so the bump just drops + rebuilds (the daemon
// re-baselines it).
inline constexpr int kSchemaVersion = 2;

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

// v2: the durable, render-from-cache transcript. One coalesced block per (session, seq) — the same
// shape the journal replays and the live stream coalesces to — so a refocus/cold-start renders the
// conversation from disk and only fetches a short journal/log delta past the persisted watermark.
inline constexpr const char* kCreateTranscriptBlocksSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_transcript_blocks (
  session_id TEXT NOT NULL,
  seq INTEGER NOT NULL,
  kind TEXT NOT NULL,
  role TEXT,
  text TEXT,
  call_id TEXT,
  tool_name TEXT,
  args_summary TEXT,
  ok INTEGER NOT NULL DEFAULT 0,
  summary TEXT,
  request_id INTEGER NOT NULL DEFAULT 0,
  host_kind TEXT,
  content_kind TEXT,
  updated_at_ms INTEGER NOT NULL,
  PRIMARY KEY (session_id, seq)
);
)sql";

// Cursor-scope key builders for the per-session resync coordinates persisted in
// daemon_sync_cursors: the applied (epoch, watermark) and the durable journal cursor. The node-wide
// feed cursor uses the fixed scope "events-since". Keeping these as cursor rows reuses the existing
// setCursor/cursor API.
inline QString sessionWatermarkScope(const QString& sessionId) {
    return QStringLiteral("session:%1:watermark").arg(sessionId);
}
inline QString sessionEpochScope(const QString& sessionId) {
    return QStringLiteral("session:%1:epoch").arg(sessionId);
}
inline QString sessionJournalScope(const QString& sessionId) {
    return QStringLiteral("session:%1:journal").arg(sessionId);
}
inline constexpr const char* kEventsSinceScope = "events-since";

} // namespace daemonapp::daemon::cache
