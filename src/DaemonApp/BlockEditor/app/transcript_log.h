// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/document_store.h"
#include "domain/session_log.h"

#include <QList>
#include <QString>

namespace be {

// Transcript <-> SessionLogEntry bridge (roadmap P4). This is the app-layer seam that
// lets the transcript be driven by a wire-shaped `domain::SessionLogEntry` sequence
// instead of a raw markdown blob:
//
//   markdown  --decomposeMarkdown-->  QList<SessionLogEntry>  --applyTranscriptLog--> DocumentStore
//
// The mock decomposes a session's stored markdown into entries; a future daemon adapter
// fills the same entry shape from the decoded wire `SessionPayload`, and rendering goes
// through the identical `applyTranscriptLog`. `be_core` stays domain-free; this bridge
// (compiled into the QML module, not `be_core`) is where `domain` meets the document.

// Split a session's markdown into one entry per block, carrying the block's role/
// messageId and canonical markdown (agent blocks also carry their structured metadata).
[[nodiscard]] QList<domain::SessionLogEntry> decomposeMarkdown(const QString& markdown);

// Rebuild `store` from an entry sequence (resets it first), reconstructing each
// role/message group through the role-aware append API so the resulting blocks and
// serialized markdown match the markdown path.
void applyTranscriptLog(DocumentStore& store, const QList<domain::SessionLogEntry>& entries);

} // namespace be
