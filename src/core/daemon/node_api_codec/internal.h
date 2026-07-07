// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// Private shared surface for the NodeApiCodec facade implementation. The public facade + the
// `Decoded*` types live in node_api_codec.h (the stability boundary); this header is internal to
// the node_api_codec/*.cpp translation units only. It pulls in the vendored zcbor-generated codec
// (WIRE-NEUTRAL: never hand-edited) and declares the free helpers shared across the split TUs.

#include "daemon/node_api_codec.h"

#include <array>
#include <functional>
#include <memory>

extern "C" {
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
}

namespace daemonapp::daemon::codec_detail {

// --- zcbor_string <-> Qt -------------------------------------------------------------------------
QString fromZcbor(const zcbor_string& s);
QByteArray bytesFromZcbor(const zcbor_string& s);
// Point a generated zcbor_string at an existing buffer (zcbor borrows; `b` must outlive the
// encode).
void setZcbor(zcbor_string& z, const QByteArray& b);

// --- enum <-> wire-string mappers ----------------------------------------------------------------
void setFsRootId(const QString& rootId, fs_root_id_t_r& out, QByteArray& scratch);
QString fsRootIdToString(const fs_root_id_t_r& r);
QString fsEntryKindName(int choice);
QString fsChangeKindName(int choice);
QString unitKindName(int choice);
QString unitStateName(int choice);
QString sessionRoleName(int choice);
// [wave2:app-delegation] F3: UnitNode v29 enrichment decoders.
QString delegationLifetimeName(int choice);
void decodeEngineSelector(const engine_selector_r& sel, QString* kind, QString* agent);
QString fsRootKindName(int choice);
QString sessionStateName(int choice);
QString lifecycleName(int choice);
QString roleName(int choice);
QString providerName(int choice);
int providerChoice(const QString& provider);
QString providerKindName(int choice);
QString contextEngineName(int choice);
QString memoryProviderName(int choice);
QString endReasonName(int choice);
int modelEngineChoice(const QString& engine);
QString modelEngineName(int choice);
int searchSortChoice(const QString& sort);
QString downloadStateName(int choice);
QString hostRequestKindName(const host_request_kind_t_r& kind);
// [wave2:app-channels-liveness] connection/presence state -> lowercase wire-name (shared by the
// TransportInstances decode and the TransportChanged node-event decode). Defined in
// fs_fleet_channels.cpp.
QString connectionStateName(const connection_state_r& c);
QString presenceStateName(const presence_state_r& p);
// [waveB:app-v30] D1: DisconnectReason -> coarse lowercase token (shared by the TransportInstances
// decode and the TransportChanged node-event decode). Defined in fs_fleet_channels.cpp.
QString disconnectReasonName(const disconnect_reason_r& r);

// --- struct projection mappers (Qt-side) ---------------------------------------------------------
// Scratch buffers a populated wire `origin` borrows (zcbor_string fields point into these); the
// caller keeps it alive until encodeRequest runs.
struct OriginScratch {
    QByteArray transport, user, chat, thread, apiKey;
};
// Populate a generated `origin` from a DecodedOrigin (shared by the Routing* encoders).
void fillOrigin(origin& out, const DecodedOrigin& o, OriginScratch& sc);
// Project a generated `origin` into the flattened DecodedOrigin.
DecodedOrigin decodeOriginStruct(const origin& o);
// Project a generated `chat_route` into DecodedChatRoute.
DecodedChatRoute decodeChatRouteStruct(const chat_route& r);
DecodedUnitNode decodeUnitNodeStruct(const unit_node& n);
// Project a generated `session_info` into a CachedSessionRow (shared by decodeSessionPage and
// decodeSessionDetail). Leaves `updatedAtMs` at 0 (the caller stamps it).
CachedSessionRow sessionRowFromInfo(const session_info& info);
void fillDescriptor(const model_descriptor& m, DecodedModelDescriptor* out);
void fillProviderDescriptor(const provider_descriptor& p, DecodedProviderDescriptor* out);
DecodedProfileSpec decodeProfileSpecStruct(const profile_spec& ps);
DecodedAgentEvent decodeAgentEvent(const agent_event_r& ev);
void fillModelRef(const model_ref& m, QString* repo, QString* file, QString* engine);
void decodeHostRequest(const host_request& req, DecodedLogEntry* out);
DecodedTranscriptBlock decodeTranscriptBlock(const transcript_block_r& block);

// --- L0 wire-envelope CBOR primitives (hand-coded; see wire_l0.cpp) ------------------------------
void cborAppendUint(QByteArray& b, quint64 v);
void cborAppendText(QByteArray& b, const char* s);
// Append an arbitrary-length CBOR text string (the short cborAppendText only handles <24 bytes;
// usernames / tokens / mechanism names can be longer).
void cborAppendTextLen(QByteArray& b, const QByteArray& s);
// Append `bytes` as a CBOR array of uints (major 4, one uint per byte). The frozen contract models
// the SASL byte payloads (AuthStart.initial / AuthStep.data) as Rust `Vec<u8>` WITHOUT serde_bytes,
// so they are `[* uint]` on the wire, NOT a bstr.
void cborAppendBytesAsUintArray(QByteArray& b, const QByteArray& bytes);
bool cborReadHead(const uchar* p, qsizetype n, qsizetype& i, quint8& major, quint64& arg);
bool cborReadText(const uchar* p, qsizetype n, qsizetype& i, QByteArray* out);
// Read a CBOR array of uints (major 4) into `out` as raw bytes (the inverse of
// cborAppendBytesAsUintArray; each element must be a 0..255 uint). Used to read AuthChallenge.data.
bool cborReadBytesFromUintArray(const uchar* p, qsizetype n, qsizetype& i, QByteArray* out);

// --- encode/decode core --------------------------------------------------------------------------
bool encodeRequest(const api_request_r& request, QByteArray* out);
// Encode a payload-free ApiRequest (just selects the choice arm).
QByteArray encodeSimple(decltype(api_request_r::api_request_choice) choice);
// Encode a data-carrying ApiRequest: select the arm, run `fill` (caller's local buffers stay alive
// across the synchronous encode), encode.
QByteArray encodeWithFill(decltype(api_request_r::api_request_choice) choice,
                          const std::function<void(api_request_r&)>& fill);

// Scratch buffers a populated profile_spec borrows (the zcbor_string fields point into these), so
// the caller must keep this alive until encodeRequest runs.
struct ProfileSpecScratch {
    QByteArray id, model, prompt, baseUrl, credRef, fbCredRef, engineAgent;
    QList<QByteArray> toolBufs, boundTransports, boundCredRefs;
};
// Populate a generated profile_spec from a DecodedProfileSpec (shared by ProfileCreate/Update and
// by ProfileImport's embedded Distribution.profile).
void fillProfileSpec(profile_spec& ps, const DecodedProfileSpec& s, ProfileSpecScratch& sc);
// Encode a ProfileCreate (update=false) or ProfileUpdate (update=true) carrying a full
// profile_spec.
QByteArray encodeProfileMutation(bool update, const DecodedProfileSpec& s);

bool decodeResponse(const QByteArray& responseCbor, api_response_r* out);
// Decode a full ApiResponse and require it to be the `expected` arm. Returns the heap-owned
// response or nullptr on a decode miss / arm mismatch. Collapses the identical decoder preambles.
std::unique_ptr<api_response_r>
decodeChecked(const QByteArray& responseCbor,
              decltype(api_response_r::api_response_choice) expected);

} // namespace daemonapp::daemon::codec_detail
