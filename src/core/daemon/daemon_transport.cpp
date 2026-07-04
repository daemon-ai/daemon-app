// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Carrier-independent transport core plus the WebSocket carrier (available on every platform,
// including wasm). The length-framed stream carriers (Unix / TLS-TCP) live in
// daemon_transport_stream.cpp and are stubbed out on wasm.

#include "daemon/daemon_transport.h"

#include <QIODevice>
#include <QtEndian>
#include <QWebSocket>
#include <QWebSocketHandshakeOptions>

namespace daemonapp::daemon {

namespace {
// The WS subprotocol both ends pin (daemon-sync-protocol-spec): one WS binary message == one mux
// CBOR frame.
constexpr auto kWsSubprotocol = "daemon-mux";
} // namespace

QByteArray DaemonTransport::framePayload(const QByteArray& cborPayload) {
    QByteArray out;
    out.resize(4);
    qToBigEndian<quint32>(static_cast<quint32>(cborPayload.size()),
                          reinterpret_cast<uchar*>(out.data()));
    out.append(cborPayload);
    return out;
}

bool DaemonTransport::tryTakeFrame(QByteArray& buffer, QByteArray* payload) {
    if (buffer.size() < 4) {
        return false;
    }
    const auto len = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(buffer.constData()));
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

DaemonTransport::DaemonTransport(QObject* parent) : QObject(parent) {}

void DaemonTransport::setSocketPath(const QString& path) {
    if (m_mode == Mode::Unix && path == m_socketPath) {
        return;
    }
    m_mode = Mode::Unix;
    m_socketPath = path;
    close(); // drop any connection to the previous target so the next send reconnects
}

void DaemonTransport::setTcpTarget(const QString& host, quint16 port, const TlsConfig& tls) {
    m_mode = Mode::Tcp;
    m_host = host;
    m_port = port;
    m_tls = tls;
    close();
}

void DaemonTransport::setWsTarget(const QUrl& url) {
    m_mode = Mode::Ws;
    m_wsUrl = url;
    m_tls = TlsConfig{}; // no mTLS on this carrier: clientCertConfigured() must not leak over
    close();
}

bool DaemonTransport::isConnected() const {
    if (m_mode == Mode::Ws) {
        return m_ws != nullptr && m_ws->state() == QAbstractSocket::ConnectedState;
    }
    return streamSocketConnected();
}

bool DaemonTransport::hasTarget() const {
    switch (m_mode) {
    case Mode::Unix:
        return !m_socketPath.isEmpty();
    case Mode::Tcp:
        return !m_host.isEmpty() && m_port != 0;
    case Mode::Ws:
        return m_wsUrl.isValid() && !m_wsUrl.isEmpty();
    }
    return false;
}

void DaemonTransport::ensureSocket() {
    if (m_mode == Mode::Ws) {
        ensureWsSocket();
    } else {
        ensureStreamSocket();
    }
}

void DaemonTransport::ensureWsSocket() {
    if (m_ws != nullptr) {
        return;
    }
    m_ws = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(m_ws, &QWebSocket::connected, this, &DaemonTransport::onReady);
    // Message framing: the WS layer preserves boundaries, so every binary message IS one mux
    // frame - deliver it directly with no length prefix / reassembly buffer.
    connect(m_ws, &QWebSocket::binaryMessageReceived, this,
            [this](const QByteArray& message) { emit frameReceived(message); });
    connect(m_ws, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        const QString message = m_ws->errorString();
        close();
        emit failed(message);
    });
    connect(m_ws, &QWebSocket::disconnected, this, [this] { emit disconnected(); });
    // No sslErrors handler on purpose for wss: QWebSocket's default verification aborts the
    // handshake on an untrusted certificate (fail-closed; no widening knobs on this carrier).
}

void DaemonTransport::open() {
    switch (m_mode) {
    case Mode::Unix:
        openUnix();
        break;
    case Mode::Tcp:
        openTcp();
        break;
    case Mode::Ws:
        openWs();
        break;
    }
}

void DaemonTransport::openWs() {
    if (!hasTarget()) {
        emit failed(tr("No daemon WebSocket target configured"));
        return;
    }
    ensureSocket();
    if (m_ws->state() == QAbstractSocket::UnconnectedState) {
        m_buffer.clear();
        QWebSocketHandshakeOptions options;
        options.setSubprotocols({QLatin1String(kWsSubprotocol)});
        m_ws->open(m_wsUrl, options);
    }
}

void DaemonTransport::onReady() {
    flushOutbox();
    emit connected();
}

void DaemonTransport::resetSocket() {
    resetStreamSockets();
    if (m_ws != nullptr) {
        m_ws->abort();
        m_ws->deleteLater();
        m_ws = nullptr;
    }
    m_io = nullptr;
}

void DaemonTransport::close() {
    resetSocket();
    m_buffer.clear();
    m_outbox.clear();
}

void DaemonTransport::writeFrame(const QByteArray& cborPayload) {
    if (m_mode == Mode::Ws) {
        m_ws->sendBinaryMessage(cborPayload); // one message == one frame; no length prefix
    } else {
        m_io->write(framePayload(cborPayload));
    }
}

void DaemonTransport::flushOutbox() {
    if (!isConnected()) {
        return;
    }
    const QList<QByteArray> pending = m_outbox;
    m_outbox.clear();
    for (const QByteArray& frame : pending) {
        writeFrame(frame);
    }
    flushSocket();
}

void DaemonTransport::flushSocket() {
    if (m_ws != nullptr) {
        m_ws->flush();
    } else {
        flushStreamSocket();
    }
}

void DaemonTransport::sendFrame(const QByteArray& cborPayload) {
    if (!hasTarget()) {
        emit failed(tr("No daemon target configured"));
        return;
    }
    ensureSocket();
    if (isConnected()) {
        writeFrame(cborPayload);
        flushSocket();
    } else {
        // Buffer until the socket is usable; flushOutbox() drains it on connected/encrypted.
        m_outbox.append(cborPayload);
        open();
    }
}

void DaemonTransport::handleReadyRead() {
    if (m_io == nullptr) {
        return;
    }
    m_buffer.append(m_io->readAll());
    QByteArray payload;
    while (tryTakeFrame(m_buffer, &payload)) {
        emit frameReceived(payload);
    }
}

} // namespace daemonapp::daemon
