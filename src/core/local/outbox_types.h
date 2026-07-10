// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QString>

// Value types shared by the durable outbox (spec 09 §6) and the local-<id>.db module. Kept
// GUI-free and dependency-light so the store/ingestor/lens layers and the TUI can all consume
// them. Namespace `mirror` matches the spec's data-layer namespace (09 §2).
namespace mirror {

// Persisted op lifecycle (spec 09 §6.1 `outbox_ops.state`). The integer values are ON-DISK and
// MUST stay stable across migrations.
enum class OpState : int {
    Pending = 0,  // durable, not yet sent (or re-queued after a transient failure)
    Inflight = 1, // handed to the wire; on boot these revert to Pending (§6.6)
    Rejected = 2, // node rejection (§6.5): lane pauses, awaits user action
    Accepted = 3, // ack seen, awaiting a provenance-stamped delta on a provenance node (§6.6)
};

// Per-verb lane class (spec 09 §6.4). `Unknown` = a verb with no declared class; the caller must
// route it direct (§7), never silently outbox it.
enum class VerbClass : int {
    ChatSend = 0,    // ConvSend
    ConvMeta = 1,    // ConvSetTopic / ConvSetTitle / ConvSetDescription (latest-wins coalesce)
    RosterEdit = 2,  // RosterAdd / RosterUpdate / RosterRemove / ContactSetAlias
    SessionMeta = 3, // SessionUpdateMeta
    TurnPrompt = 4,  // composer prompts (drain -> the Submit path)
    Unknown = 5,
};

// Lane kind (spec 09 §6.6): all §6.4 wire-verb lanes are mutation lanes (confirmed by a
// provenance-stamped read); the turn-prompt lane is a dispatch lane (consumed at drain into the
// session-turn path, not awaiting read-path visibility).
enum class LaneKind : int { Mutation = 0, Dispatch = 1 };

// api-error disposition (spec 09 §6.5). `Unauthenticated` pauses all wire lanes until re-auth (it
// is NOT a per-op rejection); transient errors back off keeping order; everything else is a
// terminal rejection (fail visible).
enum class ErrorClass : int { Transient = 0, Rejection = 1, Unauthenticated = 2 };

// One durable pending-op row (spec 09 §6.1). `payload` is the canonical CBOR of the request args
// (op_id included); `display` is the human-readable pending-UI summary.
struct PendingOp {
    QString opId;       // client-minted UUIDv7 (§6.2), primary key
    QString lane;       // '<class>\x1f<scope>'
    QString verb;       // wire request name or 'TurnPrompt'
    QByteArray payload; // canonical request-args bytes
    QString display;    // pending-UI summary
    OpState state = OpState::Pending;
    int attempts = 0;
    qint64 enqueuedMs = 0;
    QString lastError; // api-error kind + message on rejection
    int schemaV = 0;
};

} // namespace mirror

Q_DECLARE_METATYPE(mirror::PendingOp)
