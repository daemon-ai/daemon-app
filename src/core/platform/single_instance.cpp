// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/single_instance.h"

#include <QCryptographicHash>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>

namespace platform {

SingleInstanceGuard::SingleInstanceGuard(QString key, QObject* parent)
    : QObject(parent), m_key(std::move(key)) {}

QString SingleInstanceGuard::defaultKey() {
    // Hash the home directory so arbitrary user names stay socket/pipe-safe;
    // this is namespacing, not security (the socket itself is user-access).
    const QByteArray home =
        QCryptographicHash::hash(QDir::homePath().toUtf8(), QCryptographicHash::Sha256).toHex();
    return QStringLiteral("daemon-app-instance-") + QString::fromLatin1(home.left(12));
}

bool SingleInstanceGuard::tryClaim() {
    // Probe for a live primary FIRST. With socket options set, QLocalServer's
    // unix implementation binds a temp socket and rename()s it over the
    // existing file, so a second listen() would silently steal the name
    // instead of failing - listen success proves nothing. Only an answered
    // connect proves a primary.
    {
        QLocalSocket probe;
        probe.connectToServer(m_key);
        if (probe.waitForConnected(250)) {
            probe.disconnectFromServer();
            return false;
        }
    }
    // No live primary: clear a crashed primary's stale unix socket (connect
    // refused, but the file blocks a plain bind) and take the name.
    QLocalServer::removeServer(m_key);
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_server->listen(m_key)) {
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket* client = m_server->nextPendingConnection()) {
            connect(client, &QLocalSocket::readyRead, this, [this, client] {
                if (client->readAll().contains("raise")) {
                    emit activateRequested();
                }
                client->disconnectFromServer();
            });
            connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        }
    });
    return true;
}

bool SingleInstanceGuard::notifyPrimary(bool raise, int timeoutMs) {
    QLocalSocket socket;
    socket.connectToServer(m_key);
    if (!socket.waitForConnected(timeoutMs)) {
        return false;
    }
    socket.write(raise ? "raise\n" : "ping\n");
    socket.flush();
    socket.waitForBytesWritten(timeoutMs);
    socket.disconnectFromServer();
    return true;
}

} // namespace platform
