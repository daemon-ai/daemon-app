// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/ipersons_service.h"

namespace daemonapp::daemon {
class PersonsRepository;
}

namespace transports {

// [integrations wire v38] Daemon-backed persons seam. Projects the PersonsRepository's node data
// (PersonList) into the IPersonsService rows the integrations tree's Persons section renders, and
// forwards refresh(). Re-projects/re-emits on the repository's personsRefreshed signal; refresh
// piggybacks the node's PersonsChanged feed event (wired in the service graph).
//
// Lives under src/core/daemon (not transports/) because it depends on the daemon PersonsRepository,
// which links the transports interface — keeping it here avoids a library cycle (cf.
// DaemonContactsService).
class DaemonPersonsService : public IPersonsService {
    Q_OBJECT

public:
    explicit DaemonPersonsService(daemonapp::daemon::PersonsRepository* repo,
                                  QObject* parent = nullptr);

    [[nodiscard]] QVariantList persons() const override;
    void refresh() override;

private:
    daemonapp::daemon::PersonsRepository* m_repo = nullptr;
};

} // namespace transports
