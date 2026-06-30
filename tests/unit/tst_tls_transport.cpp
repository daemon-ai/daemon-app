// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_transport.h"

#include <QCryptographicHash>
#include <QHostAddress>
#include <QList>
#include <QProcess>
#include <QSignalSpy>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslServer>
#include <QSslSocket>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::TlsConfig;

namespace {

// A tiny TLS server that, once encrypted, echoes a fixed framed reply for every frame it receives.
class TlsEchoServer : public QObject {
public:
    bool start(const QString& certPath, const QString& keyPath) {
        QFile certFile(certPath);
        QFile keyFile(keyPath);
        if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
            return false;
        }
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        cfg.setLocalCertificate(QSslCertificate(certFile.readAll(), QSsl::Pem));
        cfg.setPrivateKey(QSslKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey));
        m_server.setSslConfiguration(cfg);
        connect(&m_server, &QTcpServer::pendingConnectionAvailable, this, [this] {
            while (m_server.hasPendingConnections()) {
                auto* sock = qobject_cast<QSslSocket*>(m_server.nextPendingConnection());
                if (sock == nullptr) {
                    continue;
                }
                m_socks.append(sock);
                connect(sock, &QSslSocket::readyRead, sock, [this, sock] {
                    m_buf.append(sock->readAll());
                    QByteArray payload;
                    while (DaemonTransport::tryTakeFrame(m_buf, &payload)) {
                        sock->write(DaemonTransport::framePayload(QByteArrayLiteral("\x62Ok")));
                        sock->flush();
                    }
                });
            }
        });
        return m_server.listen(QHostAddress::LocalHost, 0);
    }
    [[nodiscard]] quint16 port() const { return m_server.serverPort(); }

private:
    QSslServer m_server;
    QList<QSslSocket*> m_socks;
    QByteArray m_buf;
};

} // namespace

// The TLS/TCP transport: fail-closed by default (an untrusted self-signed cert aborts), and usable
// only when trust is explicitly widened (a SHA-256 pin, or the loud insecureSkipVerify escape).
//
// The self-signed cert+key are generated fresh at test time with openssl (in the devShell) rather
// than embedded, so no private-key material is ever committed (the secret-scan gate stays clean).
class TestTlsTransport : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        if (!QSslSocket::supportsSsl()) {
            QSKIP("No TLS backend available in this build");
        }
        const QString openssl = QStandardPaths::findExecutable(QStringLiteral("openssl"));
        if (openssl.isEmpty()) {
            QSKIP("openssl not on PATH; cannot generate a test certificate");
        }
        QVERIFY(m_dir.isValid());
        m_certPath = m_dir.filePath(QStringLiteral("cert.pem"));
        m_keyPath = m_dir.filePath(QStringLiteral("key.pem"));
        QProcess p;
        p.start(openssl, {QStringLiteral("req"), QStringLiteral("-x509"), QStringLiteral("-newkey"),
                          QStringLiteral("rsa:2048"), QStringLiteral("-keyout"), m_keyPath,
                          QStringLiteral("-out"), m_certPath, QStringLiteral("-days"),
                          QStringLiteral("2"), QStringLiteral("-nodes"), QStringLiteral("-subj"),
                          QStringLiteral("/CN=localhost")});
        QVERIFY2(p.waitForFinished(15000) && p.exitCode() == 0, "openssl cert generation failed");

        QFile certFile(m_certPath);
        QVERIFY(certFile.open(QIODevice::ReadOnly));
        const QSslCertificate cert(certFile.readAll(), QSsl::Pem);
        QVERIFY(!cert.isNull());
        m_pin = QString::fromLatin1(cert.digest(QCryptographicHash::Sha256).toHex());
    }

    // Default verification rejects the self-signed cert: the transport emits failed and never
    // delivers a frame.
    void untrustedCertIsRejected() {
        TlsEchoServer server;
        QVERIFY(server.start(m_certPath, m_keyPath));

        DaemonTransport transport;
        transport.setTcpTarget(QStringLiteral("localhost"), server.port(), TlsConfig{});
        QSignalSpy failed(&transport, &DaemonTransport::failed);
        QSignalSpy connected(&transport, &DaemonTransport::connected);
        QSignalSpy frames(&transport, &DaemonTransport::frameReceived);

        transport.sendFrame(QByteArrayLiteral("\x62Ok"));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() >= 1, 5000);
        QCOMPARE(connected.count(), 0);
        QCOMPARE(frames.count(), 0);
        QVERIFY(!transport.isConnected());
    }

    // A matching SHA-256 pin accepts the (otherwise untrusted) cert: the handshake completes and an
    // encrypted frame round-trips.
    void pinnedCertConnects() {
        TlsEchoServer server;
        QVERIFY(server.start(m_certPath, m_keyPath));

        TlsConfig tls;
        tls.pinnedSha256 = m_pin;
        DaemonTransport transport;
        transport.setTcpTarget(QStringLiteral("localhost"), server.port(), tls);
        QSignalSpy connected(&transport, &DaemonTransport::connected);
        QSignalSpy frames(&transport, &DaemonTransport::frameReceived);

        transport.sendFrame(QByteArrayLiteral("\x62Ok"));
        QTRY_VERIFY_WITH_TIMEOUT(connected.count() >= 1, 5000);
        QVERIFY(transport.tlsActive());
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() >= 1, 5000);
    }

    // The insecureSkipVerify escape hatch connects despite the untrusted cert (dev only; loud).
    void insecureSkipVerifyConnects() {
        TlsEchoServer server;
        QVERIFY(server.start(m_certPath, m_keyPath));

        TlsConfig tls;
        tls.insecureSkipVerify = true;
        DaemonTransport transport;
        transport.setTcpTarget(QStringLiteral("localhost"), server.port(), tls);
        QSignalSpy connected(&transport, &DaemonTransport::connected);

        transport.sendFrame(QByteArrayLiteral("\x62Ok"));
        QTRY_VERIFY_WITH_TIMEOUT(connected.count() >= 1, 5000);
    }

    // A wrong pin does not widen trust: still fail-closed.
    void wrongPinIsRejected() {
        TlsEchoServer server;
        QVERIFY(server.start(m_certPath, m_keyPath));

        TlsConfig tls;
        tls.pinnedSha256 = QStringLiteral("00000000000000000000000000000000"
                                          "00000000000000000000000000000000");
        DaemonTransport transport;
        transport.setTcpTarget(QStringLiteral("localhost"), server.port(), tls);
        QSignalSpy failed(&transport, &DaemonTransport::failed);
        QSignalSpy connected(&transport, &DaemonTransport::connected);

        transport.sendFrame(QByteArrayLiteral("\x62Ok"));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() >= 1, 5000);
        QCOMPARE(connected.count(), 0);
    }

private:
    QTemporaryDir m_dir;
    QString m_certPath;
    QString m_keyPath;
    QString m_pin;
};

QTEST_MAIN(TestTlsTransport)
#include "tst_tls_transport.moc"
