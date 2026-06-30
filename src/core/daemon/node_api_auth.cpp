// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/node_api_auth.h"

#include <QCryptographicHash>
#include <QList>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include <utility>

namespace daemonapp::daemon {

namespace scram {

QByteArray hmacSha256(const QByteArray& key, const QByteArray& message) {
    QMessageAuthenticationCode mac(QCryptographicHash::Sha256);
    mac.setKey(key);
    mac.addData(message);
    return mac.result();
}

QByteArray sha256(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QByteArray xorBytes(const QByteArray& a, const QByteArray& b) {
    const qsizetype n = qMin(a.size(), b.size());
    QByteArray out(n, Qt::Uninitialized);
    for (qsizetype i = 0; i < n; ++i) {
        out[i] = static_cast<char>(static_cast<quint8>(a[i]) ^ static_cast<quint8>(b[i]));
    }
    return out;
}

QByteArray pbkdf2HmacSha256(const QByteArray& password, const QByteArray& salt, int iterations,
                            int dkLen) {
    // RFC 8018 PBKDF2 with HMAC-SHA-256 (hLen = 32). dkLen here is always 32 (one block) for SCRAM,
    // but the loop is general so the helper is reusable / vector-testable.
    constexpr int kHLen = 32;
    QByteArray derived;
    const int blocks = (dkLen + kHLen - 1) / kHLen;
    for (int block = 1; block <= blocks; ++block) {
        QByteArray indexBlock(4, '\0');
        indexBlock[0] = static_cast<char>((block >> 24) & 0xFF);
        indexBlock[1] = static_cast<char>((block >> 16) & 0xFF);
        indexBlock[2] = static_cast<char>((block >> 8) & 0xFF);
        indexBlock[3] = static_cast<char>(block & 0xFF);
        QByteArray u = hmacSha256(password, salt + indexBlock);
        QByteArray t = u;
        for (int c = 1; c < iterations; ++c) {
            u = hmacSha256(password, u);
            t = xorBytes(t, u);
        }
        derived.append(t);
    }
    derived.truncate(dkLen);
    return derived;
}

} // namespace scram

namespace {

// Parse a SCRAM message ("k=v,k=v,...") into first-char -> value (value is everything after the
// first '='). Sufficient for the attributes we read (r/s/i/v).
QByteArray scramAttr(const QByteArray& message, char key) {
    const QList<QByteArray> parts = message.split(',');
    for (const QByteArray& part : parts) {
        if (part.size() >= 2 && part[0] == key && part[1] == '=') {
            return part.mid(2);
        }
    }
    return {};
}

// SCRAM username escaping (RFC 5802 §5.1): ',' -> "=2C", '=' -> "=3D". (Full SASLprep is out of
// scope; documented in the header.)
QByteArray scramEscapeUsername(const QString& username) {
    QByteArray out;
    for (const char c : username.toUtf8()) {
        if (c == ',') {
            out.append("=2C");
        } else if (c == '=') {
            out.append("=3D");
        } else {
            out.append(c);
        }
    }
    return out;
}

} // namespace

// --- PLAIN ---------------------------------------------------------------------------------------

PlainMechanism::PlainMechanism(QString username, QString password)
    : m_username(std::move(username)), m_password(std::move(password)) {}

QByteArray PlainMechanism::initialResponse() {
    // authzid (empty) NUL authcid NUL passwd.
    QByteArray out;
    out.append('\0');
    out.append(m_username.toUtf8());
    out.append('\0');
    out.append(m_password.toUtf8());
    return out;
}

SaslStep PlainMechanism::step(const QByteArray& /*challenge*/) {
    // PLAIN is single-step; a challenge means the server is doing something unexpected -> fail.
    return SaslStep{SaslStep::Kind::Failed, {}};
}

// --- EXTERNAL ------------------------------------------------------------------------------------

SaslStep ExternalMechanism::step(const QByteArray& /*challenge*/) {
    return SaslStep{SaslStep::Kind::Failed, {}};
}

// --- SCRAM-SHA-256 -------------------------------------------------------------------------------

ScramSha256Mechanism::ScramSha256Mechanism(QString username, QString password,
                                           QByteArray clientNonce)
    : m_username(std::move(username)), m_password(std::move(password)),
      m_clientNonce(std::move(clientNonce)) {
    if (m_clientNonce.isEmpty()) {
        // 24 random bytes -> base64 printable nonce (no comma; SCRAM-safe).
        QByteArray raw(24, Qt::Uninitialized);
        QRandomGenerator::system()->generate(raw.begin(), raw.end());
        m_clientNonce = raw.toBase64(QByteArray::Base64Encoding | QByteArray::OmitTrailingEquals);
    }
}

QByteArray ScramSha256Mechanism::initialResponse() {
    m_clientFirstBare = "n=" + scramEscapeUsername(m_username) + ",r=" + m_clientNonce;
    m_state = State::AwaitingServerFirst;
    return "n,," + m_clientFirstBare; // gs2-header "n,," + client-first-bare
}

SaslStep ScramSha256Mechanism::step(const QByteArray& challenge) {
    if (m_state == State::AwaitingServerFirst) {
        const QByteArray combinedNonce = scramAttr(challenge, 'r');
        const QByteArray saltB64 = scramAttr(challenge, 's');
        const QByteArray iterStr = scramAttr(challenge, 'i');
        if (combinedNonce.isEmpty() || saltB64.isEmpty() || iterStr.isEmpty() ||
            !combinedNonce.startsWith(m_clientNonce)) {
            return SaslStep{SaslStep::Kind::Failed, {}}; // missing attrs / nonce mismatch
        }
        bool ok = false;
        const int iterations = iterStr.toInt(&ok);
        if (!ok || iterations <= 0) {
            return SaslStep{SaslStep::Kind::Failed, {}};
        }
        const QByteArray salt = QByteArray::fromBase64(saltB64);

        const QByteArray saltedPassword =
            scram::pbkdf2HmacSha256(m_password.toUtf8(), salt, iterations, 32);
        const QByteArray clientKey = scram::hmacSha256(saltedPassword, "Client Key");
        const QByteArray storedKey = scram::sha256(clientKey);
        const QByteArray clientFinalNoProof = "c=biws,r=" + combinedNonce;
        const QByteArray authMessage =
            m_clientFirstBare + "," + challenge + "," + clientFinalNoProof;
        const QByteArray clientSignature = scram::hmacSha256(storedKey, authMessage);
        const QByteArray clientProof = scram::xorBytes(clientKey, clientSignature);

        const QByteArray serverKey = scram::hmacSha256(saltedPassword, "Server Key");
        m_expectedServerSig = scram::hmacSha256(serverKey, authMessage);

        m_state = State::AwaitingServerFinal;
        const QByteArray clientFinal = clientFinalNoProof + ",p=" + clientProof.toBase64();
        return SaslStep{SaslStep::Kind::Respond, clientFinal};
    }
    if (m_state == State::AwaitingServerFinal) {
        // server-final: "v=<base64 ServerSignature>". Verify, send nothing, await AuthOk.
        const QByteArray v = scramAttr(challenge, 'v');
        if (v.isEmpty()) {
            return SaslStep{SaslStep::Kind::Failed, {}};
        }
        const QByteArray got = QByteArray::fromBase64(v);
        m_state = State::Done;
        if (got != m_expectedServerSig) {
            return SaslStep{SaslStep::Kind::Failed, {}}; // server-signature mismatch (fail-closed)
        }
        return SaslStep{SaslStep::Kind::Complete, {}};
    }
    return SaslStep{SaslStep::Kind::Failed, {}};
}

// --- mechanism selection -------------------------------------------------------------------------

std::unique_ptr<SaslMechanism> chooseSaslMechanism(const QStringList& serverMechanisms,
                                                   const QString& username, const QString& password,
                                                   bool tlsActive, bool hasClientCert) {
    const bool haveCreds = !username.isEmpty() && !password.isEmpty();
    // Preference order: EXTERNAL (mTLS) > SCRAM-SHA-256 > PLAIN (TLS only).
    if (hasClientCert && serverMechanisms.contains(QStringLiteral("EXTERNAL"))) {
        return std::make_unique<ExternalMechanism>();
    }
    if (haveCreds && serverMechanisms.contains(QStringLiteral("SCRAM-SHA-256"))) {
        return std::make_unique<ScramSha256Mechanism>(username, password);
    }
    if (haveCreds && tlsActive && serverMechanisms.contains(QStringLiteral("PLAIN"))) {
        return std::make_unique<PlainMechanism>(username, password);
    }
    return nullptr;
}

} // namespace daemonapp::daemon
