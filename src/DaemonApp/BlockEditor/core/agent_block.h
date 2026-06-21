#pragma once

#include "core/block_record.h"

#include <QString>
#include <QVariantList>
#include <QVariantMap>

// Agent transcript blocks: the hybrid persistence/runtime model.
//
// At runtime the daemon orchestrator emits structured, typed events (no
// markdown); a typed BlockRecord carries the live state in its `metadata` map
// and is patched in place (keyed by callId). For persistence/export the same
// block round-trips through a canonical fenced markdown form whose info string
// is the block kind ("tool" / "reasoning" / "content") and whose body is a
// compact-JSON metadata payload - mirroring how ```mermaid / ```math fences are
// the canonical text form for those structured blocks.
namespace be {

// Fence info-string for an agent block type, or "" for a non-agent type.
QString agentFenceInfo(BlockType type);

// --- Message boundary markers (role layer, Strategy C) ----------------------
// A turn boundary is persisted as a tiny fenced block whose info string is "msg"
// and whose body is compact JSON: {"id":"m3","role":"user"}. On load the parser
// consumes the marker (it is NOT kept as a row), sets the current role/messageId,
// and tags following content blocks; on serialize a marker is re-emitted only
// where role/messageId changes. This mirrors the agent-block hybrid form.

// Fence info string that marks a message boundary.
QString messageMarkerFenceInfo();

// True when a fence info string denotes a message boundary marker.
bool isMessageMarkerFence(const QString &info);

// Serialize a message boundary marker to its canonical fenced markdown.
QByteArray serializeMessageMarker(MessageRole role, const QString &messageId);

// Parse a marker fence body (JSON, fences already stripped) into role + id.
void parseMessageMarker(const QString &body, MessageRole *role, QString *messageId);

// Map a fence info-string ("tool"/"reasoning"/"content") to its BlockType, or
// BlockType::Unknown when the info string is not an agent block kind.
BlockType agentBlockTypeForFence(const QString &info);

bool isAgentBlockType(BlockType type);

// Serialize a typed agent block to its canonical fenced markdown: a fence whose
// info string is the kind and whose body is compact, key-sorted JSON. Stable
// (round-trips through serialize -> parse -> serialize) so it is safe as the
// document save/export form.
QByteArray serializeAgentBlock(BlockType type, const QVariantMap &metadata);

// Parse the JSON body of an agent fence into a metadata map. `body` is the
// fenced content with the ``` lines already stripped (see fencedBodyOf). A
// non-object / invalid body yields an empty map.
QVariantMap parseAgentBlockMetadata(const QString &body);

// Strip the opening/closing fence lines (``` or ~~~) from a raw fenced block,
// returning the body alone. Mirrors the mermaid/math source extraction so a
// freshly-typed block (fences intact) and a parsed block (md4qt drops them)
// both resolve to the same body.
QString fencedBodyOf(const QString &markdown);

// --- View models (pure metadata -> QML-ready map transforms) ----------------
// These shape the structured metadata into the fields the QML renderers bind
// to, computing derived display values (title flip, duration label, status
// kind). They are pure functions so they unit-test without a QML engine.

// Tool-call view model. Mirrors the Hermes ToolView: a status-aware title, a
// subtitle (argsSummary), a normalized status ("running"/"ok"/"error"), the
// tone, a human duration label, an optional count, and the detail kind + its
// decoded payload passed through for the sub-renderer.
QVariantMap buildToolView(const QVariantMap &metadata);

// Reasoning view model: status ("running"/"complete"), duration label, and the
// raw markdown body (the caller projects it for display).
QVariantMap buildReasoningView(const QVariantMap &metadata);

// Content view model: the content kind and its body.
QVariantMap buildContentView(const QVariantMap &metadata);

// --- Inline answer-patch helpers (single-sourced answer contract) -----------
// Both front ends answer interactive blocks by patching the block's metadata
// map. These build the exact patch that EditorController::answerClarify /
// answerToolApproval produce, so the GUI and the TUI share one definition.

// Patch that marks a clarify tool answered. `answers` is the canonical
// per-question payload (question id -> string or string list); the result is
// { answered:true, answers, answer:"flat; human; summary" } where the flat
// summary joins each answer (lists comma-joined) with "; ".
QVariantMap clarifyAnswerPatch(const QVariantMap &answers);

// Patch that records a tool-approval decision ("approved"/"denied"/...). The
// result clears the approval gate ({ approval:decision, needsApproval:false });
// a "denied" decision also flips the tool to { status:"error" }.
QVariantMap toolApprovalPatch(const QString &decision);

// --- Sub-renderer parsers ---------------------------------------------------

// Split terminal text carrying ANSI SGR escapes into styled spans. Each span is
// a map { text, fg, bg, bold, dim, italic, underline, inverse } where fg/bg are
// the 16-color index (0-15) or -1 for the default. Only SGR (CSI ... m) is
// interpreted; other escape sequences are dropped. Newlines are preserved
// inside span text so the QML layer can render the block as wrapped pre-text.
QVariantList ansiToSpans(const QString &text);

// Split a unified diff into typed lines. Each entry is { text, kind } where kind
// is "add" | "del" | "hunk" | "meta" | "context".
QVariantList parseUnifiedDiff(const QString &diff);

} // namespace be
