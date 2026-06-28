#include "daemon/node_api_client.h"

#include "daemon/node_api_codec.h"

#include <QDateTime>

namespace daemonapp::daemon {

NodeApiClient::NodeApiClient(DaemonTransport* transport, QObject* parent)
    : QObject(parent), m_transport(transport) {
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this] {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        QList<quint64> expired;
        bool anyOneShot = false;
        for (auto it = m_pending.constBegin(); it != m_pending.constEnd(); ++it) {
            if (it.value().stream) {
                continue;
            }
            anyOneShot = true;
            if (it.value().deadlineMs != 0 && now >= it.value().deadlineMs) {
                expired.append(it.key());
            }
        }
        for (const quint64 id : expired) {
            const Pending p = m_pending.take(id);
            anyOneShot = anyOneShot && !m_pending.isEmpty();
            emit failed(p.correlationId, QStringLiteral("request timed out"));
        }
        // Stop scanning once no one-shot remains.
        bool remaining = false;
        for (auto it = m_pending.constBegin(); it != m_pending.constEnd(); ++it) {
            if (!it.value().stream) {
                remaining = true;
                break;
            }
        }
        if (!remaining) {
            m_timeoutTimer.stop();
        }
    });

    if (m_transport != nullptr) {
        connect(m_transport, &DaemonTransport::frameReceived, this,
                [this](const QByteArray& cbor) { onFrame(cbor); });
        connect(m_transport, &DaemonTransport::failed, this,
                [this](const QString& message) { failAll(message); });
        // An unexpected close (daemon exit / peer disconnect) must not leave exchanges stuck; treat
        // it as a failure that resets the handshake so the next send reconnects.
        connect(m_transport, &DaemonTransport::disconnected, this,
                [this] { failAll(QStringLiteral("daemon connection closed")); });
    }
}

void NodeApiClient::sendRequest(const QByteArray& requestCbor, const QString& correlationId) {
    enqueue(requestCbor, correlationId, /*stream=*/false);
}

quint64 NodeApiClient::openStream(const QByteArray& requestCbor) {
    return enqueue(requestCbor, QString(), /*stream=*/true);
}

void NodeApiClient::cancelStream(quint64 id) {
    // Graceful: send Cancel but keep the pending entry so the server's End ack drives streamEnded
    // (a dropped connection still cleans it up via failAll).
    const auto it = m_pending.constFind(id);
    if (it != m_pending.constEnd() && it.value().stream && m_handshake == Handshake::Ready &&
        m_transport != nullptr) {
        m_transport->sendFrame(NodeApiCodec::encodeCancelFrame(id));
    }
}

quint64 NodeApiClient::enqueue(const QByteArray& requestCbor, const QString& correlationId,
                               bool stream) {
    if (m_transport == nullptr) {
        emit failed(correlationId, QStringLiteral("No daemon transport configured"));
        return 0;
    }
    if (requestCbor.isEmpty()) {
        emit failed(correlationId, QStringLiteral("Refusing to send an empty NodeApi request"));
        return 0;
    }
    const quint64 id = m_nextId++;
    Pending pending;
    pending.correlationId = correlationId;
    pending.stream = stream;
    if (!stream) {
        pending.deadlineMs = QDateTime::currentMSecsSinceEpoch() + m_timeoutMs;
    }
    m_pending.insert(id, pending);

    const QByteArray frame = stream ? NodeApiCodec::encodeOpenFrame(id, requestCbor)
                                    : NodeApiCodec::encodeCallFrame(id, requestCbor);
    if (m_handshake == Handshake::Ready) {
        m_transport->sendFrame(frame);
    } else {
        // Hold behind the handshake so the Hello frame is always sent first.
        m_outbox.append(frame);
        ensureHandshake();
    }
    if (!stream) {
        armTimeoutScan();
    }
    return id;
}

void NodeApiClient::ensureHandshake() {
    if (m_handshake != Handshake::Disconnected || m_transport == nullptr) {
        return;
    }
    m_handshake = Handshake::Handshaking;
    // The transport opens lazily and buffers this until connected, so Hello leads the byte stream.
    m_transport->sendFrame(NodeApiCodec::encodeHelloFrame());
}

void NodeApiClient::flushOutbox() {
    if (m_transport == nullptr) {
        return;
    }
    const QList<QByteArray> frames = m_outbox;
    m_outbox.clear();
    for (const QByteArray& frame : frames) {
        m_transport->sendFrame(frame);
    }
}

void NodeApiClient::onFrame(const QByteArray& frameCbor) {
    DecodedWireFrame frame;
    if (!NodeApiCodec::decodeWireFrame(frameCbor, &frame)) {
        // Undecodable / non-envelope frame (e.g. a legacy daemon's bare ApiResponse): drop it. The
        // handshake will time out the queued requests, which is the correct signal.
        return;
    }
    switch (frame.kind) {
    case WireFrameKind::Hello:
        m_handshake = Handshake::Ready;
        // Record the server's advertised capabilities (mux/stream always; versioning only on a
        // durable node). Fall back to the always-on envelope set if an older daemon sends none.
        m_features = frame.features.isEmpty()
                         ? QStringList{QStringLiteral("mux"), QStringLiteral("stream")}
                         : frame.features;
        flushOutbox();
        emit handshakeReady();
        break;
    case WireFrameKind::Reply: {
        const auto it = m_pending.find(frame.id);
        if (it != m_pending.end() && !it.value().stream) {
            const QString correlationId = it.value().correlationId;
            m_pending.erase(it);
            emit responseReady(correlationId, frame.payload);
        }
        break;
    }
    case WireFrameKind::Item: {
        const auto it = m_pending.constFind(frame.id);
        if (it != m_pending.constEnd() && it.value().stream) {
            emit streamItem(frame.id, frame.payload);
        }
        break;
    }
    case WireFrameKind::End: {
        const auto it = m_pending.find(frame.id);
        if (it != m_pending.end()) {
            const bool stream = it.value().stream;
            const QString correlationId = it.value().correlationId;
            m_pending.erase(it);
            if (stream) {
                emit streamEnded(frame.id, frame.hasError,
                                 frame.hasError ? QStringLiteral("stream ended in error")
                                                : QString());
            } else if (frame.hasError) {
                emit failed(correlationId, QStringLiteral("the request failed"));
            }
        }
        break;
    }
    case WireFrameKind::Reset: {
        const auto it = m_pending.constFind(frame.id);
        if (it != m_pending.constEnd() && it.value().stream) {
            emit streamReset(frame.id, frame.epoch, frame.headSeq);
        }
        break;
    }
    case WireFrameKind::Unknown:
        break;
    }
}

void NodeApiClient::failAll(const QString& message) {
    m_timeoutTimer.stop();
    m_handshake = Handshake::Disconnected;
    m_features.clear();
    m_outbox.clear();
    const QHash<quint64, Pending> pending = m_pending;
    m_pending.clear();
    for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
        if (it.value().stream) {
            emit streamEnded(it.key(), true, message);
        } else {
            emit failed(it.value().correlationId, message);
        }
    }
}

void NodeApiClient::armTimeoutScan() {
    if (!m_timeoutTimer.isActive()) {
        // Scan near the timeout granularity so a short test timeout fires promptly while production
        // stays cheap (one scan every <=500 ms).
        m_timeoutTimer.start(qBound(20, m_timeoutMs, 500));
    }
}

} // namespace daemonapp::daemon
