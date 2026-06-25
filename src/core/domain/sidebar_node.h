#pragma once

#include "domain/ids.h"

#include <QMetaType>
#include <QString>

namespace domain {

// The kinds of rows the sidebar shows. The numeric order is load-bearing: QML
// branches on these integer values, so keep positions stable when extending.
enum class NodeType {
    AllSessions = 0, // 0: every non-archived session
    Archived,             // 1: archived sessions
    FleetSeparator,       // 2: the "Fleet" section header (with + add-root)
    TagSeparator,         // 3: the "Tags" section header (with + add-tag)
    Unit,                 // 4: a unit node in the supervision tree
    Tag,                  // 5: a tag label
    // Appended (positions above are load-bearing - QML branches on the integer
    // values). The DaemonNet "lens" scopes regroup the same sessions by their
    // transport instance / remote peer; `lensKey` carries the grouping key.
    ByTransport,          // 6: sessions over one transport instance (lensKey = transport id)
    ByPeer,               // 7: sessions with one remote peer (lensKey = peer id)
};

// What the session list is currently filtered to (the sidebar selection). For
// `Tag`, `tagId` is the tag id. For `Unit`, `unitId` is the supervision unit id
// and the scope folds over that unit's WHOLE subtree (the unit and all
// descendants), which is the same logic for every unit regardless of kind/depth.
// For `ByTransport`/`ByPeer`, `lensKey` is the transport-instance / peer id.
struct ListScope {
    NodeType type = NodeType::AllSessions;
    int tagId = -1;     // tag id for Tag scope; ignored otherwise
    UnitId unitId;      // unit id for Unit scope; empty otherwise
    QString lensKey;    // grouping key for ByTransport/ByPeer; empty otherwise

    friend bool operator==(const ListScope& a, const ListScope& b)
    {
        return a.type == b.type && a.tagId == b.tagId && a.unitId == b.unitId
            && a.lensKey == b.lensKey;
    }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::NodeType)
Q_DECLARE_METATYPE(domain::ListScope)
