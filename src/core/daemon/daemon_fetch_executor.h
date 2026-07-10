// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror A5 — the daemon-mode FetchExecutor (spec 09 §5.4, 07§10). Fulfils the ingestor's
// FetchScheduler jobs over the real NodeApiClient: encodes the wire request, sends it (correlated),
// decodes the reply straight into mirror entities via the decode-to-mirror bridge (mirror_decode),
// hands the ingestor already-mapped entities through deliver*(), and completes the scheduler slot.
// Internal page loops (ConvList/RosterList replace-and-prune over the accumulated union;
// ConvHistory forward-cursor until it reaches head) stay inside one scheduler job (§5.4). The codec
// + zcbor live only here + the bridge (07§10 boundary); the ingestor never sees CBOR.

#include "daemon/node_api_client.h"
#include "mirror/fetch_job.h"
#include "mirror/fetch_scheduler.h"
#include "mirror/generated/entities_gen.h"
#include "mirror/ingestor.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <vector>

namespace daemonapp::daemon {

class DaemonFetchExecutor : public QObject, public mirror::FetchExecutor {
    Q_OBJECT

public:
    DaemonFetchExecutor(NodeApiClient* api, mirror::Ingestor& ingestor,
                        mirror::FetchScheduler& scheduler, QObject* parent = nullptr);

    void execute(const mirror::FetchJob& job) override;

private:
    struct InFlight {
        mirror::FetchJob job;
        QString transport;
        QString conv;
        std::vector<mirror::Conversation> convAccum;
        std::vector<mirror::Contact> contactAccum;
    };

    void onResponse(const QString& correlationId, const QByteArray& responseCbor);
    void onFailed(const QString& correlationId, const QString& message);
    void finish(const QString& correlationId); // complete the scheduler slot + drop the in-flight

    // Issue (or re-issue for the next page) the wire request for `f` under its correlation id.
    void sendFor(const InFlight& f, const QString& pageToken, quint64 afterCursor);

    NodeApiClient* m_api = nullptr;
    mirror::Ingestor& m_ingestor;
    mirror::FetchScheduler& m_scheduler;
    QHash<QString, InFlight> m_inflight; // correlation id -> paging state
};

} // namespace daemonapp::daemon
