// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon/daemon_transport.h"
#include "daemon/node_api_auth.h"
#include "daemon/node_api_codec.h"

#include <memory>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace daemonapp::daemon {

// Multiplexed NodeApi client over the length-framed Unix-socket transport
// (daemon-sync-protocol-spec §2). On (re)connect it performs a `Hello` handshake, then wraps each
// request in a correlated envelope frame so many exchanges share one connection without
// head-of-line blocking:
//
//   - sendRequest()  -> a one-shot `Call`; the single `Reply` is delivered via responseReady().
//   - openStream()   -> an `Open`; the server pushes `Item`s (streamItem) until `End`
//   (streamEnded).
//
// The public one-shot API (responseReady / failed, keyed by a caller correlation id) is unchanged
// from the pre-mux client, so existing repositories/consumers need no changes: the envelope is
// stripped here and only the inner ApiResponse bytes are surfaced. UI layers never see CBOR.
class NodeApiClient : public QObject {
    Q_OBJECT

public:
    // Credentials the handshake uses when the server advertises auth_mechanisms. A non-empty
    // `resumeToken` takes the AuthResume fast-path; otherwise username/password drive SCRAM (or
    // PLAIN over TLS). `tlsActive`/`hasClientCert` are set by the connection service from the
    // transport so the client can gate PLAIN/EXTERNAL without introspecting the socket.
    struct AuthCredentials {
        QString username;
        QString password;
        QString resumeToken;
        bool tlsActive = false;
        bool hasClientCert = false;
    };

    explicit NodeApiClient(DaemonTransport* transport, QObject* parent = nullptr);

    [[nodiscard]] DaemonTransport* transport() const { return m_transport; }

    // Set the credentials the next handshake authenticates with. Safe to call before connect; the
    // password is held only in memory and never logged.
    void setAuthCredentials(const AuthCredentials& creds) { m_creds = creds; }
    void clearAuthCredentials() { m_creds = AuthCredentials{}; }

    [[nodiscard]] bool isAuthenticated() const { return m_handshake == Handshake::Ready; }
    [[nodiscard]] const DecodedPrincipalView& principal() const { return m_principal; }

    // Queue one one-shot ApiRequest. The response is delivered as raw ApiResponse CBOR via
    // responseReady, tagged with the same correlationId.
    void sendRequest(const QByteArray& requestCbor, const QString& correlationId = QString());

    // Open a server-stream for a streaming-capable request (e.g. Subscribe). Items arrive via
    // streamItem(id, ...) and the stream closes with streamEnded(id, ...). Returns the stream id
    // (0 if there is no transport / the request is empty).
    quint64 openStream(const QByteArray& requestCbor);
    // Cancel a stream previously opened with openStream.
    void cancelStream(quint64 id);

    // Per-one-shot timeout override (defaults to kDefaultTimeoutMs). Streams are exempt (they use
    // their own End/keepalive liveness). Exposed so tests can drive the hung-daemon path fast.
    void setRequestTimeoutMs(int ms) { m_timeoutMs = ms; }

    // Whether the negotiated server advertised the streaming capability (valid once handshaked).
    [[nodiscard]] bool streamingAvailable() const {
        return m_features.contains(QStringLiteral("stream"));
    }

    // Whether the server advertised `feature` in its Hello (valid once handshaked). Used by seams
    // to gate optional surfaces (e.g. "versioning") before issuing a request that would 404.
    [[nodiscard]] bool hasFeature(const QString& feature) const {
        return m_features.contains(feature);
    }

    // The daemon-api CONTRACT version the server advertised via its "api/<N>" Hello feature
    // (valid once handshaked). 0 when absent - a daemon that predates the advertisement; the
    // connection service's version gate treats that as incompatible.
    [[nodiscard]] quint32 daemonApiVersion() const;

signals:
    void responseReady(const QString& correlationId, const QByteArray& responseCbor);
    void failed(const QString& correlationId, const QString& message);
    // The Hello handshake completed; server capabilities (hasFeature) are now valid.
    void handshakeReady();
    // SASL authentication succeeded (AuthOk): the connection is bound to `principal`.
    void authenticated(const daemonapp::daemon::DecodedPrincipalView& principal);
    // The transport dropped (connection lost): every outstanding exchange has been failed and the
    // client reset to Disconnected. The mirror uses this to run its reconnect teardown (§5.6).
    void disconnected();
    // The server issued an opaque session token on AuthOk (store it, never the password).
    void tokenIssued(const QString& token);
    // Authentication failed (AuthError, server-signature mismatch, or no usable mechanism). The
    // reason is coarse by design; the connection stays unauthenticated/disconnected.
    void authFailed(const QString& reason);
    // Stream lifecycle (openStream): one chunk, a close (clean or error), and a re-baseline signal.
    void streamItem(quint64 id, const QByteArray& responseCbor);
    void streamEnded(quint64 id, bool error, const QString& message);
    void streamReset(quint64 id, quint64 epoch, quint64 headSeq);

private:
    enum class Handshake { Disconnected, Handshaking, Authenticating, Ready };

    struct Pending {
        QString correlationId;
        bool stream = false;
        qint64 deadlineMs = 0; // one-shot timeout deadline (epoch ms); 0 for streams
    };

    void onFrame(const QByteArray& frameCbor);
    // Assign an id, register the pending exchange, wrap the request, and send-or-queue it. Returns
    // the id.
    quint64 enqueue(const QByteArray& requestCbor, const QString& correlationId, bool stream);
    // Begin the Hello handshake if not already connecting/ready (sends Hello ahead of any queued
    // frame).
    void ensureHandshake();
    // After the server Hello with non-empty auth_mechanisms: pick the AuthResume / SASL path and
    // send the first auth frame (or fail closed if no usable mechanism). Holds the outbox.
    void beginAuthentication(const QStringList& serverMechanisms);
    // Flush frames buffered during the handshake once the connection is Ready (post-AuthOk, or
    // immediately on an unauthenticated/local-trust node).
    void flushOutbox();
    // Fail every outstanding exchange (one-shots via failed, streams via streamEnded) and reset to
    // Disconnected so the next send re-handshakes. Idempotent.
    void failAll(const QString& message);
    // (Re)start the periodic one-shot deadline scan while one-shots are outstanding.
    void armTimeoutScan();

    static constexpr int kDefaultTimeoutMs = 30000;

    DaemonTransport* m_transport = nullptr;
    Handshake m_handshake = Handshake::Disconnected;
    quint64 m_nextId = 1;
    QHash<quint64, Pending> m_pending; // id -> outstanding exchange
    QList<QByteArray> m_outbox;        // wrapped frames awaiting the Hello/AuthOk ack
    QStringList m_features;            // server capabilities from the Hello ack
    int m_timeoutMs = kDefaultTimeoutMs;
    QTimer m_timeoutTimer; // periodic scan that fails one-shots past their deadline

    // v2 auth state.
    AuthCredentials m_creds;
    std::unique_ptr<SaslMechanism> m_mechanism; // active SASL exchange (null on resume / unauth)
    DecodedPrincipalView m_principal;           // bound principal once authenticated
};

} // namespace daemonapp::daemon
