// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// A controllable multiplexed daemon stand-in for daemon-app socket tests. It speaks the wire L0
// envelope (daemon-sync-protocol-spec.md §2): it answers `Hello` with `Hello`, each one-shot
// `Call { id }` with `Reply { id, <configured payload> }`, records `Open { id }` streams for the
// test to push `Item`/`End` on, and acks `Cancel` with `End`. A "hang" mode accepts frames and
// never replies (the silent-daemon case). Plain (non-QObject) so it is header-only.
//
// The frame semantics live in the carrier-independent MuxServerCore; WireMuxServer wraps it in
// the length-framed QLocalServer carrier, and ws_mux_fixture.h adds the message-framed
// QWebSocketServer carrier (WsMuxServer) over the same core.

#include "daemon/daemon_transport.h"
#include "daemon/node_api_auth.h"
#include "daemon/node_api_codec.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>
#include <QStringList>

namespace daemonapp::test {

// ----- minimal canonical-CBOR helpers (mirror of the codec's, scoped to the envelope) ----------

inline void cborUint(QByteArray& b, quint64 v) {
    if (v < 24) {
        b.append(static_cast<char>(v));
    } else if (v <= 0xFF) {
        b.append(static_cast<char>(0x18));
        b.append(static_cast<char>(v));
    } else if (v <= 0xFFFF) {
        b.append(static_cast<char>(0x19));
        b.append(static_cast<char>((v >> 8) & 0xFF));
        b.append(static_cast<char>(v & 0xFF));
    } else if (v <= 0xFFFFFFFF) {
        b.append(static_cast<char>(0x1A));
        for (int s = 24; s >= 0; s -= 8) {
            b.append(static_cast<char>((v >> s) & 0xFF));
        }
    } else {
        b.append(static_cast<char>(0x1B));
        for (int s = 56; s >= 0; s -= 8) {
            b.append(static_cast<char>((v >> s) & 0xFF));
        }
    }
}

inline void cborText(QByteArray& b, const char* s) {
    const auto len = static_cast<quint64>(qstrlen(s));
    b.append(static_cast<char>(0x60 | static_cast<char>(len)));
    b.append(s, static_cast<qsizetype>(len));
}

inline bool cborHead(const uchar* p, qsizetype n, qsizetype& i, quint8& major, quint64& arg) {
    if (i >= n) {
        return false;
    }
    const quint8 ib = p[i++];
    major = ib >> 5;
    const quint8 lo = ib & 0x1F;
    if (lo < 24) {
        arg = lo;
        return true;
    }
    int nbytes = 0;
    switch (lo) {
    case 24:
        nbytes = 1;
        break;
    case 25:
        nbytes = 2;
        break;
    case 26:
        nbytes = 4;
        break;
    case 27:
        nbytes = 8;
        break;
    default:
        return false;
    }
    if (i + nbytes > n) {
        return false;
    }
    quint64 v = 0;
    for (int k = 0; k < nbytes; ++k) {
        v = (v << 8) | p[i++];
    }
    arg = v;
    return true;
}

inline bool cborText(const uchar* p, qsizetype n, qsizetype& i, QByteArray* out) {
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborHead(p, n, i, major, arg) || major != 3) {
        return false;
    }
    if (i + static_cast<qsizetype>(arg) > n) {
        return false;
    }
    *out = QByteArray(reinterpret_cast<const char*>(p + i), static_cast<qsizetype>(arg));
    i += static_cast<qsizetype>(arg);
    return true;
}

// Arbitrary-length text head + bytes (the short cborText only emits <24-byte heads).
inline void cborTextLen(QByteArray& b, const QByteArray& s) {
    const auto len = static_cast<quint64>(s.size());
    if (len < 24) {
        b.append(static_cast<char>(0x60 | static_cast<char>(len)));
    } else if (len <= 0xFF) {
        b.append(static_cast<char>(0x78));
        b.append(static_cast<char>(len));
    } else {
        b.append(static_cast<char>(0x79));
        b.append(static_cast<char>((len >> 8) & 0xFF));
        b.append(static_cast<char>(len & 0xFF));
    }
    b.append(s);
}

// Append `bytes` as a CBOR array of uints (`[* uint]`), the contract's SASL byte encoding.
inline void cborBytesAsUintArray(QByteArray& b, const QByteArray& bytes) {
    const auto len = static_cast<quint64>(bytes.size());
    if (len < 24) {
        b.append(static_cast<char>(0x80 | static_cast<char>(len)));
    } else if (len <= 0xFF) {
        b.append(static_cast<char>(0x98));
        b.append(static_cast<char>(len));
    } else {
        b.append(static_cast<char>(0x99));
        b.append(static_cast<char>((len >> 8) & 0xFF));
        b.append(static_cast<char>(len & 0xFF));
    }
    for (const char c : bytes) {
        cborUint(b, static_cast<quint8>(c));
    }
}

// Read a CBOR array-of-uints (`[* uint]`) back into raw bytes.
inline bool cborUintArray(const uchar* p, qsizetype n, qsizetype& i, QByteArray* out) {
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborHead(p, n, i, major, arg) || major != 4) {
        return false;
    }
    QByteArray bytes;
    for (quint64 k = 0; k < arg; ++k) {
        quint8 emaj = 0;
        quint64 earg = 0;
        if (!cborHead(p, n, i, emaj, earg) || emaj != 0 || earg > 0xFF) {
            return false;
        }
        bytes.append(static_cast<char>(static_cast<quint8>(earg)));
    }
    *out = bytes;
    return true;
}

struct ClientFrame {
    enum Kind {
        Unknown,
        Hello,
        Call,
        Open,
        Cancel,
        AuthStart,
        AuthStep,
        AuthResume
    } kind = Unknown;
    quint64 id = 0;
    QByteArray mechanism; // AuthStart
    QByteArray data;      // AuthStart.initial / AuthStep.data (raw bytes)
    QByteArray token;     // AuthResume
    QByteArray payload;   // Call/Open: the wrapped ApiRequest bytes (the "req" value)
};

inline ClientFrame parseClientFrame(const QByteArray& f) {
    ClientFrame out;
    const auto* p = reinterpret_cast<const uchar*>(f.constData());
    const qsizetype n = f.size();
    qsizetype i = 0;
    quint8 major = 0;
    quint64 arg = 0;
    if (!cborHead(p, n, i, major, arg) || major != 5 || arg != 1) {
        return out;
    }
    QByteArray tag;
    if (!cborText(p, n, i, &tag)) {
        return out;
    }
    if (!cborHead(p, n, i, major, arg) || major != 5) {
        return out;
    }
    if (tag == "Hello") {
        out.kind = ClientFrame::Hello;
        return out;
    }
    if (tag == "AuthStart") {
        QByteArray key;
        if (cborText(p, n, i, &key) && key == "mechanism" && cborText(p, n, i, &out.mechanism) &&
            cborText(p, n, i, &key) && key == "initial" && cborUintArray(p, n, i, &out.data)) {
            out.kind = ClientFrame::AuthStart;
        }
        return out;
    }
    if (tag == "AuthStep") {
        QByteArray key;
        if (cborText(p, n, i, &key) && key == "data" && cborUintArray(p, n, i, &out.data)) {
            out.kind = ClientFrame::AuthStep;
        }
        return out;
    }
    if (tag == "AuthResume") {
        QByteArray key;
        if (cborText(p, n, i, &key) && key == "token" && cborText(p, n, i, &out.token)) {
            out.kind = ClientFrame::AuthResume;
        }
        return out;
    }
    QByteArray key;
    if (!cborText(p, n, i, &key) || key != "id" || !cborHead(p, n, i, major, arg) || major != 0) {
        return out;
    }
    out.id = arg;
    if (tag == "Call") {
        out.kind = ClientFrame::Call;
    } else if (tag == "Open") {
        out.kind = ClientFrame::Open;
    } else if (tag == "Cancel") {
        out.kind = ClientFrame::Cancel;
    }
    // Capture the wrapped ApiRequest bytes: the "req" value is the frame's final element, so the
    // remainder of the buffer is exactly the encoded request (lets tests assert what was sent,
    // e.g. that a page-loop continuation carried the `after` cursor).
    if (out.kind == ClientFrame::Call || out.kind == ClientFrame::Open) {
        QByteArray reqKey;
        qsizetype j = i;
        if (cborText(p, n, j, &reqKey) && reqKey == "req") {
            out.payload = QByteArray(reinterpret_cast<const char*>(p + j), n - j);
        }
    }
    return out;
}

// The feature set a CURRENT daemon advertises: the envelope features plus its api contract
// version ("api/<N>"), which the client's connect gate checks. Fixture tests that need a stale
// daemon override the list via WireMuxServer::setHelloFeatures.
inline QStringList currentHelloFeatures() {
    return {QStringLiteral("mux"), QStringLiteral("stream"),
            QStringLiteral("api/") +
                QString::number(daemonapp::daemon::NodeApiCodec::kDaemonApiVersion)};
}

// Server Hello (v2). The S2C Hello always carries auth_mechanisms (empty => unauthenticated node).
inline QByteArray buildHello(const QStringList& authMechanisms = {},
                             const QStringList& features = currentHelloFeatures()) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "Hello");
    b.append(static_cast<char>(0xA3)); // map(3): wire_version, features, auth_mechanisms
    cborText(b, "wire_version");
    cborUint(b, 2);
    cborText(b, "features");
    b.append(static_cast<char>(0x80 | static_cast<char>(features.size())));
    for (const QString& f : features) {
        cborTextLen(b, f.toUtf8());
    }
    cborText(b, "auth_mechanisms");
    b.append(static_cast<char>(0x80 | static_cast<char>(authMechanisms.size())));
    for (const QString& m : authMechanisms) {
        cborTextLen(b, m.toUtf8());
    }
    return b;
}

inline QByteArray buildAuthChallenge(const QByteArray& data) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "AuthChallenge");
    b.append(static_cast<char>(0xA1));
    cborText(b, "data");
    cborBytesAsUintArray(b, data);
    return b;
}

inline QByteArray buildAuthError(const QByteArray& reason) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "AuthError");
    b.append(static_cast<char>(0xA1));
    cborText(b, "reason");
    cborTextLen(b, reason);
    return b;
}

inline QByteArray buildAuthOk(const QByteArray& token, const QByteArray& userId,
                              const QByteArray& username, const QStringList& roles,
                              const QStringList& capabilities) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "AuthOk");
    b.append(static_cast<char>(0xA2)); // map(2): token, principal
    cborText(b, "token");
    cborTextLen(b, token);
    cborText(b, "principal");
    b.append(static_cast<char>(0xA4)); // map(4): user_id, username, roles, capabilities
    cborText(b, "user_id");
    cborTextLen(b, userId);
    cborText(b, "username");
    cborTextLen(b, username);
    cborText(b, "roles");
    b.append(static_cast<char>(0x80 | static_cast<char>(roles.size())));
    for (const QString& r : roles) {
        cborTextLen(b, r.toUtf8());
    }
    cborText(b, "capabilities");
    b.append(static_cast<char>(0x80 | static_cast<char>(capabilities.size())));
    for (const QString& c : capabilities) {
        cborTextLen(b, c.toUtf8());
    }
    return b;
}

inline QByteArray buildIdRes(const char* tag, quint64 id, const QByteArray& res) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, tag);
    b.append(static_cast<char>(0xA2));
    cborText(b, "id");
    cborUint(b, id);
    cborText(b, "res");
    b.append(res);
    return b;
}

inline QByteArray buildReset(quint64 id, quint64 epoch, quint64 headSeq) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "Reset");
    b.append(static_cast<char>(0xA3)); // map(3): id, epoch, head_seq
    cborText(b, "id");
    cborUint(b, id);
    cborText(b, "epoch");
    cborUint(b, epoch);
    cborText(b, "head_seq");
    cborUint(b, headSeq);
    return b;
}

inline QByteArray buildEnd(quint64 id, bool error) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "End");
    b.append(static_cast<char>(0xA2));
    cborText(b, "id");
    cborUint(b, id);
    cborText(b, "error");
    if (error) {
        b.append(static_cast<char>(0xA1)); // {"Other":"e"}
        cborText(b, "Other");
        cborText(b, "e");
    } else {
        b.append(static_cast<char>(0xF6)); // null
    }
    return b;
}

// ----- L3 node-event / events-page response builders (tst_subscription_manager) ----------------
// Each node-event is an externally-tagged map(1); fields are emitted in CDDL order (the generated
// decoder reads them sequentially). events-page is {events, next_cursor, head_cursor}.

inline QByteArray neRosterChanged(quint64 rev) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "RosterChanged");
    b.append(static_cast<char>(0xA1));
    cborText(b, "rev");
    cborUint(b, rev);
    return b;
}

inline QByteArray neSessionMetaChanged(const char* session, quint64 rev) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "SessionMetaChanged");
    b.append(static_cast<char>(0xA2));
    cborText(b, "session");
    cborText(b, session);
    cborText(b, "rev");
    cborUint(b, rev);
    return b;
}

inline QByteArray neApprovalPending(const char* session, const char* requestId) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "ApprovalPending");
    b.append(static_cast<char>(0xA2));
    cborText(b, "session");
    cborText(b, session);
    cborText(b, "request_id");
    cborText(b, requestId);
    return b;
}

inline QByteArray neSessionAdvanced(const char* session, quint64 epoch, quint64 headSeq) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "SessionAdvanced");
    b.append(static_cast<char>(0xA3));
    cborText(b, "session");
    cborText(b, session);
    cborText(b, "epoch");
    cborUint(b, epoch);
    cborText(b, "head_seq");
    cborUint(b, headSeq);
    return b;
}

inline QByteArray neDownloadProgress(quint64 id, quint64 pct, const char* state,
                                     quint64 downloadedBytes, quint64 totalBytes) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "DownloadProgress");
    b.append(static_cast<char>(0xA5));
    cborText(b, "id");
    cborUint(b, id);
    cborText(b, "pct");
    cborUint(b, pct);
    cborText(b, "state");
    cborText(b, state);
    cborText(b, "downloaded_bytes");
    cborUint(b, downloadedBytes);
    cborText(b, "total_bytes");
    cborUint(b, totalBytes);
    return b;
}

// v26: the payload-free catalog-changed pointer (a bare externally-tagged unit variant).
inline QByteArray neCatalogChanged() {
    QByteArray b;
    cborText(b, "CatalogChanged");
    return b;
}

inline QByteArray neResyncNeeded(const char* scope) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "ResyncNeeded");
    b.append(static_cast<char>(0xA1));
    cborText(b, "scope");
    cborText(b, scope);
    return b;
}

// [wave2:app-channels-liveness] F5: fleet supervision changed (rev only).
inline QByteArray neFleetChanged(quint64 rev) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "FleetChanged");
    b.append(static_cast<char>(0xA1));
    cborText(b, "rev");
    cborUint(b, rev);
    return b;
}

// [wave2:app-channels-liveness] B5: live transport presence. `presence` empty omits the optional
// presence field (map(2)); non-empty carries it (map(3)). connection/presence are the capitalized
// wire enum names ("Connected"/"Available"/...).
inline QByteArray neTransportChanged(const char* transport, const char* connection,
                                     const char* presence = "") {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "TransportChanged");
    const bool hasPresence = presence != nullptr && presence[0] != '\0';
    b.append(static_cast<char>(0xA0 | (hasPresence ? 3 : 2)));
    cborText(b, "transport");
    cborText(b, transport);
    cborText(b, "connection");
    cborText(b, connection);
    if (hasPresence) {
        cborText(b, "presence");
        cborText(b, presence);
    }
    return b;
}

// [waveB:app-v30] D2: {"ConversationsChanged":{"transport","conv","change"}} — change is the
// conv-change enum tstr ("Added"/"Removed").
inline QByteArray neConversationsChanged(const char* transport, const char* conv,
                                         const char* change) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "ConversationsChanged");
    b.append(static_cast<char>(0xA3));
    cborText(b, "transport");
    cborText(b, transport);
    cborText(b, "conv");
    cborText(b, conv);
    cborText(b, "change");
    cborText(b, change);
    return b;
}

// [waveB:app-v30] D2: {"MembershipChanged":{"transport","conv","member","change","is_self"}} — the
// minimal shape (optional actor/reason omitted). `change` is the membership-change enum tstr.
inline QByteArray neMembershipChanged(const char* transport, const char* conv, const char* member,
                                      const char* change, bool isSelf) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "MembershipChanged");
    b.append(static_cast<char>(0xA5)); // map(5): transport, conv, member, change, is_self
    cborText(b, "transport");
    cborText(b, transport);
    cborText(b, "conv");
    cborText(b, conv);
    cborText(b, "member");
    cborText(b, member);
    cborText(b, "change");
    cborText(b, change);
    cborText(b, "is_self");
    b.append(static_cast<char>(isSelf ? 0xF5 : 0xF4)); // CBOR true / false
    return b;
}

// {"EventsPage":{"events":[...],"next_cursor":N,"head_cursor":M}} (events count assumed < 24).
inline QByteArray buildEventsPage(const QList<QByteArray>& events, quint64 nextCursor,
                                  quint64 headCursor) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "EventsPage");
    b.append(static_cast<char>(0xA3));
    cborText(b, "events");
    b.append(static_cast<char>(0x80 | static_cast<char>(events.size())));
    for (const QByteArray& e : events) {
        b.append(e);
    }
    cborText(b, "next_cursor");
    cborUint(b, nextCursor);
    cborText(b, "head_cursor");
    cborUint(b, headCursor);
    return b;
}

// Carrier-independent core of the fake mux daemon: the Hello/auth/Call/Open/Cancel frame
// semantics, the server-side SCRAM/PLAIN/EXTERNAL verification, the config knobs, and the
// counters. The carrier wrappers - WireMuxServer below (length-framed QLocalServer) and
// WsMuxServer in ws_mux_fixture.h (message-framed QWebSocketServer) - only move payload bytes,
// so both carriers answer identically by construction: exactly the client-visible parity the WS
// transport must preserve.
class MuxServerCore {
public:
    virtual ~MuxServerCore() = default;

    void setReplyPayload(const QByteArray& payload) { m_payload = payload; }
    // Serve these payloads one per Call in order (page-loop tests), then fall back to the fixed
    // reply payload once exhausted.
    void setReplySequence(const QList<QByteArray>& payloads) { m_replySeq = payloads; }
    void setHang(bool hang) { m_hang = hang; }
    [[nodiscard]] int requestCount() const { return m_requestCount; } // Call + Open frames
    [[nodiscard]] int helloCount() const { return m_helloCount; }
    [[nodiscard]] int openCount() const { return m_openCount; } // Open frames seen
    [[nodiscard]] quint64 lastOpenId() const { return m_lastOpenId; }
    // The wrapped ApiRequest bytes of every Call/Open received, in arrival order.
    [[nodiscard]] QList<QByteArray> callPayloads() const { return m_callPayloads; }

    // Advertise these capability strings in the Hello instead of a current daemon's set - the
    // stale-daemon stand-in for the client's api-version connect gate (e.g. {"mux","stream"} for
    // a pre-advertisement daemon, or {"mux","stream","api/22"} for an older contract).
    void setHelloFeatures(const QStringList& features) { m_helloFeatures = features; }

    // --- v2 SASL auth config -------------------------------------------------------------------
    // Advertise these SASL mechanisms in the Hello (empty => unauthenticated node, the default).
    void setAuthMechanisms(const QStringList& mechs) { m_authMechanisms = mechs; }
    // The credentials the SCRAM/PLAIN server validates against. A client password that differs
    // produces a failing proof (the "wrong password" path) naturally.
    void setCredentials(const QByteArray& user, const QByteArray& password) {
        m_user = user;
        m_password = password;
    }
    void setExpectedResumeToken(const QByteArray& token) { m_resumeToken = token; }
    void setIssuedToken(const QByteArray& token) { m_issuedToken = token; }
    void setPrincipal(const QByteArray& userId, const QByteArray& username,
                      const QStringList& roles, const QStringList& capabilities) {
        m_principalUserId = userId;
        m_principalUsername = username;
        m_principalRoles = roles;
        m_principalCaps = capabilities;
    }
    [[nodiscard]] int authStartCount() const { return m_authStartCount; }
    [[nodiscard]] int authOkCount() const { return m_authOkCount; }
    [[nodiscard]] int callsBeforeAuthOk() const { return m_callsBeforeAuthOk; }

    // Push an Item / close the most recently opened stream (tst_wire_mux drives these).
    void pushItem(const QByteArray& res) {
        if (streamPeerActive()) {
            writeStreamFrame(buildIdRes("Item", m_lastOpenId, res));
        }
    }
    void endStream(bool error) {
        if (streamPeerActive()) {
            writeStreamFrame(buildEnd(m_lastOpenId, error));
        }
    }
    // Push a Reset for the most recently opened stream (L2: signals the client to re-baseline).
    void pushReset(quint64 epoch, quint64 headSeq) {
        if (streamPeerActive()) {
            writeStreamFrame(buildReset(m_lastOpenId, epoch, headSeq));
        }
    }

protected:
    // Per-connection SCRAM server state (carried between AuthStart and AuthStep). The carrier
    // keys one per live connection.
    struct AuthCtx {
        bool authed = false;
        QByteArray clientFirstBare;
        QByteArray serverFirst;
        QByteArray salt;
        int iter = 4096;
    };

    // Process one client->server payload frame: update the counters/auth state, append the reply
    // payload frames to `replies` (the carrier writes them back with its own framing), and return
    // true iff the frame was an Open - the carrier then records that connection as the stream
    // peer for pushItem/endStream/pushReset.
    bool processFrame(AuthCtx& ctx, const QByteArray& frame, QList<QByteArray>* replies) {
        const ClientFrame cf = parseClientFrame(frame);
        const bool authed = m_authMechanisms.isEmpty() || ctx.authed;
        switch (cf.kind) {
        case ClientFrame::Hello:
            ++m_helloCount;
            if (!m_hang) {
                replies->append(buildHello(m_authMechanisms, m_helloFeatures));
            }
            break;
        case ClientFrame::AuthStart:
            if (!m_hang) {
                handleAuthStart(ctx, cf, replies);
            }
            break;
        case ClientFrame::AuthStep:
            if (!m_hang) {
                handleAuthStep(ctx, cf, replies);
            }
            break;
        case ClientFrame::AuthResume:
            if (!m_hang) {
                if (!m_resumeToken.isEmpty() && cf.token == m_resumeToken) {
                    appendAuthOk(ctx, replies);
                } else {
                    replies->append(buildAuthError("invalid token"));
                }
            }
            break;
        case ClientFrame::Call:
            ++m_requestCount;
            m_callPayloads.append(cf.payload);
            if (!authed) {
                ++m_callsBeforeAuthOk; // a leak: a request escaped pre-AuthOk
            }
            if (!m_hang && authed) {
                replies->append(buildIdRes("Reply", cf.id, nextReplyPayload()));
            }
            break;
        case ClientFrame::Open:
            ++m_requestCount;
            ++m_openCount;
            m_callPayloads.append(cf.payload);
            if (!authed) {
                ++m_callsBeforeAuthOk;
            }
            m_lastOpenId = cf.id;
            return true; // the test drives pushItem/endStream on this connection
        case ClientFrame::Cancel:
            replies->append(buildEnd(cf.id, false));
            break;
        case ClientFrame::Unknown:
            break;
        }
        return false;
    }

    // Carrier hooks for the test-driven stream pushes (Item/End/Reset to the recorded peer).
    virtual void writeStreamFrame(const QByteArray& payload) = 0;
    [[nodiscard]] virtual bool streamPeerActive() const = 0;

private:
    static QByteArray scramAttr(const QByteArray& message, char key) {
        for (const QByteArray& part : message.split(',')) {
            if (part.size() >= 2 && part[0] == key && part[1] == '=') {
                return part.mid(2);
            }
        }
        return {};
    }

    void appendAuthOk(AuthCtx& ctx, QList<QByteArray>* replies) {
        ctx.authed = true;
        ++m_authOkCount;
        replies->append(buildAuthOk(m_issuedToken, m_principalUserId, m_principalUsername,
                                    m_principalRoles, m_principalCaps));
    }

    void handleAuthStart(AuthCtx& ctx, const ClientFrame& cf, QList<QByteArray>* replies) {
        ++m_authStartCount;
        if (cf.mechanism == "SCRAM-SHA-256") {
            const QByteArray initial = cf.data; // "n,,n=user,r=cnonce"
            const QByteArray bare = initial.startsWith("n,,") ? initial.mid(3) : initial;
            const QByteArray cnonce = scramAttr(bare, 'r');
            ctx.clientFirstBare = bare;
            ctx.salt = QByteArray::fromBase64("W22ZaJ0SNY7soEsUEjb6gQ==");
            ctx.iter = 4096;
            const QByteArray combined = cnonce + "serverNonceXYZ";
            ctx.serverFirst = "r=" + combined + ",s=" + ctx.salt.toBase64() +
                              ",i=" + QByteArray::number(ctx.iter);
            replies->append(buildAuthChallenge(ctx.serverFirst));
        } else if (cf.mechanism == "PLAIN") {
            const QList<QByteArray> parts = cf.data.split('\0'); // "\0user\0pass"
            const bool ok = parts.size() == 3 && parts[1] == m_user && parts[2] == m_password;
            if (ok) {
                appendAuthOk(ctx, replies);
            } else {
                replies->append(buildAuthError("authentication failed"));
            }
        } else if (cf.mechanism == "EXTERNAL") {
            appendAuthOk(ctx, replies);
        } else {
            replies->append(buildAuthError("unsupported mechanism"));
        }
    }

    void handleAuthStep(AuthCtx& ctx, const ClientFrame& cf, QList<QByteArray>* replies) {
        namespace scram = daemonapp::daemon::scram;
        const QByteArray clientFinal = cf.data; // "c=biws,r=combined,p=proof"
        const int pIdx = clientFinal.indexOf(",p=");
        if (pIdx < 0) {
            replies->append(buildAuthError("authentication failed"));
            return;
        }
        const QByteArray clientFinalNoProof = clientFinal.left(pIdx);
        const QByteArray providedProof = clientFinal.mid(pIdx + 3);
        const QByteArray salted = scram::pbkdf2HmacSha256(m_password, ctx.salt, ctx.iter, 32);
        const QByteArray clientKey = scram::hmacSha256(salted, "Client Key");
        const QByteArray storedKey = scram::sha256(clientKey);
        const QByteArray authMessage =
            ctx.clientFirstBare + "," + ctx.serverFirst + "," + clientFinalNoProof;
        const QByteArray clientSig = scram::hmacSha256(storedKey, authMessage);
        const QByteArray expectedProof = scram::xorBytes(clientKey, clientSig).toBase64();
        if (expectedProof != providedProof) {
            replies->append(buildAuthError("authentication failed")); // wrong password
            return;
        }
        const QByteArray serverKey = scram::hmacSha256(salted, "Server Key");
        const QByteArray serverSig = scram::hmacSha256(serverKey, authMessage);
        // server-final as the LAST AuthChallenge, then AuthOk back-to-back (the frozen framing).
        replies->append(buildAuthChallenge("v=" + serverSig.toBase64()));
        appendAuthOk(ctx, replies);
    }

    QByteArray nextReplyPayload() {
        return m_replySeq.isEmpty() ? m_payload : m_replySeq.takeFirst();
    }

    quint64 m_lastOpenId = 0;
    QByteArray m_payload;
    QList<QByteArray> m_replySeq;
    QList<QByteArray> m_callPayloads;
    QStringList m_helloFeatures = currentHelloFeatures();
    bool m_hang = false;
    int m_requestCount = 0;
    int m_helloCount = 0;
    int m_openCount = 0;

    // v2 auth config / counters.
    QStringList m_authMechanisms;
    QByteArray m_user = "user";
    QByteArray m_password = "pencil";
    QByteArray m_resumeToken;
    QByteArray m_issuedToken = "session-token-xyz";
    QByteArray m_principalUserId = "uid-1";
    QByteArray m_principalUsername = "user";
    QStringList m_principalRoles{QStringLiteral("user")};
    QStringList m_principalCaps{QStringLiteral("session_read"), QStringLiteral("session_write")};
    int m_authStartCount = 0;
    int m_authOkCount = 0;
    int m_callsBeforeAuthOk = 0;
};

// The QLocalServer carrier: the daemon-host Unix-socket wire shape, a big-endian u32 length
// prefix framing each payload (DaemonTransport::framePayload/tryTakeFrame).
class WireMuxServer : public MuxServerCore {
public:
    ~WireMuxServer() override { stop(); }

    bool start(const QString& path) {
        QLocalServer::removeServer(path);
        m_server = new QLocalServer();
        QObject::connect(m_server, &QLocalServer::newConnection, m_server,
                         [this] { onNewConnection(); });
        return m_server->listen(path);
    }

    void stop() {
        for (QLocalSocket* conn : std::as_const(m_conns)) {
            conn->abort();
            conn->deleteLater();
        }
        m_conns.clear();
        m_buffers.clear();
        m_auth.clear();
        m_streamConn = nullptr;
        if (m_server != nullptr) {
            m_server->close();
            m_server->deleteLater();
            m_server = nullptr;
        }
    }

private:
    void onNewConnection() {
        while (m_server != nullptr && m_server->hasPendingConnections()) {
            QLocalSocket* conn = m_server->nextPendingConnection();
            m_conns.append(conn);
            QObject::connect(conn, &QLocalSocket::readyRead, conn,
                             [this, conn] { onReadyRead(conn); });
        }
    }

    void onReadyRead(QLocalSocket* conn) {
        m_buffers[conn].append(conn->readAll());
        QByteArray frame;
        while (daemonapp::daemon::DaemonTransport::tryTakeFrame(m_buffers[conn], &frame)) {
            QList<QByteArray> replies;
            if (processFrame(m_auth[conn], frame, &replies)) {
                m_streamConn = conn;
            }
            for (const QByteArray& payload : replies) {
                writeFrame(conn, payload);
            }
        }
    }

    void writeStreamFrame(const QByteArray& payload) override { writeFrame(m_streamConn, payload); }
    [[nodiscard]] bool streamPeerActive() const override { return m_streamConn != nullptr; }

    static void writeFrame(QLocalSocket* conn, const QByteArray& payload) {
        conn->write(daemonapp::daemon::DaemonTransport::framePayload(payload));
        conn->flush();
    }

    QLocalServer* m_server = nullptr;
    QList<QLocalSocket*> m_conns;
    QHash<QLocalSocket*, QByteArray> m_buffers;
    QHash<QLocalSocket*, AuthCtx> m_auth;
    QLocalSocket* m_streamConn = nullptr;
};

} // namespace daemonapp::test
