// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/redirect_sink.h"

#ifndef Q_OS_WASM
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#endif

namespace auth {

RedirectSink::RedirectSink(QObject* parent) : QObject(parent) {}

RedirectSink::~RedirectSink() {
    stop();
}

bool RedirectSink::start() {
#ifdef Q_OS_WASM
    // A browser page cannot host a loopback listener; the controller runs URL-only there.
    return false;
#else
    if (m_server != nullptr && m_server->isListening()) {
        return true;
    }
    stop();
    m_server = new QTcpServer(this);
    if (!m_server->listen(QHostAddress::LocalHost, 0)) {
        stop();
        return false;
    }
    connect(m_server, &QTcpServer::newConnection, this, &RedirectSink::onNewConnection);
    m_redirectUri = QStringLiteral("http://127.0.0.1:%1/cb").arg(m_server->serverPort());
    return true;
#endif
}

void RedirectSink::stop() {
#ifndef Q_OS_WASM
    if (m_server != nullptr) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
#endif
    m_redirectUri.clear();
}

bool RedirectSink::listening() const {
#ifdef Q_OS_WASM
    return false;
#else
    return m_server != nullptr && m_server->isListening();
#endif
}

void RedirectSink::onNewConnection() {
#ifndef Q_OS_WASM
    while (m_server != nullptr && m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        // Read the request line once the browser has sent it. Kept minimal on purpose: one GET,
        // one capture, then teardown — this is a single-use relay, not an HTTP server.
        connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
            const QByteArray requestLine = socket->readLine(qint64{8} * 1024);
            // "GET /cb?loginToken=... HTTP/1.1" -> the query string after the first '?'.
            const qsizetype pathStart = requestLine.indexOf(' ');
            const qsizetype pathEnd = requestLine.indexOf(' ', pathStart + 1);
            QString callback;
            if (pathStart >= 0 && pathEnd > pathStart) {
                const QByteArray target = requestLine.mid(pathStart + 1, pathEnd - pathStart - 1);
                const qsizetype q = target.indexOf('?');
                if (q >= 0) {
                    callback = QString::fromUtf8(target.mid(q + 1));
                }
            }
            const QByteArray body = QByteArrayLiteral(
                "<!doctype html><meta charset=\"utf-8\"><title>daemon-app</title>"
                "<body style=\"font-family:sans-serif\"><p>Signed in — you can close this tab "
                "and return to daemon-app.</p></body>");
            socket->write(QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: text/html; "
                                            "charset=utf-8\r\nConnection: close\r\n"
                                            "Content-Length: ") +
                          QByteArray::number(body.size()) + QByteArrayLiteral("\r\n\r\n") + body);
            socket->flush();
            socket->disconnectFromHost();
            if (!callback.isEmpty()) {
                // One capture per start(): the flow id is single-use, so tear down before
                // relaying (a second hit would race a consumed flow).
                stop();
                emit captured(callback);
            }
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
#endif
}

} // namespace auth
