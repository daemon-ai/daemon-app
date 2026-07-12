// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_transport_registry.h"

#include "daemon/daemon_cache_store.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

namespace {

// [integrations wire v38] Project one decoded settings field into the display-only row the
// schema-driven add/edit-account form renders (the enriched kind/default/placeholder/choices lets
// it mask secrets + prefill).
QVariantMap settingsFieldToVariant(const daemonapp::daemon::DecodedSettingsField& f) {
    QVariantMap row;
    row[QStringLiteral("key")] = f.key;
    row[QStringLiteral("label")] = f.label;
    row[QStringLiteral("required")] = f.required;
    row[QStringLiteral("kind")] = f.kind;
    row[QStringLiteral("default")] = f.hasDefault ? f.defaultValue : QString();
    row[QStringLiteral("placeholder")] = f.hasPlaceholder ? f.placeholder : QString();
    row[QStringLiteral("choices")] = QVariant(f.choices);
    return row;
}

} // namespace

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
        // [acct-mgmt] forward the two-phase form descriptors + member list + op errors so both
        // surfaces render them off the same seam.
        connect(m_repo, &daemonapp::daemon::TransportRepository::joinDetailsReady, this,
                &ITransportRegistry::joinDetailsReady);
        connect(m_repo, &daemonapp::daemon::TransportRepository::createDetailsReady, this,
                &ITransportRegistry::createDetailsReady);
        connect(m_repo, &daemonapp::daemon::TransportRepository::membersReady, this,
                &ITransportRegistry::membersChanged);
        connect(m_repo, &daemonapp::daemon::TransportRepository::operationFailed, this,
                &ITransportRegistry::roomOperationFailed);
        // [integrations wire v38] re-emit the settings read-back as the seam's settingsChanged so
        // the edit-account form re-hydrates off the same seam both surfaces bind.
        connect(m_repo, &daemonapp::daemon::TransportRepository::settingsRefreshed, this,
                [this](const QString& transport) {
                    emit settingsChanged(transport, settings(transport));
                });
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
        // [acct-mgmt] Per-verb ops (wire v33): the key is present ONLY when the node reported a
        // concrete ops map, so consumers distinguish "authoritative per-verb info" (key present)
        // from "absent/null — fall back to the coarse capability" (key missing / undefined in
        // QML) without a parallel flag.
        if (a.hasConversationOps) {
            row[QStringLiteral("conversationOps")] = a.conversationOps;
        }
        if (a.hasMembershipOps) {
            row[QStringLiteral("membershipOps")] = a.membershipOps;
        }
        if (a.hasContactsOps) {
            row[QStringLiteral("contactsOps")] = a.contactsOps;
        }
        if (a.hasRosterOps) {
            row[QStringLiteral("rosterOps")] = a.rosterOps;
        }
        if (a.hasDirectory) {
            row[QStringLiteral("directory")] = a.directory;
        }
        // [integrations wire v38] the adapter's account-settings schema (the "Add/Edit account"
        // form's fields, with the enriched metadata). Empty list when the adapter reported none.
        QVariantList schema;
        for (const daemonapp::daemon::DecodedSettingsField& f : a.accountSchema) {
            schema.append(settingsFieldToVariant(f));
        }
        row[QStringLiteral("schema")] = schema;
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
        // [acct-mgmt] wire v35: the node-persisted label overlays the display name wherever the
        // account renders (account card/row, sidebar); the raw label is exposed for rename
        // pre-fill.
        row[QStringLiteral("displayName")] = i.label.isEmpty() ? i.displayName : i.label;
        row[QStringLiteral("label")] = i.label;
        row[QStringLiteral("enabled")] = i.enabled;
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
        row[QStringLiteral("kind")] = c.kind; // [integrations wire v38] may be "space" (container)
        row[QStringLiteral("title")] = c.title;
        row[QStringLiteral("topic")] = c.topic;
        // [integrations wire v38] the containing space id ("" = a root) — the tree groups by it.
        row[QStringLiteral("parent")] = c.parent;
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

// [acct-mgmt] wire v35: forward the reversible-lifecycle + label intents to the repository. An
// empty `label` maps to an explicit-null clear (hasLabel=false).
void DaemonTransportRegistry::connectAccount(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->connectTransport(transport);
    }
}

void DaemonTransportRegistry::setEnabled(const QString& transport, bool enabled) {
    if (m_repo != nullptr) {
        m_repo->setEnabled(transport, enabled);
    }
}

void DaemonTransportRegistry::setLabel(const QString& transport, const QString& label) {
    if (m_repo != nullptr) {
        m_repo->setTransportLabel(transport, !label.isEmpty(), label);
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

// [acct-mgmt] Room lifecycle + member management, forwarded to the repository.
void DaemonTransportRegistry::conversationJoinDetails(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->conversationJoinDetails(transport);
    }
}

void DaemonTransportRegistry::joinRoom(const QString& transport, const QVariantMap& form) {
    if (m_repo != nullptr) {
        m_repo->joinRoom(transport, form);
    }
}

void DaemonTransportRegistry::conversationCreateDetails(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->conversationCreateDetails(transport);
    }
}

void DaemonTransportRegistry::createRoom(const QString& transport, const QVariantMap& form) {
    if (m_repo != nullptr) {
        m_repo->createRoom(transport, form);
    }
}

void DaemonTransportRegistry::leaveRoom(const QString& transport, const QString& conversation) {
    if (m_repo != nullptr) {
        m_repo->leaveRoom(transport, conversation);
    }
}

void DaemonTransportRegistry::deleteRoom(const QString& transport, const QString& conversation) {
    if (m_repo != nullptr) {
        m_repo->deleteRoom(transport, conversation);
    }
}

void DaemonTransportRegistry::conversationMembers(const QString& transport,
                                                  const QString& conversation) {
    if (m_repo != nullptr) {
        m_repo->conversationMembers(transport, conversation);
    }
}

void DaemonTransportRegistry::memberInvite(const QString& transport, const QString& conversation,
                                           const QString& contactId) {
    if (m_repo != nullptr) {
        m_repo->memberInvite(transport, conversation, contactId);
    }
}

void DaemonTransportRegistry::memberKick(const QString& transport, const QString& conversation,
                                         const QString& contactId) {
    if (m_repo != nullptr) {
        m_repo->memberKick(transport, conversation, contactId);
    }
}

void DaemonTransportRegistry::memberBan(const QString& transport, const QString& conversation,
                                        const QString& contactId) {
    if (m_repo != nullptr) {
        m_repo->memberBan(transport, conversation, contactId);
    }
}

void DaemonTransportRegistry::memberSetRole(const QString& transport, const QString& conversation,
                                            const QString& contactId, const QString& role) {
    if (m_repo != nullptr) {
        m_repo->memberSetRole(transport, conversation, contactId, role);
    }
}

// [integrations wire v38] Account settings read + configure, delegated to the repository.
QVariantMap DaemonTransportRegistry::settings(const QString& transport) const {
    QVariantMap out;
    if (m_repo == nullptr) {
        return out;
    }
    const QMap<QString, QString> vals = m_repo->settings(transport);
    for (auto it = vals.constBegin(); it != vals.constEnd(); ++it) {
        out.insert(it.key(), it.value());
    }
    return out;
}

void DaemonTransportRegistry::refreshSettings(const QString& transport) {
    if (m_repo != nullptr) {
        m_repo->refreshSettings(transport);
    }
}

void DaemonTransportRegistry::configure(const QString& transport, const QVariantMap& values) {
    if (m_repo == nullptr) {
        return;
    }
    QMap<QString, QString> settingsMap;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        settingsMap.insert(it.key(), it.value().toString());
    }
    m_repo->configure(transport, settingsMap);
}

} // namespace transports
