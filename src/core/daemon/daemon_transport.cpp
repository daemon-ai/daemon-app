// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_transport.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QLocalSocket>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslSocket>
#include <QtEndian>

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

bool DaemonTransport::isConnected() const {
    if (m_mode == Mode::Unix) {
        return m_local != nullptr && m_local->state() == QLocalSocket::ConnectedState;
    }
    return m_ssl != nullptr && m_ssl->state() == QAbstractSocket::ConnectedState &&
           m_ssl->isEncrypted();
}

void DaemonTransport::ensureSocket() {
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

void DaemonTransport::open() {
    if (m_mode == Mode::Unix) {
        openUnix();
    } else {
        openTcp();
    }
}

void DaemonTransport::openUnix() {
    if (m_socketPath.isEmpty()) {
        emit failed(QStringLiteral("No daemon socket path configured"));
        return;
    }
    ensureSocket();
    if (m_local->state() == QLocalSocket::UnconnectedState) {
        m_buffer.clear();
        m_local->connectToServer(m_socketPath);
    }
}

void DaemonTransport::openTcp() {
    if (m_host.isEmpty() || m_port == 0) {
        emit failed(QStringLiteral("No daemon TCP target configured"));
        return;
    }
    ensureSocket();
    if (m_ssl->state() == QAbstractSocket::UnconnectedState) {
        m_buffer.clear();
        m_ssl->connectToHostEncrypted(m_host, m_port);
    }
}

void DaemonTransport::onReady() {
    flushOutbox();
    emit connected();
}

void DaemonTransport::resetSocket() {
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
    m_io = nullptr;
}

void DaemonTransport::close() {
    resetSocket();
    m_buffer.clear();
    m_outbox.clear();
}

void DaemonTransport::flushOutbox() {
    if (m_io == nullptr) {
        return;
    }
    const QList<QByteArray> pending = m_outbox;
    m_outbox.clear();
    for (const QByteArray& frame : pending) {
        m_io->write(framePayload(frame));
    }
    flushSocket();
}

void DaemonTransport::flushSocket() {
    if (m_local != nullptr) {
        m_local->flush();
    } else if (m_ssl != nullptr) {
        m_ssl->flush();
    }
}

void DaemonTransport::sendFrame(const QByteArray& cborPayload) {
    const bool noTarget = (m_mode == Mode::Unix && m_socketPath.isEmpty()) ||
                          (m_mode == Mode::Tcp && (m_host.isEmpty() || m_port == 0));
    if (noTarget) {
        emit failed(QStringLiteral("No daemon target configured"));
        return;
    }
    ensureSocket();
    if (isConnected()) {
        m_io->write(framePayload(cborPayload));
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
