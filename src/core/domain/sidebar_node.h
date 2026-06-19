#pragma once

#include <QMetaType>

namespace domain {

// The kinds of rows the sidebar tree shows.
enum class NodeType {
    AllConversations,
    Archived,
    FolderSeparator,
    TagSeparator,
    Folder,
    Tag,
};

// What the conversations list is currently filtered to (the sidebar selection).
// `id` is the folder/tag id for Folder/Tag scopes, ignored otherwise.
struct ListScope {
    NodeType type = NodeType::AllConversations;
    int id = -1;

    friend bool operator==(const ListScope& a, const ListScope& b)
    {
        return a.type == b.type && a.id == b.id;
    }
};

} // namespace domain

Q_DECLARE_METATYPE(domain::NodeType)
Q_DECLARE_METATYPE(domain::ListScope)
