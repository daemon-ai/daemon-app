#pragma once
// mirror::parity — dual-write parity asserts (spec 09 §13 M1 exit criterion; dual-write
// discipline §13). During the migration waves the legacy repositories keep writing their caches
// while the ingestor feeds the mirror; at quiesce points a parity check compares the mirror's
// row-set for a domain against the legacy repository's row-set and logs-on-mismatch in dev builds
// (never silent divergence). This is a diagnostic, not a gate on the hot path.

#include "mirror/model.h"

#include <QSet>
#include <QString>
#include <QStringList>

namespace mirror::parity {

// The result of one domain's row-set comparison.
struct Result {
    QStringList onlyInMirror; // keys the mirror has that the legacy cache lacks
    QStringList onlyInLegacy; // keys the legacy cache has that the mirror lacks
    [[nodiscard]] bool matches() const noexcept {
        return onlyInMirror.isEmpty() && onlyInLegacy.isEmpty();
    }
};

// Row-set parity by canonical key (§13: "compare row sets per domain").
[[nodiscard]] Result compareKeys(const QSet<QString>& mirrorKeys, const QSet<QString>& legacyKeys);

// Extract mirror Conversation keys (canonical serialize()) for one transport, for comparison
// against the legacy TransportRegistry's conversations list.
[[nodiscard]] QSet<QString> conversationKeys(const MirrorModel& snapshot, const QString& transport);

// Extract mirror Contact keys for one transport (vs the legacy ContactsRepository roster).
[[nodiscard]] QSet<QString> contactKeys(const MirrorModel& snapshot, const QString& transport);

// Extract mirror Session keys (canonical serialize() == the authoritative session id) for the
// whole roster, for mirror-vs-mirror feeder comparison (mock Seeder vs daemon ingestor delivery
// (spec 09 §13 M4: "parity asserts vs legacy roster until deletion").
[[nodiscard]] QSet<QString> sessionKeys(const MirrorModel& snapshot);

// Extract mirror FleetUnit keys (canonical serialize() == the unit id) for the whole fleet, for
// comparison against the legacy fleet-tree row-set (§13 M4 FleetUnit unification).
[[nodiscard]] QSet<QString> fleetUnitKeys(const MirrorModel& snapshot);

// Log a mismatch under the daemon.mirror.parity category (dev builds); returns matches() so
// callers can `Q_ASSERT` in debug scenarios (§13 "log-on-mismatch in dev").
bool checkAndLog(const QString& domain, const Result& result);

} // namespace mirror::parity
