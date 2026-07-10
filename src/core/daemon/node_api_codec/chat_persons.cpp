// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] The native-chat + persons + transport-settings codec wrappers: ConvSend /
// ConvHistory (a per-conversation JournalRecordPayload::Chat stream), PersonList, and the account
// settings read/configure ops. Kept in its own TU (the channels/messaging domain) so it merges
// cleanly beside fs_fleet_channels.cpp. Hand-written over the vendored zcbor codec — never CBOR by
// hand.

#include "daemon/node_api_codec/internal.h"

namespace daemonapp::daemon {

using namespace codec_detail;

// ===== STUBS (red) — implemented in the green commit =====

QByteArray NodeApiCodec::encodeConvSendRequest(const QString& transport, const QString& conv,
                                               const QString& text) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    Q_UNUSED(text)
    return {};
}

QByteArray NodeApiCodec::encodeConvHistoryRequest(const QString& transport, const QString& conv,
                                                  bool hasAfter, quint64 afterCursor, bool hasMax,
                                                  quint32 max) {
    Q_UNUSED(transport)
    Q_UNUSED(conv)
    Q_UNUSED(hasAfter)
    Q_UNUSED(afterCursor)
    Q_UNUSED(hasMax)
    Q_UNUSED(max)
    return {};
}

bool NodeApiCodec::decodeConvHistory(const QByteArray& responseCbor, QList<DecodedChatMessage>* out,
                                     quint64* nextCursor, quint64* headCursor) {
    Q_UNUSED(responseCbor)
    Q_UNUSED(out)
    Q_UNUSED(nextCursor)
    Q_UNUSED(headCursor)
    return false;
}

QByteArray NodeApiCodec::encodePersonListRequest() {
    return {};
}

bool NodeApiCodec::decodePersons(const QByteArray& responseCbor, QList<DecodedPerson>* out) {
    Q_UNUSED(responseCbor)
    Q_UNUSED(out)
    return false;
}

QByteArray NodeApiCodec::encodeTransportSettingsRequest(const QString& transport) {
    Q_UNUSED(transport)
    return {};
}

bool NodeApiCodec::decodeTransportSettings(const QByteArray& responseCbor,
                                           QMap<QString, QString>* out) {
    Q_UNUSED(responseCbor)
    Q_UNUSED(out)
    return false;
}

QByteArray NodeApiCodec::encodeTransportConfigureRequest(const QString& transport,
                                                         const QMap<QString, QString>& settings) {
    Q_UNUSED(transport)
    Q_UNUSED(settings)
    return {};
}

} // namespace daemonapp::daemon
