#pragma once

#include "domain/ids.h"

#include <QMetaType>
#include <QString>

namespace domain {

// The kinds of rows the sidebar shows. The numeric order is load-bearing: QML
// branches on these integer values, so keep positions stable when extending.
enum class NodeType {
    AllConversations = 0, // 0: every non-archived session
    Archived,             // 1: archived sessions
    FleetSeparator,       // 2: the "Fleet" section header (with + add-root)
    TagSeparator,         // 3: the "Tags" section header (with + add-tag)
    Unit,                 // 4: a unit node in the supervision tree
    Tag,                  // 5: a tag label
};

// What the session list is currently filtered to (the sidebar selection). For
// `Tag`, `tagId` is the tag id. For `Unit`, `unitId` is the supervision unit id
// and the scope folds over that unit's WHOLE subtree (the unit and all
// descendants), which is the same logic for every unit regardless of kind/depth.
struct ListScope {
    NodeType type = NodeType::AllConversations;
    int tagId = -1;     // tag id for Tag scope; ignored otherwise
    UnitId unitId;      // unit id for Unit scope; empty otherwise

    friend bool operator==(const ListScope& a, const ListScope& b)
    {
        return a.type == b.type && a.tagId == b.tagId && a.unitId == b.unitId;
    }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::NodeType)
Q_DECLARE_METATYPE(domain::ListScope)
