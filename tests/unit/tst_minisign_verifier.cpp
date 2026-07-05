// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/minisign_verifier.h"

#include <QtTest>

using update::MinisignVerifier;

// Verifies the minisign implementation against real fixtures generated with the
// upstream `minisign` tool (prehashed ED signature, trusted comment
// "daemon stable 9.9.9"). Covers the happy path plus every tamper class the
// threat model in packaging/UPDATES.md calls out.
class MinisignVerifierTests : public QObject {
    Q_OBJECT

private:
    // The exact signed bytes.
    static QByteArray message() { return QByteArrayLiteral("PAYLOAD-daemon-manifest-v9.9.9"); }
    // The matching pinned public key.
    static QString pubKey() {
        return QStringLiteral("RWRKQPBgmqc5elwDEVaAjs5W26k60Fh3l8BdddaZHxJXdolsJzMskoHS");
    }
    // A different key (the manifest was NOT signed with it).
    static QString wrongKey() {
        return QStringLiteral("RWTL2lrL4IhRMC7W6E19BydZb3gGvcHMAbgcCiNaFgRb25vteCOf06FE");
    }
    // The detached signature file (4 lines).
    static QByteArray minisig() {
        return QByteArrayLiteral("untrusted comment: signature from minisign secret key\n"
                                 "RURKQPBgmqc5elBX+W82YhsGe0dtCMX/Cz/"
                                 "p46Umy2AIEDG1M6FyEZ1fMCu11Rm5SD3ruq+JCPjfpCJ2QV1JSaF"
                                 "HNurDii9iXA4=\n"
                                 "trusted comment: daemon stable 9.9.9\n"
                                 "aaSfs5FeDCyfbLnuHHjotlphBgGzmrByggrZak4tcFD09pD3OX0LCd6dUAgQhR/"
                                 "gMH0bheOOH67RYqv+xp2gAQ==\n");
    }

private slots:
    void validSignature();
    void wrongKeyRejected();
    void tamperedManifestRejected();
    void tamperedTrustedCommentRejected();
    void truncatedSignatureRejected();
    void legacyAlgorithmRejected();
    void malformedKeyRejected();
};

void MinisignVerifierTests::validSignature() {
    const MinisignVerifier::Result r = MinisignVerifier::verify(message(), minisig(), pubKey());
    QVERIFY2(r.ok, qPrintable(r.error));
    QCOMPARE(r.trustedComment, QStringLiteral("daemon stable 9.9.9"));
}

void MinisignVerifierTests::wrongKeyRejected() {
    const MinisignVerifier::Result r = MinisignVerifier::verify(message(), minisig(), wrongKey());
    QVERIFY(!r.ok);
    QVERIFY(!r.error.isEmpty());
}

void MinisignVerifierTests::tamperedManifestRejected() {
    QByteArray tampered = message();
    tampered[0] = static_cast<char>(tampered.at(0) ^ 0x01);
    const MinisignVerifier::Result r = MinisignVerifier::verify(tampered, minisig(), pubKey());
    QVERIFY(!r.ok);
}

void MinisignVerifierTests::tamperedTrustedCommentRejected() {
    QByteArray sig = minisig();
    // Flip the version in the trusted comment: the global signature no longer
    // authenticates it.
    sig.replace(QByteArrayLiteral("daemon stable 9.9.9"), QByteArrayLiteral("daemon stable 9.9.8"));
    const MinisignVerifier::Result r = MinisignVerifier::verify(message(), sig, pubKey());
    QVERIFY(!r.ok);
}

void MinisignVerifierTests::truncatedSignatureRejected() {
    QByteArray sig = minisig();
    // Chop the tail off the signature blob (line 2) - a malformed base64 length.
    const QList<QByteArray> lines = sig.split('\n');
    QByteArray truncated =
        lines.at(0) + '\n' + lines.at(1).left(20) + '\n' + lines.at(2) + '\n' + lines.at(3) + '\n';
    const MinisignVerifier::Result r = MinisignVerifier::verify(message(), truncated, pubKey());
    QVERIFY(!r.ok);
}

void MinisignVerifierTests::legacyAlgorithmRejected() {
    // Rewrite the signature blob's algorithm from prehashed "ED" (0x45 0x44) to
    // legacy pure "Ed" (0x45 0x64) and re-encode; the verifier must reject it.
    const QList<QByteArray> lines = minisig().split('\n');
    const QByteArray::FromBase64Result decoded = QByteArray::fromBase64Encoding(lines.at(1));
    QVERIFY(decoded.decodingStatus == QByteArray::Base64DecodingStatus::Ok);
    QByteArray blob = *decoded;
    blob[1] = 'd'; // "ED" -> "Ed"
    const QByteArray legacyLine = blob.toBase64();
    const QByteArray sig =
        lines.at(0) + '\n' + legacyLine + '\n' + lines.at(2) + '\n' + lines.at(3) + '\n';
    const MinisignVerifier::Result r = MinisignVerifier::verify(message(), sig, pubKey());
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains(QStringLiteral("algorithm")));
}

void MinisignVerifierTests::malformedKeyRejected() {
    const MinisignVerifier::Result r =
        MinisignVerifier::verify(message(), minisig(), QStringLiteral("not-a-key"));
    QVERIFY(!r.ok);
    QVERIFY(r.error.contains(QStringLiteral("public key")));
}

QTEST_MAIN(MinisignVerifierTests)
#include "tst_minisign_verifier.moc"
