#pragma once

#include "daemon/daemon_transport.h"

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

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

    // Per-request timeout override (defaults to kRequestTimeoutMs). Exposed so tests can drive the
    // hung-daemon path without a 30s wait; production code keeps the default.
    void setRequestTimeoutMs(int ms) { m_requestTimer.setInterval(ms); }

signals:
    void responseReady(const QString& correlationId, const QByteArray& responseCbor);
    void failed(const QString& correlationId, const QString& message);

private:
    struct PendingRequest {
        QString correlationId;
        QByteArray requestCbor;
    };

    void dispatchNext();
    // Fail the in-flight request (if any) then drain the queue with the same error. Idempotent, so
    // it is safe to call from both the transport's failed and disconnected signals.
    void failAllPending(const QString& message);

    // A hung-but-connected daemon (accepts the frame, never replies) would otherwise leave
    // m_inFlight stuck forever and stall the queue. Bound every in-flight request so the queue
    // self-unsticks (and so the connection heartbeat can detect a dead-but-open socket).
    static constexpr int kRequestTimeoutMs = 30000;

    DaemonTransport* m_transport = nullptr;
    QList<PendingRequest> m_queue;
    bool m_inFlight = false;
    QString m_currentCorrelation;
    QTimer m_requestTimer;
};

} // namespace daemonapp::daemon
