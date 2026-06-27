#include "daemon/node_api_client.h"

namespace daemonapp::daemon {

NodeApiClient::NodeApiClient(DaemonTransport* transport, QObject* parent)
    : QObject(parent), m_transport(transport) {
    m_requestTimer.setSingleShot(true);
    m_requestTimer.setInterval(kRequestTimeoutMs);
    connect(&m_requestTimer, &QTimer::timeout, this,
            [this] { failAllPending(QStringLiteral("request timed out")); });
    if (m_transport != nullptr) {
        connect(m_transport, &DaemonTransport::frameReceived, this, [this](const QByteArray& cbor) {
            if (!m_inFlight) {
                // No request is outstanding; without a request-id envelope an unsolicited frame
                // cannot be correlated, so drop it.
                return;
            }
            m_requestTimer.stop();
            const QString correlationId = m_currentCorrelation;
            m_inFlight = false;
            m_currentCorrelation.clear();
            emit responseReady(correlationId, cbor);
            dispatchNext();
        });
        connect(m_transport, &DaemonTransport::failed, this,
                [this](const QString& message) { failAllPending(message); });
        // An unexpected close (daemon exit / peer disconnect) while a request is in flight would
        // otherwise leave m_inFlight stuck true and stall the queue forever; treat it as a failure.
        connect(m_transport, &DaemonTransport::disconnected, this,
                [this] { failAllPending(QStringLiteral("daemon connection closed")); });
    }
}

void NodeApiClient::failAllPending(const QString& message) {
    // Fail the in-flight request, then drain the queue with the same error so callers are not left
    // waiting on a dead socket. Idempotent: once nothing is in flight and the queue is empty this
    // is a no-op, so the error+disconnect double-fire is harmless.
    m_requestTimer.stop();
    if (m_inFlight) {
        const QString correlationId = m_currentCorrelation;
        m_inFlight = false;
        m_currentCorrelation.clear();
        emit failed(correlationId, message);
    }
    const QList<PendingRequest> pending = m_queue;
    m_queue.clear();
    for (const PendingRequest& request : pending) {
        emit failed(request.correlationId, message);
    }
}

void NodeApiClient::sendRequest(const QByteArray& requestCbor, const QString& correlationId) {
    if (m_transport == nullptr) {
        emit failed(correlationId, QStringLiteral("No daemon transport configured"));
        return;
    }
    if (requestCbor.isEmpty()) {
        emit failed(correlationId, QStringLiteral("Refusing to send an empty NodeApi request"));
        return;
    }
    m_queue.append({correlationId, requestCbor});
    dispatchNext();
}

void NodeApiClient::dispatchNext() {
    if (m_inFlight || m_queue.isEmpty()) {
        return;
    }
    const PendingRequest next = m_queue.takeFirst();
    m_inFlight = true;
    m_currentCorrelation = next.correlationId;
    m_requestTimer.start(); // bound this request so a silent daemon can't stall the queue
    m_transport->sendFrame(next.requestCbor);
}

} // namespace daemonapp::daemon
