#pragma once

#include "domain/ids.h"

#include <QDateTime>
#include <QList>
#include <QMetaType>
#include <QString>

namespace domain {

// A session: a running conversation/transcript (mirrors a daemon `SessionId` /
// `SessionInfo`). For now the whole transcript is a single markdown string
// (`content`); a structured message model can replace it later behind the store
// seam without changing this type's role. `id` is the local row handle (int);
// `unitId` is the supervision unit this session belongs to.
struct Session {
    int id = -1;
    // The unit this session belongs to (a `UnitId`; empty if unassigned). Maps
    // later to a unit's transcript drained via `unit_outbound(unitId)`: a leaf
    // engine owns exactly one session, while a parent node folds its whole
    // subtree's sessions.
    UnitId unitId;
    QList<int> tagIds;
    QString title;
    QString content; // markdown
    bool isArchived = false;
    // Pinned sessions float to the top of any list scope (client-side action; the
    // daemon carries the same flag on SessionInfo).
    bool isPinned = false;
    QDateTime created;
    QDateTime modified;

    [[nodiscard]] bool isValid() const { return id >= 0; }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::Session)
