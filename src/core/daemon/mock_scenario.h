// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror A8 (spec 09 §9, wave M5) — the mock scenario catalog. A scenario is the ONLY seed source
// of mock mode: one declaration feeds BOTH the mirror (via mirror::Seeder — canonical entities
// through the same apply pipeline, journal origin = seeder) and the surviving legacy delegate
// stores (the SeedBundle behind InMemorySessionStore / MockFleetTree — the transcript-content +
// fleet-tree sources until A7T/A9 retire them). Deriving the mirror session/fleet rows FROM the
// bundle makes the two sides' ids provably agree — the delegated content() join depends on it.
//
// The scenario also carries the mock Hello's api/<N> (§5.6/§6.8: 38 = degraded refetch_diff +
// auto-replay OFF; 39 = full wire_delta + Bootstrap + auto-replay ON) and the per-verb outcome
// script (§9 scripted outcomes, MANDATORY rejection rules included). `DAEMON_APP_MOCK_SCENARIO`
// selects a built-in by name; this replaces the deleted DAEMON_APP_MOCK_INTEGRATIONS mechanism
// (no alias — the env var and its demo tree died with the M3 seam deletion).

#include "daemonnet/mock_fleet_source.h"
#include "mirror/seeder.h"

#include <QString>
#include <QStringList>

namespace daemonapp::daemon {

struct MockScenario {
    QString name;                 // built-in scenario id ("default" | "api38" | "empty")
    daemonnet::SeedBundle bundle; // session/fleet DOMAIN seed (legacy delegate stores)
    mirror::Scenario mirror;      // canonical mirror seed + api/<N> + verb script + timeline
};

// The built-in catalog. Unknown names warn and fall back to "default".
[[nodiscard]] QStringList mockScenarioNames();
[[nodiscard]] MockScenario mockScenarioByName(const QString& name);

// Resolve from DAEMON_APP_MOCK_SCENARIO (unset/empty = "default").
[[nodiscard]] MockScenario mockScenarioFromEnvironment();

} // namespace daemonapp::daemon
