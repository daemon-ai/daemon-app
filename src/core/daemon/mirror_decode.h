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
#include <QHash>
#include <QString>
#include <QStringList>
#include <vector>

namespace daemonapp::daemon {

// ConvList → Conversations page. `next` (if non-null) receives the resume cursor ("" = last page).
// [api/39 §10.2] `rev` (if non-null) receives the collection revision; `removed` (if non-null)
// receives the delta-read tombstone id list (the conversations removed since the request's
// since_rev). Both are absent/empty on a v38 full-list reply.
bool decodeConversationsToMirror(const QByteArray& responseCbor,
                                 std::vector<mirror::Conversation>* out, QString* next,
                                 quint64* rev = nullptr, QStringList* removed = nullptr);

// ConvHistory (JournalPageView) → the conversation's ChatMessages, scoped to (transport, conv).
// `nextCursor`/`headCursor` drive the forward page loop (§13 M1 cursor fix).
bool decodeChatHistoryToMirror(const QByteArray& responseCbor, const QString& transport,
                               const QString& conv, std::vector<mirror::ChatMessage>* out,
                               quint64* nextCursor, quint64* headCursor);

// PersonList → Persons. [api/39 §10.2] `rev`/`removed` (if non-null) receive the delta-read
// revision + tombstone id list (absent/empty on a v38 full-list reply).
bool decodePersonsToMirror(const QByteArray& responseCbor, std::vector<mirror::Person>* out,
                           quint64* rev = nullptr, QStringList* removed = nullptr);

// RosterList → Contacts for `transport` (the payload carries only the id; transport is the scope).
// `next` (if non-null) receives the roster resume cursor. [api/39 §10.2] `rev`/`removed` (if
// non-null) receive the delta-read revision + tombstone id list.
bool decodeContactsToMirror(const QByteArray& responseCbor, const QString& transport,
                            std::vector<mirror::Contact>* out, QString* next,
                            quint64* rev = nullptr, QStringList* removed = nullptr);

// RoutingListChats → ChatRoutes page → RoutePins (the origin→session pin table, §5.4 / M3). `next`
// (if non-null) receives the page resume cursor ("" = last page). RoutePins are a global list (not
// transport-scoped): the origin_key IS the canonical key.
bool decodeRoutePinsToMirror(const QByteArray& responseCbor, std::vector<mirror::RoutePin>* out,
                             QString* next);

// TransportRooms → Rooms page → Rooms for `transport` (the bindable rooms/chats + their pinned
// session). `next` (if non-null) receives the page resume cursor. `transport` stamps the
// (transport, room) key so the per-transport replace-and-prune matches the fetch scope (§3.6
// merging rule).
bool decodeRoomsToMirror(const QByteArray& responseCbor, const QString& transport,
                         std::vector<mirror::Room>* out, QString* next);

// SessionsQuery → SessionPage → Session entities (M4). `next` (if non-null) receives the roster
// page resume cursor ("" = last page; the executor page-loops full reads like the legacy
// SessionRepository). [api/39 §10.2] `rev`/`removed` (if non-null) receive the roster revision +
// the delta-read tombstone id list (absent/empty on a v38 full-page reply). [api/39 §10.3]
// `originOps` (if non-null) receives the page-side provenance map (session id → the causing op's
// client op_id) so the delta apply stamps `origin_op` and the outbox lands the op (§6.6).
bool decodeSessionsToMirror(const QByteArray& responseCbor, std::vector<mirror::Session>* out,
                            QString* next = nullptr, quint64* rev = nullptr,
                            QStringList* removed = nullptr,
                            QHash<QString, QString>* originOps = nullptr);

// Tree → TreeReport page → FleetUnit entities (M4). `next` (if non-null) receives the unit-id page
// resume cursor ("" = last page); `rev` (if non-null) receives the tree revision recorded for the
// FleetChanged rung-1 skip-gate (§5.5). The supervision-tree hierarchy is flattened into flat rows
// (child_count only); parent/children reconstruction is a §3.5 relation-index concern.
bool decodeFleetUnitsToMirror(const QByteArray& responseCbor, std::vector<mirror::FleetUnit>* out,
                              QString* next, quint64* rev = nullptr);

// SessionGet → SessionDetail → a single hydrated Session (M4): the base session-info fields plus
// the detail-only model + checkpoints. `*found` (if non-null) is set false on the null arm (the
// node does not know the session). Returns false only on a decode error.
bool decodeSessionDetailToMirror(const QByteArray& responseCbor, mirror::Session* out,
                                 bool* found = nullptr);

} // namespace daemonapp::daemon
