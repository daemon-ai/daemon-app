// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/ipresence_service.h"

#include <QHash>
#include <QString>

namespace daemonapp::daemon {
class TransportRepository;
}

namespace transports {

// Daemon-backed, offline-first per-account presence (Phase 6a, EIO-9 read). Projects the
// TransportRepository's cached transport instances into the Pidgin-style status the roster/channels
// status dots bind to: connectionState/presence look up the last-known cached row (so they render
// with no connection), refreshed on the repository's instancesRefreshed.
//
// Caveat: the daemon's connection/presence is coarse today (Matrix: credential-present heuristic,
// presence Unknown) and poll/refresh-driven, not a live push - this seam renders the honest
// last-known state. Real live presence is a deferred backend follow-up.
//
// Under src/core/daemon to avoid a library cycle (depends on the daemon TransportRepository).
class DaemonPresenceService : public IPresenceService {
    Q_OBJECT

public:
    explicit DaemonPresenceService(daemonapp::daemon::TransportRepository* repo,
                                   QObject* parent = nullptr);

    [[nodiscard]] QString connectionState(const QString& transport) const override;
    [[nodiscard]] QString presence(const QString& transport) const override;

private:
    void rebuild();

    daemonapp::daemon::TransportRepository* m_repo = nullptr;
    QHash<QString, QString> m_connection; // transport -> connection state
    QHash<QString, QString> m_presence;   // transport -> presence
};

} // namespace transports
