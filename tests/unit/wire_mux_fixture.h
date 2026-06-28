#pragma once

// A controllable multiplexed daemon stand-in for daemon-app socket tests. It speaks the wire L0
// envelope (daemon-sync-protocol-spec.md §2) over a QLocalServer: it answers `Hello` with `Hello`,
// each one-shot `Call { id }` with `Reply { id, <configured payload> }`, records `Open { id }`
// streams for the test to push `Item`/`End` on, and acks `Cancel` with `End`. A "hang" mode accepts
// frames and never replies (the silent-daemon case). Plain (non-QObject) so it is header-only.

#include "daemon/daemon_transport.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>

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

struct ClientFrame {
    enum Kind { Unknown, Hello, Call, Open, Cancel } kind = Unknown;
    quint64 id = 0;
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
    return out;
}

inline QByteArray buildHello() {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "Hello");
    b.append(static_cast<char>(0xA2));
    cborText(b, "wire_version");
    cborUint(b, 1);
    cborText(b, "features");
    b.append(static_cast<char>(0x82));
    cborText(b, "mux");
    cborText(b, "stream");
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

inline QByteArray neFleetChanged(quint64 rev) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "FleetChanged");
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

inline QByteArray neDownloadProgress(quint64 id, quint64 pct, const char* state) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    cborText(b, "DownloadProgress");
    b.append(static_cast<char>(0xA3));
    cborText(b, "id");
    cborUint(b, id);
    cborText(b, "pct");
    cborUint(b, pct);
    cborText(b, "state");
    cborText(b, state);
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

class WireMuxServer {
public:
    ~WireMuxServer() { stop(); }

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
        m_streamConn = nullptr;
        if (m_server != nullptr) {
            m_server->close();
            m_server->deleteLater();
            m_server = nullptr;
        }
    }

    void setReplyPayload(const QByteArray& payload) { m_payload = payload; }
    void setHang(bool hang) { m_hang = hang; }
    [[nodiscard]] int requestCount() const { return m_requestCount; } // Call + Open frames
    [[nodiscard]] int helloCount() const { return m_helloCount; }
    [[nodiscard]] int openCount() const { return m_openCount; } // Open frames seen
    [[nodiscard]] quint64 lastOpenId() const { return m_lastOpenId; }

    // Push an Item / close the most recently opened stream (tst_wire_mux drives these).
    void pushItem(const QByteArray& res) {
        if (m_streamConn != nullptr) {
            writeFrame(m_streamConn, buildIdRes("Item", m_lastOpenId, res));
        }
    }
    void endStream(bool error) {
        if (m_streamConn != nullptr) {
            writeFrame(m_streamConn, buildEnd(m_lastOpenId, error));
        }
    }
    // Push a Reset for the most recently opened stream (L2: signals the client to re-baseline).
    void pushReset(quint64 epoch, quint64 headSeq) {
        if (m_streamConn != nullptr) {
            writeFrame(m_streamConn, buildReset(m_lastOpenId, epoch, headSeq));
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
            const ClientFrame cf = parseClientFrame(frame);
            switch (cf.kind) {
            case ClientFrame::Hello:
                ++m_helloCount;
                if (!m_hang) {
                    writeFrame(conn, buildHello());
                }
                break;
            case ClientFrame::Call:
                ++m_requestCount;
                if (!m_hang) {
                    writeFrame(conn, buildIdRes("Reply", cf.id, m_payload));
                }
                break;
            case ClientFrame::Open:
                ++m_requestCount;
                ++m_openCount;
                m_streamConn = conn;
                m_lastOpenId = cf.id;
                break; // the test drives pushItem/endStream
            case ClientFrame::Cancel:
                writeFrame(conn, buildEnd(cf.id, false));
                break;
            case ClientFrame::Unknown:
                break;
            }
        }
    }

    static void writeFrame(QLocalSocket* conn, const QByteArray& payload) {
        conn->write(daemonapp::daemon::DaemonTransport::framePayload(payload));
        conn->flush();
    }

    QLocalServer* m_server = nullptr;
    QList<QLocalSocket*> m_conns;
    QHash<QLocalSocket*, QByteArray> m_buffers;
    QLocalSocket* m_streamConn = nullptr;
    quint64 m_lastOpenId = 0;
    QByteArray m_payload;
    bool m_hang = false;
    int m_requestCount = 0;
    int m_helloCount = 0;
    int m_openCount = 0;
};

} // namespace daemonapp::test
