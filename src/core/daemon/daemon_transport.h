#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>

namespace daemonapp::daemon {

// Length-prefixed Unix-socket transport for daemon NodeApi frames.
//
// The wire shape matches ../daemon's daemon-host socket: a big-endian u32 byte length followed by
// one CBOR ApiRequest/ApiResponse payload. Codec ownership deliberately stays outside this class so
// the transport can be tested with raw fixture bytes and later fed by zcbor-generated DTO wrappers.
class DaemonTransport : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

    [[nodiscard]] static QByteArray framePayload(const QByteArray& cborPayload);
    [[nodiscard]] static bool tryTakeFrame(QByteArray& buffer, QByteArray* payload);

    void setSocketPath(const QString& path) { m_socketPath = path; }
    [[nodiscard]] QString socketPath() const { return m_socketPath; }
    void sendFrame(const QByteArray& cborPayload);

signals:
    void frameReceived(const QByteArray& cborPayload);
    void failed(const QString& message);

private:
    QString m_socketPath;
};

} // namespace daemonapp::daemon
