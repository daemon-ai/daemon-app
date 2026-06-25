#include "daemon/daemon_transport.h"

#include <QLocalSocket>
#include <QtEndian>

namespace daemonapp::daemon {

QByteArray DaemonTransport::framePayload(const QByteArray& cborPayload)
{
    QByteArray out;
    out.resize(4);
    qToBigEndian<quint32>(static_cast<quint32>(cborPayload.size()),
                          reinterpret_cast<uchar*>(out.data()));
    out.append(cborPayload);
    return out;
}

bool DaemonTransport::tryTakeFrame(QByteArray& buffer, QByteArray* payload)
{
    if (buffer.size() < 4) {
        return false;
    }
    const quint32 len = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(buffer.constData()));
    const qsizetype frameSize = 4 + static_cast<qsizetype>(len);
    if (buffer.size() < frameSize) {
        return false;
    }
    if (payload != nullptr) {
        *payload = buffer.mid(4, static_cast<qsizetype>(len));
    }
    buffer.remove(0, frameSize);
    return true;
}

DaemonTransport::DaemonTransport(QObject* parent)
    : QObject(parent)
{
}

void DaemonTransport::setSocketPath(const QString& path)
{
    if (path == m_socketPath) {
        return;
    }
    m_socketPath = path;
    // Drop any connection to the previous target so the next send reconnects.
    close();
}

bool DaemonTransport::isConnected() const
{
    return m_socket != nullptr && m_socket->state() == QLocalSocket::ConnectedState;
}

void DaemonTransport::ensureSocket()
{
    if (m_socket != nullptr) {
        return;
    }
    m_socket = new QLocalSocket(this);
    connect(m_socket, &QLocalSocket::connected, this, [this] {
        flushOutbox();
        emit connected();
    });
    connect(m_socket, &QLocalSocket::readyRead, this, &DaemonTransport::handleReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this,
            [this](QLocalSocket::LocalSocketError) {
                // Capture the message, then fully reset: a failed/aborted socket must not leave a
                // queued frame in m_outbox, or it would flush ahead of the next request on the next
                // successful connect and corrupt request/response correlation.
                const QString message = m_socket->errorString();
                close();
                emit failed(message);
            });
    connect(m_socket, &QLocalSocket::disconnected, this, [this] { emit disconnected(); });
}

void DaemonTransport::open()
{
    if (m_socketPath.isEmpty()) {
        emit failed(QStringLiteral("No daemon socket path configured"));
        return;
    }
    ensureSocket();
    if (m_socket->state() == QLocalSocket::UnconnectedState) {
        m_buffer.clear();
        m_socket->connectToServer(m_socketPath);
    }
}

void DaemonTransport::close()
{
    if (m_socket != nullptr) {
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_buffer.clear();
    m_outbox.clear();
}

void DaemonTransport::flushOutbox()
{
    if (m_socket == nullptr) {
        return;
    }
    const QList<QByteArray> pending = m_outbox;
    m_outbox.clear();
    for (const QByteArray& frame : pending) {
        m_socket->write(framePayload(frame));
    }
    m_socket->flush();
}

void DaemonTransport::sendFrame(const QByteArray& cborPayload)
{
    if (m_socketPath.isEmpty()) {
        emit failed(QStringLiteral("No daemon socket path configured"));
        return;
    }
    ensureSocket();
    if (isConnected()) {
        m_socket->write(framePayload(cborPayload));
        m_socket->flush();
    } else {
        // Buffer until the socket connects; flushOutbox() drains it on the connected signal.
        m_outbox.append(cborPayload);
        open();
    }
}

void DaemonTransport::handleReadyRead()
{
    m_buffer.append(m_socket->readAll());
    QByteArray payload;
    while (tryTakeFrame(m_buffer, &payload)) {
        emit frameReceived(payload);
    }
}

} // namespace daemonapp::daemon
