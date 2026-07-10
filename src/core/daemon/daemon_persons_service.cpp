// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_persons_service.h"

#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"

namespace transports {

// ===== STUB (red) — implemented in the green commit =====

DaemonPersonsService::DaemonPersonsService(daemonapp::daemon::PersonsRepository* repo,
                                           QObject* parent)
    : IPersonsService(parent), m_repo(repo) {}

QVariantList DaemonPersonsService::persons() const {
    return {};
}

void DaemonPersonsService::refresh() {}

} // namespace transports
