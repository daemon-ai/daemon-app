#include "daemon/daemon_transport.h"

#include <QLocalSocket>
#include <QSharedPointer>
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

void DaemonTransport::sendFrame(const QByteArray& cborPayload)
{
    if (m_socketPath.isEmpty()) {
        emit failed(QStringLiteral("No daemon socket path configured"));
        return;
    }

    auto* socket = new QLocalSocket(this);
    auto buffer = QSharedPointer<QByteArray>::create();

    connect(socket, &QLocalSocket::connected, socket, [socket, cborPayload] {
        socket->write(framePayload(cborPayload));
        socket->flush();
    });
    connect(socket, &QLocalSocket::readyRead, this, [this, socket, buffer] {
        buffer->append(socket->readAll());
        QByteArray payload;
        if (tryTakeFrame(*buffer, &payload)) {
            emit frameReceived(payload);
            socket->disconnectFromServer();
        }
    });
    connect(socket, &QLocalSocket::errorOccurred, this, [this, socket](QLocalSocket::LocalSocketError) {
        emit failed(socket->errorString());
        socket->deleteLater();
    });
    connect(socket, &QLocalSocket::disconnected, socket, [socket] {
        socket->deleteLater();
    });
    socket->connectToServer(m_socketPath);
}

} // namespace daemonapp::daemon
