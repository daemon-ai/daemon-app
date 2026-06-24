// Anchors the da_domain static library and registers the value types with the
// Qt metatype system so they can cross queued connections / be used in QVariant.

#include "domain/unit_node.h"
#include "domain/session.h"
#include "domain/ids.h"
#include "domain/sidebar_node.h"
#include "domain/tag.h"

namespace domain {
namespace {

const int kSessionId = qRegisterMetaType<Session>();
const int kUnitNodeId = qRegisterMetaType<UnitNode>();
const int kTagId = qRegisterMetaType<Tag>();
const int kListScopeId = qRegisterMetaType<ListScope>();
const int kProfileRefId = qRegisterMetaType<ProfileRef>();
const int kUnitIdId = qRegisterMetaType<UnitId>();
const int kSessionIdId = qRegisterMetaType<SessionId>();

} // namespace
} // namespace domain
