// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QLocalSocket;
class QSslSocket;
class QIODevice;
QT_END_NAMESPACE

namespace daemonapp::daemon {

// TLS configuration for a TCP (remote) connection. Empty fields mean "use the default": full CA +
// hostname verification with NO ignoreSslErrors (fail-closed). The optional knobs widen trust only
// when explicitly set.
struct TlsConfig {
    QString caFile; // extra CA bundle to trust (e.g. a private CA)
    QString
        pinnedSha256; // pin the leaf cert by SHA-256 fingerprint (accepts a matching self-signed)
    QString clientCertFile;          // mTLS client certificate (PEM) -> enables EXTERNAL
    QString clientKeyFile;           // mTLS client private key (PEM)
    bool insecureSkipVerify = false; // dev-only escape hatch; loud + never the default
};

// Length-prefixed transport for daemon NodeApi frames over either a Unix socket (local, plaintext)
// or a TCP socket wrapped in TLS (remote). The wire shape matches ../daemon's daemon-host socket: a
// big-endian u32 byte length followed by one CBOR ApiRequest/ApiResponse payload. Codec ownership
// stays outside this class so the transport can be tested with raw fixture bytes.
//
// The connection is persistent: a single socket is opened lazily on the first send and reused for
// every frame, with a receive buffer that reassembles partial frames. Request/response correlation
// is the client's job (NodeApiClient), not the transport's.
//
// TLS is FAIL-CLOSED: QSslSocket's default verification (CA + hostname) is left in force; an
// untrusted certificate aborts the connection unless a pin / CA / insecureSkipVerify is configured.
class DaemonTransport : public QObject {
    Q_OBJECT

public:
    explicit DaemonTransport(QObject* parent = nullptr);
    // No custom destructor: the sockets are parented to this QObject, so base QObject teardown
    // deletes them. (Calling close()/deleteLater() from the destructor would double-free the
    // already-parented children.)

    [[nodiscard]] static QByteArray framePayload(const QByteArray& cborPayload);
    [[nodiscard]] static bool tryTakeFrame(QByteArray& buffer, QByteArray* payload);

    // Local Unix-socket target (plaintext). Resets any active connection.
    void setSocketPath(const QString& path);
    [[nodiscard]] QString socketPath() const { return m_socketPath; }

    // Remote TCP target (TLS). `host:port`; `tls` carries the verification policy. Resets any
    // active connection.
    void setTcpTarget(const QString& host, quint16 port, const TlsConfig& tls);

    // Whether the active target is TLS (gates PLAIN/EXTERNAL SASL) and whether a client cert is
    // set.
    [[nodiscard]] bool tlsActive() const { return m_mode == Mode::Tcp; }
    [[nodiscard]] bool clientCertConfigured() const {
        return !m_tls.clientCertFile.isEmpty() && !m_tls.clientKeyFile.isEmpty();
    }

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
    enum class Mode { Unix, Tcp };

    void ensureSocket();
    void openUnix();
    void openTcp();
    void onReady(); // socket reached its usable state (connected / encrypted): flush + signal
    void handleReadyRead();
    void flushOutbox();
    void flushSocket(); // push buffered bytes out of the active socket now
    void resetSocket();

    Mode m_mode = Mode::Unix;
    QString m_socketPath;
    QString m_host;
    quint16 m_port = 0;
    TlsConfig m_tls;

    QLocalSocket* m_local = nullptr;
    QSslSocket* m_ssl = nullptr;
    QIODevice* m_io = nullptr; // points at whichever socket is active
    QByteArray m_buffer;
    QList<QByteArray> m_outbox;
};

} // namespace daemonapp::daemon
