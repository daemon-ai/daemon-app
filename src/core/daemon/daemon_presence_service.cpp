// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_presence_service.h"

#include "daemon/daemon_cache_store.h"
#include "daemon/repositories.h"

#include <QSet>

namespace transports {

DaemonPresenceService::DaemonPresenceService(daemonapp::daemon::TransportRepository* repo,
                                             QObject* parent)
    : IPresenceService(parent), m_repo(repo) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::TransportRepository::instancesRefreshed, this,
                &DaemonPresenceService::rebuild);
        rebuild(); // offline-first: seed from the cache before any refresh
    }
}

QString DaemonPresenceService::connectionState(const QString& transport) const {
    return m_connection.value(transport, QStringLiteral("offline"));
}

QString DaemonPresenceService::presence(const QString& transport) const {
    return m_presence.value(transport, QStringLiteral("unknown"));
}

void DaemonPresenceService::rebuild() {
    if (m_repo == nullptr) {
        return;
    }
    const QHash<QString, QString> prevConnection = m_connection;
    m_connection.clear();
    m_presence.clear();
    QSet<QString> touched;
    for (const daemonapp::daemon::CachedTransportInstanceRow& i : m_repo->cachedInstances()) {
        m_connection.insert(i.transport, i.connection);
        m_presence.insert(i.transport, i.presence);
        touched.insert(i.transport);
    }
    // Notify per-transport for whatever changed (or dropped) so the status dots re-read.
    for (auto it = prevConnection.constBegin(); it != prevConnection.constEnd(); ++it) {
        touched.insert(it.key());
    }
    for (const QString& transport : touched) {
        emit presenceChanged(transport);
    }
}

} // namespace transports
