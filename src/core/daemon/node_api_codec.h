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

// Thin C++ facade over the zcbor-generated daemon-api subset codec (see
// codec/generated, regenerated from ../daemon `daemon-api.cddl` via `xtask zcbor-spike`).
//
// Scope is deliberately the first daemon slice: encode the requests the slice issues and
// decode the responses it consumes. The raw CBOR bytes travel over DaemonTransport; GUI/TUI
// view models never see CBOR - repositories call this codec and write typed rows into the
// DaemonCacheStore. No hand-rolled Qt CBOR lives anywhere in the client.
class NodeApiCodec {
public:
    // Requests.
    [[nodiscard]] static QByteArray encodeHealthRequest();
    // SessionsQuery with an empty query map (daemon applies its default scope).
    [[nodiscard]] static QByteArray encodeSessionsQueryRequest();

    // Response inspection / decode.
    [[nodiscard]] static ApiResponseKind responseKind(const QByteArray& responseCbor);
    static bool decodeHealth(const QByteArray& responseCbor, DecodedHealth* out);
    static bool decodeSessionPage(const QByteArray& responseCbor, QList<CachedSessionRow>* out,
                                  QString* nextCursor = nullptr);
};

} // namespace daemonapp::daemon
