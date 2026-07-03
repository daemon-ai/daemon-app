// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// The WebSocket carrier of the fake mux daemon: MuxServerCore (wire_mux_fixture.h) served over a
// QWebSocketServer. Framing follows the pinned WS transport contract: one WS BINARY message
// carries exactly one mux CBOR frame - no u32 length prefix on this path - and the handshake
// offers the `daemon-mux` subprotocol. Everything above the carrier (Hello, SASL SCRAM,
// Call/Reply, Open/Item/End) is byte-identical to the QLocalServer fixture by construction, which
// is what makes the WS-vs-Unix parity tests meaningful.

#include "wire_mux_fixture.h"

#include <QHostAddress>
#include <QUrl>
#include <QWebSocket>
#include <QWebSocketServer>

namespace daemonapp::test {

class WsMuxServer : public MuxServerCore {
public:
    ~WsMuxServer() override { stop(); }

    // Listen on an ephemeral localhost port (ws://, plaintext); url() is the client target.
    bool start() {
        m_server = new QWebSocketServer(QStringLiteral("daemon-mux-fixture"),
                                        QWebSocketServer::NonSecureMode);
        m_server->setSupportedSubprotocols({QStringLiteral("daemon-mux")});
        QObject::connect(m_server, &QWebSocketServer::newConnection, m_server,
                         [this] { onNewConnection(); });
        return m_server->listen(QHostAddress::LocalHost, 0);
    }

    void stop() {
        for (QWebSocket* conn : std::as_const(m_conns)) {
            conn->abort();
            conn->deleteLater();
        }
        m_conns.clear();
        m_auth.clear();
        m_streamConn = nullptr;
        if (m_server != nullptr) {
            m_server->close();
            m_server->deleteLater();
            m_server = nullptr;
        }
    }

    [[nodiscard]] QUrl url() const {
        return QUrl(QStringLiteral("ws://127.0.0.1:%1").arg(m_server->serverPort()));
    }

    // The subprotocol negotiated on the most recent connection (empty until one arrives). The
    // transport must request - and therefore negotiate - "daemon-mux".
    [[nodiscard]] QString lastSubprotocol() const { return m_lastSubprotocol; }

private:
    void onNewConnection() {
        while (m_server != nullptr && m_server->hasPendingConnections()) {
            QWebSocket* conn = m_server->nextPendingConnection();
            m_conns.append(conn);
            m_lastSubprotocol = conn->subprotocol();
            QObject::connect(conn, &QWebSocket::binaryMessageReceived, conn,
                             [this, conn](const QByteArray& frame) { onFrame(conn, frame); });
        }
    }

    void onFrame(QWebSocket* conn, const QByteArray& frame) {
        QList<QByteArray> replies;
        if (processFrame(m_auth[conn], frame, &replies)) {
            m_streamConn = conn;
        }
        for (const QByteArray& payload : replies) {
            conn->sendBinaryMessage(payload);
        }
    }

    void writeStreamFrame(const QByteArray& payload) override {
        m_streamConn->sendBinaryMessage(payload);
    }
    [[nodiscard]] bool streamPeerActive() const override { return m_streamConn != nullptr; }

    QWebSocketServer* m_server = nullptr;
    QList<QWebSocket*> m_conns;
    QHash<QWebSocket*, AuthCtx> m_auth;
    QWebSocket* m_streamConn = nullptr;
    QString m_lastSubprotocol;
};

} // namespace daemonapp::test
