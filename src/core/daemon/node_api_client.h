#pragma once

#include "daemon/daemon_transport.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

namespace daemonapp::daemon {

// NodeApi request dispatcher with client-side correlation.
//
// NodeApi has no top-level request-id envelope, so a response carries nothing that identifies which
// request it answers. This client therefore keeps at most one request in flight at a time: callers
// queue encoded ApiRequests with a correlation id, requests are sent in order, and each response
// frame is matched to the outstanding request before the next one is dispatched.
//
// The public API speaks raw CBOR; encoding/decoding lives in NodeApiCodec and is owned by
// repositories. UI layers must not depend on this class directly.
class NodeApiClient : public QObject {
    Q_OBJECT

public:
    explicit NodeApiClient(DaemonTransport* transport, QObject* parent = nullptr);

    [[nodiscard]] DaemonTransport* transport() const { return m_transport; }

    // Queue one encoded ApiRequest. The response is delivered as raw ApiResponse CBOR via
    // responseReady, tagged with the same correlationId.
    void sendRequest(const QByteArray& requestCbor, const QString& correlationId = QString());

signals:
    void responseReady(const QString& correlationId, const QByteArray& responseCbor);
    void failed(const QString& correlationId, const QString& message);

private:
    struct PendingRequest {
        QString correlationId;
        QByteArray requestCbor;
    };

    void dispatchNext();

    DaemonTransport* m_transport = nullptr;
    QList<PendingRequest> m_queue;
    bool m_inFlight = false;
    QString m_currentCorrelation;
};

} // namespace daemonapp::daemon
