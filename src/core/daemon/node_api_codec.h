#pragma once

#include "daemon/daemon_cache_store.h"

#include <QByteArray>
#include <QList>
#include <QString>

namespace daemonapp::daemon {

struct DecodedServiceHealth {
    QString name;
    bool ok = false;
    quint32 restarts = 0;
    QString detail;
};

struct DecodedHealth {
    bool allOk = false;
    QList<DecodedServiceHealth> services;
};

enum class ApiResponseKind {
    Unknown,
    Health,
    SessionPage,
    LogPage,
    FsRoots,
    Ok,
};

// Thin C++ facade over the zcbor-generated NodeApi codec (codec/generated). The generated C is
// produced and drift-checked by the superproject (flake daemon-zcbor-codec / `just update-codec`)
// from the daemon-node contract; this facade never hand-rolls CBOR.
//
// Scope is the daemon slice the client currently exercises: encode the requests it issues and
// decode the responses it consumes. The raw CBOR bytes travel over DaemonTransport; GUI/TUI view
// models never see CBOR - repositories call this codec and write typed rows into DaemonCacheStore.
class NodeApiCodec {
public:
    // Requests.
    [[nodiscard]] static QByteArray encodeHealthRequest();
    // SessionsQuery with an empty query map (daemon applies its default scope).
    [[nodiscard]] static QByteArray encodeSessionsQueryRequest();
    // Subscribe to a session's merged log from afterSeq (exclusive), up to max entries.
    [[nodiscard]] static QByteArray encodeSubscribeRequest(const QString& sessionId,
                                                           quint64 afterSeq, quint32 max);

    // Response inspection / decode.
    [[nodiscard]] static ApiResponseKind responseKind(const QByteArray& responseCbor);
    static bool decodeHealth(const QByteArray& responseCbor, DecodedHealth* out);
    static bool decodeSessionPage(const QByteArray& responseCbor, QList<CachedSessionRow>* out,
                                  QString* nextCursor = nullptr);
    // Decode a LogPage into cache rows tagged with sessionId. The whole-message codec does not
    // expose per-entry payload re-encoding, so payloadCbor is left empty for now; seq/direction/
    // disposition + the next/head cursors are populated (enough to advance the sync cursor).
    static bool decodeLogPage(const QByteArray& responseCbor, const QString& sessionId,
                              QList<CachedLogRow>* out, quint64* nextSeq = nullptr,
                              quint64* headSeq = nullptr);
};

} // namespace daemonapp::daemon
