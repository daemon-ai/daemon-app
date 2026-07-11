// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_fetch_executor.h"

#include "daemon/mirror_decode.h"
#include "daemon/node_api_codec.h"
#include "mirror/policy_table.h"

#include <QChar>

namespace daemonapp::daemon {

namespace {
// Split a job scope-tail into transport [+ conv] (unit-separator, §3.1).
void splitScope(const QString& scope, QString* transport, QString* conv) {
    const qsizetype sep = scope.indexOf(QChar(0x1f));
    if (sep < 0) {
        *transport = scope;
        conv->clear();
    } else {
        *transport = scope.left(sep);
        *conv = scope.mid(sep + 1);
    }
}
} // namespace

DaemonFetchExecutor::DaemonFetchExecutor(NodeApiClient* api, mirror::Ingestor& ingestor,
                                         mirror::FetchScheduler& scheduler, QObject* parent)
    : QObject(parent), m_api(api), m_ingestor(ingestor), m_scheduler(scheduler) {
    if (m_api != nullptr) {
        connect(m_api, &NodeApiClient::responseReady, this, &DaemonFetchExecutor::onResponse);
        connect(m_api, &NodeApiClient::failed, this, &DaemonFetchExecutor::onFailed);
    }
}

void DaemonFetchExecutor::execute(const mirror::FetchJob& job) {
    if (m_api == nullptr) {
        m_scheduler.complete(job.coalesceKey());
        return;
    }
    InFlight f;
    f.job = job;
    splitScope(job.scope, &f.transport, &f.conv);
    const QString corr = job.correlation();
    m_inflight.insert(corr, f);
    sendFor(m_inflight[corr], QString(), job.afterCursor);
}

void DaemonFetchExecutor::sendFor(const InFlight& f, const QString& pageToken,
                                  quint64 afterCursor) {
    // [api/39 §10.2] A since_rev delta read: send it on the FIRST page of a full-mode job (a
    // sinceRev of 0 is a full read — the bootstrap epoch-restart fallback). Subsequent pages resume
    // via the `after` token, which continues the same delta query node-side.
    const bool deltaRead = f.job.fullMode && f.job.sinceRev > 0;
    const bool firstPage = pageToken.isEmpty();
    const bool sendSinceRev = deltaRead && firstPage;
    QByteArray req;
    switch (f.job.op) {
    case mirror::FetchOp::ConvList:
    case mirror::FetchOp::ConvGet:
        // ConvGet (a single added conversation) is fulfilled by re-listing the transport — the
        // node owns membership; the replace-and-prune reconciles the added/removed row. (A keyed
        // ConvGet decoder is a later optimization; correctness holds under dual-write.)
        req = NodeApiCodec::encodeConvListRequest(f.transport, pageToken, sendSinceRev,
                                                  f.job.sinceRev);
        break;
    case mirror::FetchOp::RosterList:
        req = NodeApiCodec::encodeRosterListRequest(f.transport, pageToken, sendSinceRev,
                                                    f.job.sinceRev);
        break;
    case mirror::FetchOp::PersonList:
        req = NodeApiCodec::encodePersonListRequest(sendSinceRev, f.job.sinceRev);
        break;
    case mirror::FetchOp::ConvHistory:
        req = NodeApiCodec::encodeConvHistoryRequest(f.transport, f.conv, afterCursor != 0,
                                                     afterCursor);
        break;
    case mirror::FetchOp::Bootstrap:
        // [api/39 §10.3] The reconnect baseline probe (§5.6 full step 1): the reply decodes to
        // {cursor, epoch, revs} and drives ingestor.onBootstrap(), which issues the delta reads.
        req = NodeApiCodec::encodeBootstrapRequest();
        break;
    case mirror::FetchOp::RoutingListChats:
        // M3: the origin→session pin table. Global list; `after` resumes the page loop.
        req = NodeApiCodec::encodeRoutingListChatsRequest(pageToken);
        break;
    case mirror::FetchOp::TransportRooms:
        // M3: a transport instance's bindable rooms; `after` resumes the page loop.
        req = NodeApiCodec::encodeTransportRoomsRequest(f.transport, pageToken);
        break;
    default:
        // Ops the mirror path does not fulfil yet (sessions/fleet/etc. — A7's M4 wave) complete
        // immediately; the legacy repositories still serve those surfaces (dual-write).
        m_scheduler.complete(f.job.coalesceKey());
        m_inflight.remove(f.job.correlation());
        return;
    }
    m_api->sendRequest(req, f.job.correlation());
}

void DaemonFetchExecutor::onResponse(const QString& correlationId, const QByteArray& responseCbor) {
    auto it = m_inflight.find(correlationId);
    if (it == m_inflight.end()) {
        return; // not ours (a legacy repository correlation)
    }
    InFlight& f = it.value();
    // [api/39 §10.2] A since_rev delta read (fullMode with a non-zero sinceRev) applies through the
    // ingestor's delta seam — upsert changed + tombstone `removed`, no replace-and-prune. A full
    // read (sinceRev == 0, incl. ConvGet re-lists) keeps the full-list replace-and-prune path.
    const bool deltaRead = f.job.fullMode && f.job.sinceRev > 0;
    switch (f.job.op) {
    case mirror::FetchOp::ConvList:
    case mirror::FetchOp::ConvGet: {
        std::vector<mirror::Conversation> page;
        QString next;
        quint64 rev = 0;
        QStringList removed;
        if (!decodeConversationsToMirror(responseCbor, &page, &next, &rev, &removed)) {
            finish(correlationId);
            return;
        }
        for (auto& c : page) {
            f.convAccum.push_back(std::move(c));
        }
        f.removedAccum += removed;
        f.rev = rev;
        if (!next.isEmpty()) {
            sendFor(f, next, 0); // next page — same scheduler job
            return;
        }
        if (deltaRead) {
            m_ingestor.deliverConversationsDelta(f.transport, f.convAccum, f.removedAccum, f.rev,
                                                 /*isFinalPage=*/true);
        } else {
            m_ingestor.deliverConversations(f.transport, f.convAccum, /*isFinalPage=*/true);
        }
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::RosterList: {
        std::vector<mirror::Contact> page;
        QString next;
        quint64 rev = 0;
        QStringList removed;
        if (!decodeContactsToMirror(responseCbor, f.transport, &page, &next, &rev, &removed)) {
            finish(correlationId);
            return;
        }
        for (auto& c : page) {
            f.contactAccum.push_back(std::move(c));
        }
        f.removedAccum += removed;
        f.rev = rev;
        if (!next.isEmpty()) {
            sendFor(f, next, 0);
            return;
        }
        if (deltaRead) {
            m_ingestor.deliverContactsDelta(f.transport, f.contactAccum, f.removedAccum, f.rev,
                                            /*isFinalPage=*/true);
        } else {
            m_ingestor.deliverContacts(f.transport, f.contactAccum, /*isFinalPage=*/true);
        }
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::PersonList: {
        std::vector<mirror::Person> persons;
        quint64 rev = 0;
        QStringList removed;
        if (decodePersonsToMirror(responseCbor, &persons, &rev, &removed)) {
            if (deltaRead) {
                m_ingestor.deliverPersonsDelta(persons, removed, rev, /*isFinalPage=*/true);
            } else {
                m_ingestor.deliverPersons(persons, /*isFinalPage=*/true);
            }
        }
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::Bootstrap: {
        // [api/39 §10.3] Decode the probe → drive the reconnect baseline (§5.6 full step 1-2).
        quint64 cursor = 0;
        quint64 epoch = 0;
        QHash<QString, quint64> revs;
        if (NodeApiCodec::decodeBootstrap(responseCbor, &cursor, &epoch, &revs)) {
            m_ingestor.onBootstrap(cursor, epoch, revs);
        }
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::RoutingListChats: {
        std::vector<mirror::RoutePin> page;
        QString nextPage;
        if (!decodeRoutePinsToMirror(responseCbor, &page, &nextPage)) {
            finish(correlationId);
            return;
        }
        for (auto& p : page) {
            f.routePinAccum.push_back(std::move(p));
        }
        if (!nextPage.isEmpty()) {
            sendFor(f, nextPage, 0); // next page — same scheduler job
            return;
        }
        m_ingestor.deliverRoutePins(f.routePinAccum, /*isFinalPage=*/true);
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::TransportRooms: {
        std::vector<mirror::Room> page;
        QString nextPage;
        if (!decodeRoomsToMirror(responseCbor, f.transport, &page, &nextPage)) {
            finish(correlationId);
            return;
        }
        for (auto& r : page) {
            f.roomAccum.push_back(std::move(r));
        }
        if (!nextPage.isEmpty()) {
            sendFor(f, nextPage, 0);
            return;
        }
        m_ingestor.deliverRooms(f.transport, f.roomAccum, /*isFinalPage=*/true);
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::ConvHistory: {
        std::vector<mirror::ChatMessage> page;
        quint64 nextCursor = 0;
        quint64 headCursor = 0;
        if (!decodeChatHistoryToMirror(responseCbor, f.transport, f.conv, &page, &nextCursor,
                                       &headCursor)) {
            finish(correlationId);
            return;
        }
        m_ingestor.deliverChatPage(f.transport, f.conv, page, nextCursor, headCursor);
        // Forward page loop (§13 M1 cursor fix): continue until the window abuts head.
        if (nextCursor < headCursor && !page.empty()) {
            sendFor(f, QString(), nextCursor);
            return;
        }
        finish(correlationId);
        return;
    }
    default:
        finish(correlationId);
        return;
    }
}

void DaemonFetchExecutor::onFailed(const QString& correlationId, const QString& message) {
    auto it = m_inflight.find(correlationId);
    if (it == m_inflight.end()) {
        return;
    }
    m_ingestor.fetchFailed(it.value().job, message);
    finish(correlationId);
}

void DaemonFetchExecutor::finish(const QString& correlationId) {
    auto it = m_inflight.find(correlationId);
    if (it == m_inflight.end()) {
        return;
    }
    const mirror::FetchJob job = it.value().job;
    m_inflight.erase(it);
    m_scheduler.complete(job.coalesceKey());
}

} // namespace daemonapp::daemon
