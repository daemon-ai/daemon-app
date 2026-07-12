// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/session.h"
#include "domain/unit_node.h"

#include <QList>

namespace daemonnet {

// The canonical mock DOMAIN seed (AD: extracted from the deleted MockFleetSource): the
// supervision units (the delegation tree) + the sessions (each with its authoritative
// `sessionId`, content, owning unit). The mock scenario (mock_scenario.cpp) is the ONLY
// consumer — it derives the mirror `sessions`/`fleet_units` rows and the transcript blocks from
// this ONE declaration, so every mock surface renders the same ids by construction (§9).
// The legacy tags/participants members died with their only reader (InMemorySessionStore).
struct SeedBundle {
    QList<domain::UnitNode> units;
    QList<domain::Session> sessions;
};

// The canonical fleet-of-fleets demo content (units + sessions).
[[nodiscard]] SeedBundle defaultSeedBundle();

} // namespace daemonnet
