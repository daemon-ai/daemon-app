#include "daemon/node_api_client.h"

namespace daemonapp::daemon {

NodeApiClient::NodeApiClient(DaemonTransport* transport, QObject* parent)
    : QObject(parent)
    , m_transport(transport)
{
    if (m_transport != nullptr) {
        connect(m_transport, &DaemonTransport::frameReceived, this, [this](const QByteArray& cbor) {
            const QString correlationId = m_pending.isEmpty() ? QString() : m_pending.takeFirst();
            emit responseReady(correlationId, cbor);
        });
        connect(m_transport, &DaemonTransport::failed, this, [this](const QString& message) {
            const QString correlationId = m_pending.isEmpty() ? QString() : m_pending.takeFirst();
            emit failed(correlationId, message);
        });
    }
}

void NodeApiClient::sendRequest(const QByteArray& requestCbor, const QString& correlationId)
{
    if (m_transport == nullptr) {
        emit failed(correlationId, QStringLiteral("No daemon transport configured"));
        return;
    }
    if (requestCbor.isEmpty()) {
        emit failed(correlationId, QStringLiteral("Refusing to send an empty NodeApi request"));
        return;
    }
    m_pending.append(correlationId);
    m_transport->sendFrame(requestCbor);
}

} // namespace daemonapp::daemon
