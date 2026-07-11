// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

namespace daemonapp::daemon::cache {

// v8 ([waveB:app-v30]): (1) adds connection_reason + connection_message + fatal to
// daemon_transport_instances (D1: the node-reported disconnect reason / human message / fatal flag
// render offline-first, and `fatal` gates the re-auth affordance without a live connection); and
// (2) adds end_reason to daemon_fleet_units (stretch: UnitNode.end_reason is node-reported wire
// state, so the subagent strip's error status renders from it — closing the Wave A gap where
// failedCount was always 0). The cache is non-authoritative, so the bump just drops + rebuilds.
// v7 ([wave2:app-delegation]): adds lifetime + engine_kind + engine_agent to daemon_fleet_units
// (F3: authoritative per-child lifetime/engine chips render offline-first). The cache is
// non-authoritative, so the bump just drops + rebuilds (the daemon re-baselines it). (F1's
// completion-notice renders as a durable System Message block via the existing role/text path — no
// new transcript columns.)
// v6 (D1): adds detail_kind + detail_body to daemon_transcript_blocks so a tool result's rich
// detail (diff / search hits / screenshot path / full output in `summary`) survives restart and
// history scroll-back.
// v5 (Phase 6a): adds daemon_transport_instances + daemon_conversations (offline-first Channels
// read surface - configured accounts + their live conversations render with no connection).
// v4 (Phase 5b): adds daemon_fleet_units (offline-first fleet/subagent tree). The cache is
// non-authoritative, so the bump just drops + rebuilds (the daemon re-baselines it).
// v3 (Phase 4 closeout): retired the dead daemon_session_log + transient daemon_approvals tables
// (the transcript caches in daemon_transcript_blocks; pending approvals are live). daemon_profiles
// became live (offline-first read).
// v2 (L3): added daemon_transcript_blocks (render-from-cache transcript) + per-session resync
// cursors.
// v9 ([acct-mgmt] wire v35): adds enabled + label to daemon_transport_instances (node-persisted
// desired state + display-label override; render offline-first). The cache is non-authoritative, so
// the bump just drops + rebuilds (the daemon re-baselines it).
// v10 ([integrations] wire v38): adds parent to daemon_conversations (the containing space/server-
// level conversation id; NULL/'' = a root) — with kind now carrying "space" container rows, the
// integrations tree's hierarchy renders offline-first. Non-authoritative: drop + rebuild.
inline constexpr int kSchemaVersion = 10;

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

// v4: the offline-first fleet/subagent tree. One row per unit; parent_id ('' = a root) lets the
// tree be rebuilt without a connection. G2 (M5): its LAST reader (DaemonFleetTree) is deleted —
// the fleet TREE renders from the mirror FleetUnit entity; this cache + the FleetRepository tree
// feed remain only as the dual-write baseline and go with the AD deletions.
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
  -- [wave2:app-delegation] v7 (F3): authoritative per-child wire enrichment (UnitNode v29).
  lifetime TEXT,      -- '' | 'Persistent' | 'Ephemeral'
  engine_kind TEXT,   -- '' | 'Core' | 'Foreign'
  engine_agent TEXT,  -- foreign agent name (only when engine_kind == 'Foreign')
  -- [waveB:app-v30] v8 (stretch): UnitNode.end_reason (node-reported terminal reason for a
  -- Finished unit) so the subagent strip renders an error status offline-first.
  end_reason TEXT,
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
  -- [waveB:app-v30] v8 (D1): node-reported disconnect provenance. connection_reason is a coarse
  -- token (see disconnectReasonName); connection_message is the node's human string (rendered
  -- verbatim); fatal gates the re-auth affordance. All empty/0 when the node reported none.
  connection_reason TEXT,
  connection_message TEXT,
  fatal INTEGER NOT NULL DEFAULT 0,
  -- [acct-mgmt] v9 (wire v35): node-persisted desired state (enabled, default true) + display-label
  -- override (label; NULL/empty when the node reported none / an explicit null — nullable like the
  -- connection_reason/message columns, so a cleared QString binds cleanly).
  enabled INTEGER NOT NULL DEFAULT 1,
  label TEXT,
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
  -- [integrations] v10 (wire v38): the containing space/server-level conversation id on the same
  -- transport (NULL/'' = a root; kind 'space' marks the container rows). Nullable like title/topic.
  parent TEXT,
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
  detail_kind TEXT,
  detail_body BLOB,
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
