// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_transport_registry.h"

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

DaemonTransportRegistry::DaemonTransportRegistry(daemonapp::daemon::TransportRepository* repo,
                                                 QObject* parent)
    : ITransportRegistry(parent), m_repo(repo) {
    if (m_repo != nullptr) {
        connect(m_repo, &daemonapp::daemon::TransportRepository::adaptersRefreshed, this,
                &ITransportRegistry::adaptersChanged);
        connect(m_repo, &daemonapp::daemon::TransportRepository::instancesRefreshed, this,
                &ITransportRegistry::instancesChanged);
        connect(m_repo, &daemonapp::daemon::TransportRepository::conversationsRefreshed, this,
                &ITransportRegistry::conversationsChanged);
    }
}

QVariantList DaemonTransportRegistry::availableAdapters() const {
    QVariantList out;
    if (m_repo == nullptr) {
        return out;
    }
    for (const daemonapp::daemon::DecodedAdapterInfo& a : m_repo->adapters()) {
        QVariantMap row;
        row[QStringLiteral("family")] = a.family;
        row[QStringLiteral("displayName")] = a.displayName;
        row[QStringLiteral("capabilities")] = a.capabilities;
        // [waveB:app-v30] D3: node-labeled policy rows ({key,label,value}); rendered read-only.
        QVariantList policies;
        for (const QVariantMap& p : a.policies) {
            policies.append(p);
        }
        row[QStringLiteral("policies")] = policies;
        out.append(row);
    }
    return out;
}

QVariantList DaemonTransportRegistry::instances() const {
    QVariantList out;
    if (m_repo == nullptr) {
        return out;
    }
    // Offline-first: project the cached accounts (render with no daemon connection).
    for (const daemonapp::daemon::CachedTransportInstanceRow& i : m_repo->cachedInstances()) {
        QVariantMap row;
        row[QStringLiteral("transport")] = i.transport;
        row[QStringLiteral("family")] = i.family;
        row[QStringLiteral("displayName")] = i.displayName;
        row[QStringLiteral("boundProfile")] = i.boundProfile;
        // [waveB:app-v30] D1: node-reported disconnect provenance (offline-first).
        row[QStringLiteral("connectionReason")] = i.connectionReason;
        row[QStringLiteral("connectionMessage")] = i.connectionMessage;
        row[QStringLiteral("fatal")] = i.fatal;
        out.append(row);
    }
    return out;
}

QVariantList DaemonTransportRegistry::conversations(const QString& transport) const {
    QVariantList out;
    if (m_repo == nullptr) {
        return out;
    }
    // Offline-first: the last-known rooms for this account (live ConvList refreshes them).
    for (const daemonapp::daemon::CachedConversationRow& c :
         m_repo->cachedConversations(transport)) {
        QVariantMap row;
        row[QStringLiteral("transport")] = c.transport;
        row[QStringLiteral("id")] = c.convId;
        row[QStringLiteral("kind")] = c.kind;
        row[QStringLiteral("title")] = c.title;
        row[QStringLiteral("topic")] = c.topic;
        out.append(row);
    }
    return out;
}

void DaemonTransportRegistry::refreshConversations(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->refreshConversations(transport);
    }
}

// [waveB:app-v30] D1: forward the teardown intents to the repository.
void DaemonTransportRegistry::disconnect(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->disconnect(transport);
    }
}

void DaemonTransportRegistry::remove(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->remove(transport);
    }
}

// [wave2:app-channels-liveness] B2: delegate the "new room" affordance to the repository's
// (presentation-only, in-memory) seen-set tracking.
bool DaemonTransportRegistry::isNewConversation(const QString& transport,
                                                const QString& conversation) const {
    return m_repo != nullptr && m_repo->isNewConversation(transport, conversation);
}

void DaemonTransportRegistry::markConversationSeen(const QString& transport,
                                                   const QString& conversation) {
    if (m_repo != nullptr) {
        m_repo->markConversationSeen(transport, conversation);
    }
}

} // namespace transports
