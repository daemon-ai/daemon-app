// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/block_record.h"
#include "core/coordinate_map.h"
#include "core/markdown_parser.h"

#include <optional>
#include <QByteArray>
#include <QRegularExpression>
#include <QString>
#include <QStringView>
#include <QVariantMap>

namespace be::docstore {

// Free parse/slice/serialize helpers backing DocumentStore. Split out of
// document_store.cpp (where they were a file-local anonymous namespace) to keep
// that translation unit focused on the store's stateful methods; this unit is
// included only by document_store.cpp.

// Retype a freshly built code-fence record whose info string names an agent block
// kind into the matching typed block and hydrate its metadata from the fenced JSON
// body (the markdown -> typed step of the agent-block round-trip).
void applyAgentBlockFromFence(BlockRecord& record, const QString& info);

bool isListType(BlockType type);

// Cached compiled regexes for the leading block markers.
const QRegularExpression& headingRe();

QString firstLineOf(const QString& content);

// GitHub-style heading slug: lowercase, drop punctuation other than '-'/'_', then
// collapse space runs to single hyphens ("## Foo Bar!" -> "foo-bar").
QString headingSlug(const QString& headingText);

// Language token after a leading ``` / ~~~ fence on the first line, or empty.
QString fenceLanguageOf(const QString& content);

// Store image attributes (url/alt/title/link/size) parsed from a single-line
// standalone image block into `metadata`.
void applyImageMetadata(QVariantMap& metadata, const QString& content);

// Length (UTF-16) of a list item's leading marker (indent + bullet/number/checkbox
// + trailing space). 0 when absent.
qsizetype leadingMarkerLength(const QString& content, BlockType type);

// Strip a leading block-level marker (list marker or heading hashes) so the inline
// text can be concatenated onto another block.
QString strippedLineText(const QString& content, BlockType type);

// Marker a freshly continued list item should carry (tasks reset to unchecked).
QString continuationMarker(const QString& content, BlockType type);

// Canonical separator between two adjacent serialized blocks.
QByteArray separatorBetween(const BlockRecord& prev, const BlockRecord& cur);

// Offset (UTF-16) of the start of the last block-group in `buffer` (the byte after
// the last blank line that is not inside a fenced code block). Everything before is
// structurally settled.
qsizetype fenceSafeLastBoundary(const QString& buffer);

// Rewrite the literal number of an ordered-list item's marker.
QString withOrderedNumber(const QString& content, int number);

// True when `line` (after trimming) opens or closes a fenced code block.
bool isFenceMarkerLine(QStringView line);

// Recovered ``` / ~~~ fenced span (full delimiters included), in UTF-16 offsets.
struct FenceSpan {
    qsizetype start = 0;
    qsizetype end = 0;
};

// Recover a fenced code block's full span (md4qt reports only the body between the
// delimiters). Prefers md4qt's delimiter line positions and falls back to a raw
// line scan; nullopt when no fence span can be recovered.
std::optional<FenceSpan> fenceSpanUtf16(const CoordinateMap& map, const QString& text,
                                        const ParsedBlock& pb);

// Slice a parsed block's raw text out of `text` via `map`, trimming trailing
// newlines, normalizing list indentation and expanding fenced code to include its
// delimiters.
QString sliceBlockContent(const CoordinateMap& map, const QString& text, const ParsedBlock& pb);

int orderedNumberOf(const QString& content, int fallback);

// First-line classification of a list item (task/ordered/bullet). Returns
// BlockType::Unknown when `line` is not a list item; sets `*indent` on a match.
BlockType classifyListItem(const QString& line, quint16* indent);

// First-line classification of the non-list, non-heading block shapes
// (image/table/code-fence/custom/quote/paragraph).
BlockType classifyBlockShape(const QString& content, const QString& line);

} // namespace be::docstore
