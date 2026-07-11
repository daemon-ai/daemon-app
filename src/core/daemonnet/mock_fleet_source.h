// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/participant.h"
#include "domain/session.h"
#include "domain/tag.h"
#include "domain/unit_node.h"

#include <QList>
#include <QObject>

namespace uimodels {
class VariantListModel;
}

namespace daemonnet {

// The typed seed the session store copies at construction: the supervision units (the delegation
// tree), the sessions (each with its authoritative `sessionId`, content, tags, owning unit), the
// client-local tags, and the transcript participants.
struct SeedBundle {
    QList<domain::UnitNode> units;
    QList<domain::Session> sessions;
    QList<domain::Tag> tags;
    QList<domain::Participant> participants;
};

// The mock fleet/roster/session seed (the M3-era successor of the deleted mock network seam,
// stripped of the routing / transports-tree / patch-bay surfaces the mirror now owns). It builds
// the fleet-of-fleets supervision units + their sessions + tags, exposes the fleet + session-roster
// projections the mock Fleet/Sessions pages render, and hands the typed `seed()` the mock
// InMemorySessionStore copies. It is a plain seed (no network-seam interface) and carries no
// routing state — it is the EXISTING non-mirror mock source for the sessions/fleet surfaces (A7's
// M4 wave), kept alive so those surfaces still work in mock mode after the M3 seam deletion.
class MockFleetSource : public QObject {
    Q_OBJECT

public:
    // Projects the canonical demo bundle (defaultSeed()).
    explicit MockFleetSource(QObject* parent = nullptr);
    // Projects a caller-supplied bundle — the mock scenario (A8/M5) is the seed source; this
    // class only PROJECTS (fleet tree + roster rows) and hands the bundle to the session store.
    explicit MockFleetSource(const SeedBundle& bundle, QObject* parent = nullptr);

    // The canonical fleet-of-fleets demo content (units + sessions + tags + participants) — the
    // single source the default mock scenario builds on (its mirror rows are derived from this,
    // so the mirror session ids and the delegated transcript store's ids provably agree). A
    // static member (not a free function) so the tr() context of the participant labels is
    // unchanged for the i18n catalogs.
    [[nodiscard]] static SeedBundle defaultSeed();

    // Fleet projection: the delegation spanning tree (pre-order), rows
    // { depth, id, name, kind(orchestrator|worker), status, model } — the IFleetTree row contract.
    [[nodiscard]] QObject* fleet() const;
    // Flat session projection (the roster), rows
    // { id, title, profile, state, lifecycle, lastActivity, tokens, rewindable } — ISessionRoster.
    [[nodiscard]] QObject* sessions() const;

    // The typed seed bundle the session store copies (units + sessions + tags + participants).
    [[nodiscard]] SeedBundle seed() const;

private:
    void computeProjections();
    void projectFleet();
    void projectSessions();

    QList<domain::UnitNode> m_units;
    QList<domain::Session> m_sessions;
    QList<domain::Tag> m_tags;
    QList<domain::Participant> m_participants;

    uimodels::VariantListModel* m_fleetModel = nullptr;
    uimodels::VariantListModel* m_sessionsModel = nullptr;
};

} // namespace daemonnet
