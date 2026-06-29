#pragma once

#include <QString>

namespace daemonapp::daemon::cache {

// v5 (Phase 6a): adds daemon_transport_instances + daemon_conversations (offline-first Channels
// read surface - configured accounts + their live conversations render with no connection).
// v4 (Phase 5b): adds daemon_fleet_units (offline-first fleet/subagent tree). The cache is
// non-authoritative, so the bump just drops + rebuilds (the daemon re-baselines it).
// v3 (Phase 4 closeout): retired the dead daemon_session_log + transient daemon_approvals tables
// (the transcript caches in daemon_transcript_blocks; pending approvals are live). daemon_profiles
// became live (offline-first read).
// v2 (L3): added daemon_transcript_blocks (render-from-cache transcript) + per-session resync
// cursors.
inline constexpr int kSchemaVersion = 5;

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

// v4: the offline-first fleet/subagent tree. One row per unit (DaemonFleetTree projects these to
// the IFleetTree rows); parent_id ('' = a root) lets the tree be rebuilt without a connection.
inline constexpr const char* kCreateFleetUnitsSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_fleet_units (
  unit_id TEXT PRIMARY KEY,
  parent_id TEXT,  -- NULL/'' == a root (a null QString binds as NULL, which reads back as "")
  depth INTEGER NOT NULL DEFAULT 0,
  ordinal INTEGER NOT NULL DEFAULT 0,
  name TEXT,
  kind TEXT NOT NULL,
  state TEXT NOT NULL,
  role TEXT,
  profile_ref TEXT,
  session_id TEXT,
  work TEXT,
  updated_at_ms INTEGER NOT NULL
);
)sql";

// v5: the offline-first Channels read surface. One row per configured transport instance (account)
// with its last-known connection/presence, and one row per enumerated conversation, so the channels
// surface renders accounts + rooms with no daemon connection (the daemon re-baselines on connect).
inline constexpr const char* kCreateTransportInstancesSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_transport_instances (
  transport TEXT PRIMARY KEY,
  family TEXT NOT NULL,
  display_name TEXT,
  connection TEXT NOT NULL DEFAULT 'offline',
  presence TEXT NOT NULL DEFAULT 'unknown',
  bound_profile TEXT,
  updated_at_ms INTEGER NOT NULL
);
)sql";

inline constexpr const char* kCreateConversationsSql = R"sql(
CREATE TABLE IF NOT EXISTS daemon_conversations (
  transport TEXT NOT NULL,
  conv_id TEXT NOT NULL,
  kind TEXT NOT NULL,
  title TEXT,
  topic TEXT,
  updated_at_ms INTEGER NOT NULL,
  PRIMARY KEY (transport, conv_id)
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
// daemon_sync_cursors: the applied (epoch, watermark). The node-wide feed cursor uses the fixed
// scope "events-since". Keeping these as cursor rows reuses the existing setCursor/cursor API.
inline QString sessionWatermarkScope(const QString& sessionId) {
    return QStringLiteral("session:%1:watermark").arg(sessionId);
}
inline QString sessionEpochScope(const QString& sessionId) {
    return QStringLiteral("session:%1:epoch").arg(sessionId);
}
inline constexpr const char* kEventsSinceScope = "events-since";

} // namespace daemonapp::daemon::cache
