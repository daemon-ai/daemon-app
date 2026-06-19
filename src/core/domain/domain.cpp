// Anchors the da_domain static library and registers the value types with the
// Qt metatype system so they can cross queued connections / be used in QVariant.

#include "domain/conversation.h"
#include "domain/folder.h"
#include "domain/sidebar_node.h"
#include "domain/tag.h"

namespace domain {
namespace {

const int kConversationId = qRegisterMetaType<Conversation>();
const int kFolderId = qRegisterMetaType<Folder>();
const int kTagId = qRegisterMetaType<Tag>();
const int kListScopeId = qRegisterMetaType<ListScope>();

} // namespace
} // namespace domain
