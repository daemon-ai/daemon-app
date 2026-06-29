// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace daemonapp::daemon {

// Length-prefixed Unix-socket transport for daemon NodeApi frames.
//
// The wire shape matches ../daemon's daemon-host socket: a big-endian u32 byte length followed by
// one CBOR ApiRequest/ApiResponse payload. Codec ownership deliberately stays outside this class so
// the transport can be tested with raw fixture bytes and fed by the zcbor-generated codec.
//
// The connection is persistent: a single QLocalSocket is opened lazily on the first send and
// reused for every frame, with a receive buffer that reassembles partial frames. NodeApi has no
// request-id envelope, so request/response correlation is the client's job (see NodeApiClient),
// not the transport's.
class DaemonTransport : public QObject {
    Q_OBJECT

public:
    explicit DaemonTransport(QObject* parent = nullptr);

    [[nodiscard]] static QByteArray framePayload(const QByteArray& cborPayload);
    [[nodiscard]] static bool tryTakeFrame(QByteArray& buffer, QByteArray* payload);

    void setSocketPath(const QString& path);
    [[nodiscard]] QString socketPath() const { return m_socketPath; }

    [[nodiscard]] bool isConnected() const;
    void open();
    void close();
    void sendFrame(const QByteArray& cborPayload);

signals:
    void connected();
    void disconnected();
    void frameReceived(const QByteArray& cborPayload);
    void failed(const QString& message);

private:
    void ensureSocket();
    void handleReadyRead();
    void flushOutbox();

    QString m_socketPath;
    QLocalSocket* m_socket = nullptr;
    QByteArray m_buffer;
    QList<QByteArray> m_outbox;
};

} // namespace daemonapp::daemon
