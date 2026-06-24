#pragma once

#include "daemon/daemon_transport.h"

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QString>

namespace daemonapp::daemon {

// Thin NodeApi request dispatcher.
//
// The public API intentionally speaks raw CBOR for now: Phase 2 will replace callers with typed
// zcbor wrappers, while transport/cache/repository tests can already use golden CBOR fixtures from
// ../daemon. UI layers must not depend on this class directly; repositories own it.
class NodeApiClient : public QObject {
    Q_OBJECT

public:
    explicit NodeApiClient(DaemonTransport* transport, QObject* parent = nullptr);

    [[nodiscard]] DaemonTransport* transport() const { return m_transport; }

    // Queue one encoded ApiRequest. The response is delivered as raw ApiResponse CBOR.
    void sendRequest(const QByteArray& requestCbor, const QString& correlationId = QString());

signals:
    void responseReady(const QString& correlationId, const QByteArray& responseCbor);
    void failed(const QString& correlationId, const QString& message);

private:
    DaemonTransport* m_transport = nullptr;
    QStringList m_pending;
};

} // namespace daemonapp::daemon
