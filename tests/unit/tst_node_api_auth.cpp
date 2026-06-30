// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/node_api_auth.h"

#include <memory>
#include <QtTest/QtTest>

using daemonapp::daemon::chooseSaslMechanism;
using daemonapp::daemon::ExternalMechanism;
using daemonapp::daemon::PlainMechanism;
using daemonapp::daemon::SaslStep;
using daemonapp::daemon::ScramSha256Mechanism;
namespace scram = daemonapp::daemon::scram;

// The SCRAM-SHA-256 client mechanism + crypto helpers, checked against the RFC 7677 §3 worked
// example so the client is verifiable without a live server (Auth 3). Also covers the server-final
// (v=) verification (the coordinator's authoritative framing: the client MUST verify v= and accept
// no response to it) and the PLAIN/EXTERNAL single-step shapes + selection policy.
class TestNodeApiAuth : public QObject {
    Q_OBJECT

private slots:
    // PBKDF2-HMAC-SHA256 against the SaltedPassword implied by RFC 7677 (salt
    // "W22ZaJ0SNY7soEsUEjb6gQ==", i=4096, password "pencil"). We derive it indirectly via the full
    // proof below; here we just assert the primitive is deterministic and the right length.
    void pbkdf2IsDeterministic() {
        const QByteArray salt = QByteArray::fromBase64("W22ZaJ0SNY7soEsUEjb6gQ==");
        const QByteArray a = scram::pbkdf2HmacSha256("pencil", salt, 4096, 32);
        const QByteArray b = scram::pbkdf2HmacSha256("pencil", salt, 4096, 32);
        QCOMPARE(a.size(), 32);
        QCOMPARE(a, b);
    }

    // The canonical RFC 7677 SCRAM-SHA-256 exchange with the example's fixed client nonce. The
    // client-first, client-final (incl. the exact ClientProof) and the server-final verification
    // must all match the RFC byte-for-byte.
    void scramRfc7677Vector() {
        ScramSha256Mechanism mech(QStringLiteral("user"), QStringLiteral("pencil"),
                                  QByteArrayLiteral("rOprNGfwEbeRWgbNEkqO"));
        QCOMPARE(mech.mechanismName(), QStringLiteral("SCRAM-SHA-256"));
        QCOMPARE(mech.initialResponse(), QByteArrayLiteral("n,,n=user,r=rOprNGfwEbeRWgbNEkqO"));

        const QByteArray serverFirst =
            QByteArrayLiteral("r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s="
                              "W22ZaJ0SNY7soEsUEjb6gQ==,i=4096");
        const SaslStep s1 = mech.step(serverFirst);
        QCOMPARE(s1.kind, SaslStep::Kind::Respond);
        QCOMPARE(s1.response,
                 QByteArrayLiteral("c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,"
                                   "p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ="));

        // server-final: the client must verify v= and then accept NO further response.
        const SaslStep s2 =
            mech.step(QByteArrayLiteral("v=6rriTRBi23WpRR/wtup+mMhUZUn/dB5nLTJRsjl95G4="));
        QCOMPARE(s2.kind, SaslStep::Kind::Complete);
        QVERIFY(s2.response.isEmpty());
    }

    // A tampered server signature must fail closed (the connection is rejected).
    void scramRejectsBadServerSignature() {
        ScramSha256Mechanism mech(QStringLiteral("user"), QStringLiteral("pencil"),
                                  QByteArrayLiteral("rOprNGfwEbeRWgbNEkqO"));
        QVERIFY(!mech.initialResponse().isEmpty());
        const SaslStep s1 =
            mech.step(QByteArrayLiteral("r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,s="
                                        "W22ZaJ0SNY7soEsUEjb6gQ==,i=4096"));
        QCOMPARE(s1.kind, SaslStep::Kind::Respond);
        const SaslStep s2 =
            mech.step(QByteArrayLiteral("v=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="));
        QCOMPARE(s2.kind, SaslStep::Kind::Failed);
    }

    // A server nonce that does not extend our client nonce is a protocol violation.
    void scramRejectsNonceMismatch() {
        ScramSha256Mechanism mech(QStringLiteral("user"), QStringLiteral("pencil"),
                                  QByteArrayLiteral("rOprNGfwEbeRWgbNEkqO"));
        QVERIFY(!mech.initialResponse().isEmpty());
        const SaslStep s1 =
            mech.step(QByteArrayLiteral("r=DIFFERENTNONCE,s=W22ZaJ0SNY7soEsUEjb6gQ==,i=4096"));
        QCOMPARE(s1.kind, SaslStep::Kind::Failed);
    }

    // PLAIN: "\0user\0pass"; any challenge is a protocol error (single-step).
    void plainInitialResponse() {
        PlainMechanism mech(QStringLiteral("alice"), QStringLiteral("s3cr3t"));
        QByteArray expected;
        expected.append('\0');
        expected.append("alice");
        expected.append('\0');
        expected.append("s3cr3t");
        QCOMPARE(mech.initialResponse(), expected);
        QCOMPARE(mech.step(QByteArrayLiteral("anything")).kind, SaslStep::Kind::Failed);
    }

    // EXTERNAL: empty initial response (the cert fingerprint maps server-side).
    void externalInitialResponseIsEmpty() {
        ExternalMechanism mech;
        QVERIFY(mech.initialResponse().isEmpty());
    }

    // Selection policy: EXTERNAL needs a client cert; PLAIN needs TLS; SCRAM needs creds; nothing
    // usable -> nullptr (the client then fails closed and shows the login form).
    void mechanismSelection() {
        const QStringList all{QStringLiteral("SCRAM-SHA-256"), QStringLiteral("EXTERNAL"),
                              QStringLiteral("PLAIN")};
        // Prefer EXTERNAL when a client cert is present.
        QCOMPARE(chooseSaslMechanism(all, QString(), QString(), true, true)->mechanismName(),
                 QStringLiteral("EXTERNAL"));
        // SCRAM with username/password (no cert).
        QCOMPARE(chooseSaslMechanism(all, QStringLiteral("u"), QStringLiteral("p"), true, false)
                     ->mechanismName(),
                 QStringLiteral("SCRAM-SHA-256"));
        // PLAIN only when TLS is active and SCRAM isn't offered.
        const QStringList plainOnly{QStringLiteral("PLAIN")};
        QVERIFY(chooseSaslMechanism(plainOnly, QStringLiteral("u"), QStringLiteral("p"),
                                    /*tls=*/false, false) == nullptr);
        QCOMPARE(chooseSaslMechanism(plainOnly, QStringLiteral("u"), QStringLiteral("p"),
                                     /*tls=*/true, false)
                     ->mechanismName(),
                 QStringLiteral("PLAIN"));
        // No credentials and no cert -> nothing usable.
        QVERIFY(chooseSaslMechanism(all, QString(), QString(), true, false) == nullptr);
    }
};

QTEST_GUILESS_MAIN(TestNodeApiAuth)
#include "tst_node_api_auth.moc"
