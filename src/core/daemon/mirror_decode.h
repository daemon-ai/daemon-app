// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror A5 — the decode-to-mirror bridge (spec 09 §3.6/§5.4, 07§10 codec boundary). Decodes a
// wire ApiResponse (raw CBOR) straight into the canonical mirror entities via the G1 GENERATED
// mappers (mirror::map_*), the single wire→canonical point (§3.3). This keeps zcbor + the mappers
// confined to the daemon codec/bridge layer (07§10): the DaemonFetchExecutor hands the ingestor
// already-mapped entities, never a second row shape (rule zero / §14.1).
//
// Scope fields the payload does not carry are set from the REQUEST scope per the entity-map
// merging rule (§3.6): Contact.transport (RosterList scope) and ChatMessage.transport/conv/cursor
// (ConvHistory request scope + the journal-record envelope cursor).

#include "mirror/generated/entities_gen.h"

#include <QByteArray>
#include <QString>
#include <vector>

namespace daemonapp::daemon {

// ConvList → Conversations page. `next` (if non-null) receives the resume cursor ("" = last page).
bool decodeConversationsToMirror(const QByteArray& responseCbor,
                                 std::vector<mirror::Conversation>* out, QString* next);

// ConvHistory (JournalPageView) → the conversation's ChatMessages, scoped to (transport, conv).
// `nextCursor`/`headCursor` drive the forward page loop (§13 M1 cursor fix).
bool decodeChatHistoryToMirror(const QByteArray& responseCbor, const QString& transport,
                               const QString& conv, std::vector<mirror::ChatMessage>* out,
                               quint64* nextCursor, quint64* headCursor);

// PersonList → Persons.
bool decodePersonsToMirror(const QByteArray& responseCbor, std::vector<mirror::Person>* out);

// RosterList → Contacts for `transport` (the payload carries only the id; transport is the scope).
// `next` (if non-null) receives the roster resume cursor.
bool decodeContactsToMirror(const QByteArray& responseCbor, const QString& transport,
                            std::vector<mirror::Contact>* out, QString* next);

} // namespace daemonapp::daemon
