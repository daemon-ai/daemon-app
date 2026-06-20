#pragma once

#include <QMetaType>
#include <QString>

namespace domain {

// The kinds of rows the sidebar shows. The numeric order is load-bearing: QML
// branches on these integer values, so keep positions stable when extending.
enum class NodeType {
    AllConversations = 0, // 0: every non-archived conversation
    Archived,             // 1: archived conversations
    FleetSeparator,       // 2: the "Fleet" section header (with + add-root)
    TagSeparator,         // 3: the "Tags" section header (with + add-tag)
    Node,                 // 4: an agent node in the supervision tree
    Tag,                  // 5: a tag label
};

// What the conversations list is currently filtered to (the sidebar selection).
// For `Tag`, `id` is the tag id. For `Node`, `nodeId` is the agent node id and
// the scope folds over that node's WHOLE subtree (the node and all descendants),
// which is the same logic for every node regardless of kind or depth.
struct ListScope {
    NodeType type = NodeType::AllConversations;
    int id = -1;      // tag id for Tag scope; ignored otherwise
    QString nodeId;   // agent node id for Node scope; empty otherwise

    friend bool operator==(const ListScope& a, const ListScope& b)
    {
        return a.type == b.type && a.id == b.id && a.nodeId == b.nodeId;
    }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::NodeType)
Q_DECLARE_METATYPE(domain::ListScope)
