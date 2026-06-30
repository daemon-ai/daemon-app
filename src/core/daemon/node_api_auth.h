// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <memory>
#include <QByteArray>
#include <QString>
#include <QStringList>

// Client-side SASL mechanisms for the v2 wire handshake (daemon-access-control.md §2). These are
// PURE (no socket / Qt event loop): the NodeApiClient drives them, feeding AuthChallenge bytes in
// and sending the produced AuthStart/AuthStep bytes back out. Keeping them transport-free lets the
// SCRAM crypto be unit-tested against the RFC 7677 / RFC 5802 vectors with no server.
//
// Authoritative SCRAM-SHA-256 framing (coordinator decision OPEN-1), which the server matches:
//   AuthStart{client-first} -> AuthChallenge{server-first} -> AuthStep{client-final}
//     -> AuthChallenge{server-final v=} -> AuthOk{token, principal}
// The server sends the server-final as the LAST AuthChallenge then AuthOk back-to-back; the client
// MUST verify the server signature (v=) from that final challenge, send NO response to it, and only
// then accept AuthOk. PLAIN and EXTERNAL are single-step (AuthStart -> AuthOk/AuthError).

namespace daemonapp::daemon {

// --- SCRAM crypto primitives (pure; RFC-vector tested) -------------------------------------------
namespace scram {
QByteArray hmacSha256(const QByteArray& key, const QByteArray& message);
QByteArray sha256(const QByteArray& data);
// PBKDF2-HMAC-SHA256 (Qt has no PBKDF2; hand-rolled over QMessageAuthenticationCode).
QByteArray pbkdf2HmacSha256(const QByteArray& password, const QByteArray& salt, int iterations,
                            int dkLen);
QByteArray xorBytes(const QByteArray& a, const QByteArray& b);
} // namespace scram

// The result of feeding one AuthChallenge to a mechanism.
struct SaslStep {
    enum class Kind {
        Respond,  // send AuthStep{response}, await the next challenge / AuthOk
        Complete, // verification done (server-final ok); send NOTHING, await AuthOk
        Failed,   // protocol / verification error; abort the connection (fail-closed)
    };
    Kind kind = Kind::Failed;
    QByteArray response; // valid iff kind == Respond
};

// A client SASL mechanism. `initialResponse()` is the AuthStart payload; `step()` consumes each
// AuthChallenge until Complete/Failed (PLAIN/EXTERNAL never see a challenge in the normal flow).
class SaslMechanism {
public:
    virtual ~SaslMechanism() = default;
    [[nodiscard]] virtual QString mechanismName() const = 0;
    [[nodiscard]] virtual QByteArray initialResponse() = 0;
    [[nodiscard]] virtual SaslStep step(const QByteArray& challenge) = 0;
};

// PLAIN: initial response is "\0<username>\0<password>". Single-step; ANY challenge is a protocol
// error. The selector only offers PLAIN over an active TLS transport (never on plaintext).
class PlainMechanism : public SaslMechanism {
public:
    PlainMechanism(QString username, QString password);
    [[nodiscard]] QString mechanismName() const override { return QStringLiteral("PLAIN"); }
    [[nodiscard]] QByteArray initialResponse() override;
    [[nodiscard]] SaslStep step(const QByteArray& challenge) override;

private:
    QString m_username;
    QString m_password;
};

// EXTERNAL: empty initial response (authzid omitted; the server maps the mTLS client-cert
// fingerprint). Single-step; the selector only offers it when a client certificate is configured.
class ExternalMechanism : public SaslMechanism {
public:
    [[nodiscard]] QString mechanismName() const override { return QStringLiteral("EXTERNAL"); }
    [[nodiscard]] QByteArray initialResponse() override { return {}; }
    [[nodiscard]] SaslStep step(const QByteArray& challenge) override;
};

// SCRAM-SHA-256 (RFC 5802 + RFC 7677). The only multi-challenge mechanism.
class ScramSha256Mechanism : public SaslMechanism {
public:
    // `clientNonce` is for tests (RFC vectors); empty => a fresh random nonce is generated.
    ScramSha256Mechanism(QString username, QString password, QByteArray clientNonce = {});
    [[nodiscard]] QString mechanismName() const override { return QStringLiteral("SCRAM-SHA-256"); }
    [[nodiscard]] QByteArray initialResponse() override;
    [[nodiscard]] SaslStep step(const QByteArray& challenge) override;

private:
    enum class State { Initial, AwaitingServerFirst, AwaitingServerFinal, Done };

    QString m_username;
    QString m_password;
    QByteArray m_clientNonce;     // base64 token
    QByteArray m_clientFirstBare; // "n=user,r=cnonce"
    QByteArray m_expectedServerSig;
    State m_state = State::Initial;
};

// Choose the best client mechanism from the server's offered list, honoring what credentials /
// transport allow. Returns nullptr when nothing is usable (the caller then surfaces "credentials
// required" and stays disconnected). Preference: EXTERNAL (mTLS) > SCRAM-SHA-256 > PLAIN (TLS
// only).
std::unique_ptr<SaslMechanism> chooseSaslMechanism(const QStringList& serverMechanisms,
                                                   const QString& username, const QString& password,
                                                   bool tlsActive, bool hasClientCert);

} // namespace daemonapp::daemon
