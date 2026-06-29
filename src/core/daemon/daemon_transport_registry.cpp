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

} // namespace transports
