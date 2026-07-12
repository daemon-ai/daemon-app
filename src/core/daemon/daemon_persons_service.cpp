// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_persons_service.h"

#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

namespace {

// Project one DecodedPerson into the display-only row map the surfaces render.
QVariantMap personToVariant(const daemonapp::daemon::DecodedPerson& p) {
    QVariantMap row;
    row[QStringLiteral("id")] = p.id;
    row[QStringLiteral("alias")] = p.hasAlias ? p.alias : QString();
    QVariantList endpoints;
    for (const daemonapp::daemon::DecodedPersonEndpoint& ep : p.endpoints) {
        QVariantMap e;
        e[QStringLiteral("transport")] = ep.transport;
        e[QStringLiteral("id")] = ep.contact.id;
        e[QStringLiteral("displayName")] = ep.contact.displayName;
        e[QStringLiteral("presence")] = ep.contact.presence;
        e[QStringLiteral("permission")] = ep.contact.permission;
        endpoints.append(e);
    }
    row[QStringLiteral("endpoints")] = endpoints;
    return row;
}

QVariantList personsToVariant(const QList<daemonapp::daemon::DecodedPerson>& persons) {
    QVariantList out;
    out.reserve(persons.size());
    for (const daemonapp::daemon::DecodedPerson& p : persons) {
        out.append(personToVariant(p));
    }
    return out;
}

} // namespace

DaemonPersonsService::DaemonPersonsService(daemonapp::daemon::PersonsRepository* repo,
                                           QObject* parent)
    : IPersonsService(parent), m_repo(repo) {
    if (m_repo == nullptr) {
        return;
    }
    connect(m_repo, &daemonapp::daemon::PersonsRepository::personsRefreshed, this,
            [this] { emit personsChanged(personsToVariant(m_repo->persons())); });
    connect(m_repo, &daemonapp::daemon::PersonsRepository::operationFailed, this,
            &IPersonsService::personsOperationFailed);
}

QVariantList DaemonPersonsService::persons() const {
    if (m_repo == nullptr) {
        return {};
    }
    return personsToVariant(m_repo->persons());
}

void DaemonPersonsService::refresh() {
    if (m_repo != nullptr) {
        m_repo->refresh();
    }
}

} // namespace transports
