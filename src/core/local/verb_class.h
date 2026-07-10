// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "outbox_types.h"

#include <QString>

// The per-verb-class table (spec 09 §6.4) + the lane/gate/error derivations it drives. This is the
// one place that answers "which verbs are outboxed and how" — every other module reads it, never
// re-decides (mirrors the spec's guardrail against per-verb confirmation folklore, §14.13).
namespace mirror {

// Classify a wire verb name into its lane class (§6.4). Returns `Unknown` for any verb not on the
// table — the caller routes those direct (§7), never silently outboxes them.
[[nodiscard]] VerbClass verbClass(const QString& verb);

// Lane kind for a class (§6.6): TurnPrompt is a dispatch lane; every wire-verb class is a mutation
// lane.
[[nodiscard]] LaneKind laneKind(VerbClass cls);

// Latest-wins coalescing per (verb, target) (§6.4): true only for conv-meta.
[[nodiscard]] bool isCoalescing(VerbClass cls);

// Whether draining the class requires a connected+authenticated wire (§6.3). True for every
// mutation lane; false for the turn-prompt dispatch lane (which is gated on session-idle instead).
[[nodiscard]] bool requiresConnected(VerbClass cls);

// Stable short name used to build lane ids and for the pending-UI / logs.
[[nodiscard]] QString laneClassName(VerbClass cls);

// Build the canonical lane id '<class>\x1f<scope>' (§6.3). `scope` is per-conversation for sends,
// per-session for turn prompts, per-transport for roster edits — chosen by the caller who knows
// the target.
[[nodiscard]] QString buildLane(VerbClass cls, const QString& scope);

// Map a closed-set api-error kind (§6.5) to its disposition. Unknown kinds default to Rejection
// (fail visible), matching the spec's "anything new defaults to rejection".
[[nodiscard]] ErrorClass classifyError(const QString& apiErrorKind);

} // namespace mirror
