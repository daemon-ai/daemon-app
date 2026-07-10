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
    QByteArray req;
    switch (f.job.op) {
    case mirror::FetchOp::ConvList:
    case mirror::FetchOp::ConvGet:
        // ConvGet (a single added conversation) is fulfilled by re-listing the transport — the
        // node owns membership; the replace-and-prune reconciles the added/removed row. (A keyed
        // ConvGet decoder is a later optimization; correctness holds under dual-write.)
        req = NodeApiCodec::encodeConvListRequest(f.transport, pageToken);
        break;
    case mirror::FetchOp::RosterList:
        req = NodeApiCodec::encodeRosterListRequest(f.transport, pageToken);
        break;
    case mirror::FetchOp::PersonList:
        req = NodeApiCodec::encodePersonListRequest();
        break;
    case mirror::FetchOp::ConvHistory:
        req = NodeApiCodec::encodeConvHistoryRequest(f.transport, f.conv, afterCursor != 0,
                                                     afterCursor);
        break;
    default:
        // Ops A5 does not yet fulfil on the mirror path (routing/rooms/etc.) complete immediately;
        // the legacy repositories still serve those surfaces (dual-write). A6+ ports them.
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
    switch (f.job.op) {
    case mirror::FetchOp::ConvList:
    case mirror::FetchOp::ConvGet: {
        std::vector<mirror::Conversation> page;
        QString next;
        if (!decodeConversationsToMirror(responseCbor, &page, &next)) {
            finish(correlationId);
            return;
        }
        for (auto& c : page) {
            f.convAccum.push_back(std::move(c));
        }
        if (!next.isEmpty()) {
            sendFor(f, next, 0); // next page — same scheduler job
            return;
        }
        m_ingestor.deliverConversations(f.transport, f.convAccum, /*isFinalPage=*/true);
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::RosterList: {
        std::vector<mirror::Contact> page;
        QString next;
        if (!decodeContactsToMirror(responseCbor, f.transport, &page, &next)) {
            finish(correlationId);
            return;
        }
        for (auto& c : page) {
            f.contactAccum.push_back(std::move(c));
        }
        if (!next.isEmpty()) {
            sendFor(f, next, 0);
            return;
        }
        m_ingestor.deliverContacts(f.transport, f.contactAccum, /*isFinalPage=*/true);
        finish(correlationId);
        return;
    }
    case mirror::FetchOp::PersonList: {
        std::vector<mirror::Person> persons;
        if (decodePersonsToMirror(responseCbor, &persons)) {
            m_ingestor.deliverPersons(persons, /*isFinalPage=*/true);
        }
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
