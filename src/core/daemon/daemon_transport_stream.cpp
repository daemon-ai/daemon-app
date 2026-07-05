// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// The length-framed STREAM carriers of DaemonTransport: the local Unix socket (QLocalSocket,
// plaintext) and the remote TCP socket wrapped in TLS (QSslSocket, fail-closed verification).
// Desktop-only: Qt-for-wasm lacks the `localserver` and `ssl` features, so this TU is excluded
// there and daemon_transport_stream_wasm.cpp supplies fail-closed stubs instead.

#include "daemon/daemon_transport.h"
#include "daemon/windows_pipe_name.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QLocalSocket>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>

namespace daemonapp::daemon {

namespace {

// Best-effort load of a PEM private key, trying the common algorithms (we don't know it a priori).
QSslKey loadPrivateKey(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray pem = f.readAll();
    for (const QSsl::KeyAlgorithm algo : {QSsl::Rsa, QSsl::Ec, QSsl::Dsa}) {
        QSslKey key(pem, algo, QSsl::Pem, QSsl::PrivateKey);
        if (!key.isNull()) {
            return key;
        }
    }
    return {};
}

} // namespace

bool DaemonTransport::streamSocketConnected() const {
    if (m_mode == Mode::Unix) {
        return m_local != nullptr && m_local->state() == QLocalSocket::ConnectedState;
    }
    return m_ssl != nullptr && m_ssl->state() == QAbstractSocket::ConnectedState &&
           m_ssl->isEncrypted();
}

void DaemonTransport::ensureStreamSocket() {
    if (m_io != nullptr) {
        return;
    }
    if (m_mode == Mode::Unix) {
        m_local = new QLocalSocket(this);
        m_io = m_local;
        connect(m_local, &QLocalSocket::connected, this, &DaemonTransport::onReady);
        connect(m_local, &QLocalSocket::readyRead, this, &DaemonTransport::handleReadyRead);
        connect(m_local, &QLocalSocket::errorOccurred, this,
                [this](QLocalSocket::LocalSocketError) {
                    const QString message = m_local->errorString();
                    close();
                    emit failed(message);
                });
        connect(m_local, &QLocalSocket::disconnected, this, [this] { emit disconnected(); });
        return;
    }
    // TCP + TLS.
    m_ssl = new QSslSocket(this);
    m_io = m_ssl;

    QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
    if (!m_tls.caFile.isEmpty()) {
        QList<QSslCertificate> cas = cfg.caCertificates();
        cas.append(QSslCertificate::fromPath(m_tls.caFile));
        cfg.setCaCertificates(cas);
    }
    if (clientCertConfigured()) {
        const QList<QSslCertificate> certs = QSslCertificate::fromPath(m_tls.clientCertFile);
        if (!certs.isEmpty()) {
            cfg.setLocalCertificate(certs.first());
        }
        const QSslKey key = loadPrivateKey(m_tls.clientKeyFile);
        if (!key.isNull()) {
            cfg.setPrivateKey(key);
        }
    }
    m_ssl->setSslConfiguration(cfg);

    // Flush only once the TLS handshake completes (not at TCP connect), so no bytes go in the
    // clear.
    connect(m_ssl, &QSslSocket::encrypted, this, &DaemonTransport::onReady);
    connect(m_ssl, &QSslSocket::readyRead, this, &DaemonTransport::handleReadyRead);
    connect(m_ssl, &QSslSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        const QString message = m_ssl->errorString();
        close();
        emit failed(message);
    });
    connect(m_ssl, &QSslSocket::disconnected, this, [this] { emit disconnected(); });
    // FAIL-CLOSED cert verification: only widen trust when explicitly configured.
    connect(m_ssl, &QSslSocket::sslErrors, this, [this](const QList<QSslError>& errors) {
        if (m_tls.insecureSkipVerify) {
            qWarning() << "DaemonTransport: TLS certificate verification DISABLED "
                          "(conn/tls/insecureSkipVerify) - do not use in production";
            m_ssl->ignoreSslErrors();
            return;
        }
        if (!m_tls.pinnedSha256.isEmpty()) {
            const QSslCertificate peer = m_ssl->peerCertificate();
            if (!peer.isNull()) {
                const QString got =
                    QString::fromLatin1(peer.digest(QCryptographicHash::Sha256).toHex());
                const QString want = m_tls.pinnedSha256.toLower().remove(QLatin1Char(':'));
                if (got.compare(want, Qt::CaseInsensitive) == 0) {
                    m_ssl->ignoreSslErrors(); // pinned leaf matches: accept (even self-signed)
                    return;
                }
            }
        }
        // Otherwise do NOT ignore: QSslSocket aborts the handshake and errorOccurred follows.
        Q_UNUSED(errors)
    });
}

void DaemonTransport::openUnix() {
    if (!hasTarget()) {
        emit failed(QStringLiteral("No daemon socket path configured"));
        return;
    }
    ensureSocket();
    if (m_local->state() == QLocalSocket::UnconnectedState) {
        m_buffer.clear();
        // On Windows QLocalSocket names are pipes; map the socket path to the daemon's bound pipe
        // name via the shared contract (localServerName). On Unix this is the path unchanged.
        m_local->connectToServer(localServerName(m_socketPath));
    }
}

void DaemonTransport::openTcp() {
    if (!hasTarget()) {
        emit failed(QStringLiteral("No daemon TCP target configured"));
        return;
    }
    ensureSocket();
    if (m_ssl->state() == QAbstractSocket::UnconnectedState) {
        m_buffer.clear();
        m_ssl->connectToHostEncrypted(m_host, m_port);
    }
}

void DaemonTransport::flushStreamSocket() {
    if (m_local != nullptr) {
        m_local->flush();
    } else if (m_ssl != nullptr) {
        m_ssl->flush();
    }
}

void DaemonTransport::resetStreamSockets() {
    if (m_local != nullptr) {
        m_local->abort();
        m_local->deleteLater();
        m_local = nullptr;
    }
    if (m_ssl != nullptr) {
        m_ssl->abort();
        m_ssl->deleteLater();
        m_ssl = nullptr;
    }
}

} // namespace daemonapp::daemon
