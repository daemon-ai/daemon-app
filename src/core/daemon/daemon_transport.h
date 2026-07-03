// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QLocalSocket;
class QSslSocket;
class QIODevice;
class QWebSocket;
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

// Transport for daemon NodeApi frames over one of three carriers:
//
//   - Unix socket (local, plaintext) and TCP wrapped in TLS (remote): length-framed STREAM
//     carriers. The wire shape matches ../daemon's daemon-host socket: a big-endian u32 byte
//     length followed by one CBOR ApiRequest/ApiResponse payload (framePayload/tryTakeFrame).
//   - WebSocket (`ws://` / `wss://`): a MESSAGE carrier where one WS binary message carries
//     exactly one mux CBOR frame - no u32 length prefix on this path (the WS layer already
//     preserves message boundaries). This is the only carrier a browser (wasm) build has; it is
//     implemented with QWebSocket so it works - and is tested - on desktop too.
//
// Codec ownership stays outside this class so the transport can be tested with raw fixture bytes;
// sendFrame()/frameReceived() always deal in bare CBOR payloads regardless of the carrier.
//
// The connection is persistent: a single socket is opened lazily on the first send and reused for
// every frame, with a receive buffer that reassembles partial frames (stream carriers only).
// Request/response correlation is the client's job (NodeApiClient), not the transport's.
//
// TLS is FAIL-CLOSED: QSslSocket's default verification (CA + hostname) is left in force; an
// untrusted certificate aborts the connection unless a pin / CA / insecureSkipVerify is
// configured. `wss://` uses the platform's default verification (no widening knobs).
//
// The stream carriers live in daemon_transport_stream.cpp and are replaced by fail-closed stubs
// (daemon_transport_stream_wasm.cpp) on wasm, where Qt lacks the `localserver` and `ssl` features.
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

    // Remote WebSocket target (`ws://host:port` or `wss://host[:port][/path]`). The handshake
    // requests the `daemon-mux` subprotocol. Resets any active connection.
    void setWsTarget(const QUrl& url);
    [[nodiscard]] QUrl wsTarget() const { return m_wsUrl; }

    // Whether the active target is TLS (gates PLAIN SASL): the TLS/TCP mode always, the WebSocket
    // mode iff the target scheme is wss. Client certs (EXTERNAL) exist only on the TLS/TCP mode.
    [[nodiscard]] bool tlsActive() const {
        return m_mode == Mode::Tcp ||
               (m_mode == Mode::Ws && m_wsUrl.scheme() == QLatin1String("wss"));
    }
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
    enum class Mode { Unix, Tcp, Ws };

    // Carrier-independent core (daemon_transport.cpp).
    [[nodiscard]] bool hasTarget() const;
    void ensureSocket();
    void onReady(); // socket reached its usable state (connected / encrypted): flush + signal
    void handleReadyRead();
    void writeFrame(const QByteArray& cborPayload); // one frame on the active carrier's framing
    void flushOutbox();
    void flushSocket(); // push buffered bytes out of the active socket now
    void resetSocket();

    // Length-framed stream carriers: Unix + TLS/TCP (daemon_transport_stream.cpp; fail-closed
    // stubs on wasm in daemon_transport_stream_wasm.cpp).
    void ensureStreamSocket();
    void openUnix();
    void openTcp();
    [[nodiscard]] bool streamSocketConnected() const;
    void flushStreamSocket();
    void resetStreamSockets();

    // Message-framed WebSocket carrier (daemon_transport.cpp; all platforms).
    void ensureWsSocket();
    void openWs();

    Mode m_mode = Mode::Unix;
    QString m_socketPath;
    QString m_host;
    quint16 m_port = 0;
    TlsConfig m_tls;
    QUrl m_wsUrl;

    QLocalSocket* m_local = nullptr;
    QSslSocket* m_ssl = nullptr;
    QWebSocket* m_ws = nullptr;
    QIODevice* m_io = nullptr; // points at whichever STREAM socket is active (never the WS)
    QByteArray m_buffer;
    QList<QByteArray> m_outbox;
};

} // namespace daemonapp::daemon
