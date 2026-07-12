#pragma once
// mirror::MirrorModel — the single value-typed store root (spec 09 §4.1, ADR-002).
//
// One immer::table per class-M/L entity kind (keyed by the generated typed key via
// entity_key_fn), one per-scope cursor-ordered Window per class-W kind, the collection
// sync-state mirror (§5.5), and the worked conversationsByTransport relation index (§3.5).
// The struct is a regular value: rebuilt only through the apply pipeline (mirror::Store),
// published as ONE atomic snapshot per batch. immer's structural sharing makes whole-value
// copies and equality cheap (01§3.5).
//
// The element types are G1's generated Q_GADGET structs (mirror/generated/entities_gen.h) —
// NEVER a parallel shape. Value equality + canonical dumps are derived generically from the
// Q_GADGET meta-object (entity_reflect.h) so no per-field code (and no edit to generated/) is
// needed; immer::diff and lager has_changed gating both consume that operator==.

#include "mirror/entity_reflect.h"
#include "mirror/generated/entities_gen.h"

#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/map.hpp>
#include <immer/map_transient.hpp>
#include <immer/table.hpp>
#include <immer/table_transient.hpp>
#include <QString>
#include <type_traits>
#include <utility>

namespace mirror {

// ---------------------------------------------------------------------------
// The census as X-macros — the single place kinds are enumerated for the root
// members, the type->member accessors, and the derived operator== definitions.
// Non-windowed tables cover every class-M and class-L kind (31); the 3 class-W
// kinds get per-scope windows. Order follows the generated EntityKind enum.
// ---------------------------------------------------------------------------
#define MIRROR_TABLE_ENTITIES(X)                                                                   \
    X(AccessUser, access_users)                                                                    \
    X(Adapter, adapters)                                                                           \
    X(AgentEntry, agent_entries)                                                                   \
    X(Approval, approvals)                                                                         \
    X(CapsReport, caps_reports)                                                                    \
    X(Checkpoint, checkpoints)                                                                     \
    X(CommandSpec, command_specs)                                                                  \
    X(Contact, contacts)                                                                           \
    X(Conversation, conversations)                                                                 \
    X(ConversationMember, conversation_members)                                                    \
    X(Credential, credentials)                                                                     \
    X(CronJob, cron_jobs)                                                                          \
    X(CustomProvider, custom_providers)                                                            \
    X(FleetUnit, fleet_units)                                                                      \
    X(GatewayStatus, gateway_statuses)                                                             \
    X(InstalledModel, installed_models)                                                            \
    X(ModelDownload, model_downloads)                                                              \
    X(Notification, notifications)                                                                 \
    X(Person, persons)                                                                             \
    X(PersonEndpoint, person_endpoints)                                                            \
    X(Profile, profiles)                                                                           \
    X(ProviderDescriptor, provider_descriptors)                                                    \
    X(QuantizeJob, quantize_jobs)                                                                  \
    X(RememberedFingerprint, remembered_fingerprints)                                              \
    X(RoleInfo, role_infos)                                                                        \
    X(Room, rooms)                                                                                 \
    X(RoutePin, route_pins)                                                                        \
    X(SavedPresence, saved_presences)                                                              \
    X(Session, sessions)                                                                           \
    X(ToolInfo, tools)                                                                             \
    X(TransportAccount, transport_accounts)

#define MIRROR_WINDOW_ENTITIES(X)                                                                  \
    X(ChatMessage, chat)                                                                           \
    X(TranscriptBlock, transcripts)                                                                \
    X(FsEntry, fs_listings)

// Generic value equality for the generated Q_GADGET entities (see entity_reflect.h). Defined in
// namespace mirror so ADL + std::equal_to<T> (which immer::diff hard-codes) resolve it; keeps
// generated/ untouched. Windowed and non-windowed kinds alike get it.
#define MIRROR_DEFINE_EQ(Type, member)                                                             \
    inline bool operator==(const Type& lhs, const Type& rhs) noexcept {                            \
        return ::mirror::reflect::gadget_equal(lhs, rhs);                                          \
    }                                                                                              \
    inline bool operator!=(const Type& lhs, const Type& rhs) noexcept {                            \
        return !(lhs == rhs);                                                                      \
    }
MIRROR_TABLE_ENTITIES(MIRROR_DEFINE_EQ)
MIRROR_WINDOW_ENTITIES(MIRROR_DEFINE_EQ)
#undef MIRROR_DEFINE_EQ

// immer::table key function: the generated entity exposes a typed key() (M/L) already made
// hashable + comparable in entities_gen.h. Get-only is sufficient — the apply pipeline uses
// insert() (upsert), erase(key) and find(key) exclusively (spec §4.2), never update()/operator[].
struct entity_key_fn {
    template <typename T>
    auto operator()(T&& x) const {
        return std::forward<T>(x).key();
    }
};

template <typename T>
using EntityTable = immer::table<T, entity_key_fn>;

// The scope type a windowed entity is grouped by (ChatMessageScope, ...).
template <typename T>
using ScopeOf = std::decay_t<decltype(std::declval<T>().scope())>;

// Per-scope window bookkeeping (spec §4.6): budget accounting lives here and is persisted into
// window_meta in the same write-behind transaction as the rows.
struct WindowMeta {
    quint64 oldest_cursor = 0;
    quint64 newest_cursor = 0;
    int item_count = 0;
    qint64 byte_count = 0;
    bool contiguous_to_head = false;
};

// A class-W collection window: cursor-ordered items (boxed — fat payloads stay out of the
// map's CHAMP chunks, 01§3.5) + meta.
template <typename T>
struct Window {
    immer::flex_vector<immer::box<T>> items;
    WindowMeta meta;

    friend bool operator==(const Window& lhs, const Window& rhs) noexcept {
        if (lhs.items.size() != rhs.items.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.items.size(); ++i) {
            if (!(lhs.items[i].get() == rhs.items[i].get())) {
                return false;
            }
        }
        return true;
    }
    friend bool operator!=(const Window& lhs, const Window& rhs) noexcept { return !(lhs == rhs); }
};

template <typename T>
using WindowMap = immer::map<ScopeOf<T>, Window<T>>;

// Per-collection sync state (spec §5.5, E5). A read-only projection lives in the root so E5
// lenses render staleness honestly; the ingestor (A4) is the only writer of the authoritative
// copy in sync_cursors/node_revs.
enum class CollectionState : quint8 { Fresh = 0, Stale = 1, Syncing = 2, Error = 3 };
enum class StampingMode : quint8 { RefetchDiff = 0, WireDelta = 1 };

struct CollectionSyncState {
    quint64 node_rev = 0;
    qint64 fetched_at_ms = 0;
    CollectionState state = CollectionState::Stale;
    StampingMode mode = StampingMode::RefetchDiff;
    QString last_error;

    friend bool operator==(const CollectionSyncState& a, const CollectionSyncState& b) noexcept {
        return a.node_rev == b.node_rev && a.fetched_at_ms == b.fetched_at_ms &&
               a.state == b.state && a.mode == b.mode && a.last_error == b.last_error;
    }
    friend bool operator!=(const CollectionSyncState& a, const CollectionSyncState& b) noexcept {
        return !(a == b);
    }
};

// ---------------------------------------------------------------------------
// The root value.
// ---------------------------------------------------------------------------
struct MirrorModel {
#define X(Type, member) EntityTable<Type> member;
    MIRROR_TABLE_ENTITIES(X)
#undef X
#define X(Type, member) WindowMap<Type> member;
    MIRROR_WINDOW_ENTITIES(X)
#undef X

    // Sync-metadata mirror (§5.5). Keyed by collection name.
    immer::map<QString, CollectionSyncState> sync;

    // Worked typed-relation secondary index (§3.5): conversation keys grouped by transport,
    // maintained ONLY inside the apply pipeline. The remaining §3.5 indexes
    // (childrenByParent, membersByConv, endpointsByPerson, sessionsByProfile, unitsByParent)
    // are A3/A4 integration points — added here as they gain consumers, same discipline.
    immer::map<QString, immer::flex_vector<ConversationKey>> conversationsByTransport;
};

// Typed member accessors for the apply pipeline (tag dispatch keeps the template code that
// touches the root generic across kinds). One overload per census row, mechanically expanded.
template <typename T>
struct TypeTag {};

#define X(Type, member)                                                                            \
    inline EntityTable<Type>& table_for(MirrorModel& m, TypeTag<Type>) {                           \
        return m.member;                                                                           \
    }                                                                                              \
    inline const EntityTable<Type>& table_for(const MirrorModel& m, TypeTag<Type>) {               \
        return m.member;                                                                           \
    }
MIRROR_TABLE_ENTITIES(X)
#undef X

#define X(Type, member)                                                                            \
    inline WindowMap<Type>& window_for(MirrorModel& m, TypeTag<Type>) {                            \
        return m.member;                                                                           \
    }                                                                                              \
    inline const WindowMap<Type>& window_for(const MirrorModel& m, TypeTag<Type>) {                \
        return m.member;                                                                           \
    }
MIRROR_WINDOW_ENTITIES(X)
#undef X

// Whole-root equality (used by the lager observe seam's has_changed gate, and handy in tests).
// Cheap under immer: unchanged tables compare by shared-node identity (01§3.5).
inline bool operator==(const MirrorModel& a, const MirrorModel& b) noexcept {
    bool eq = true;
#define X(Type, member) eq = eq && (a.member == b.member);
    MIRROR_TABLE_ENTITIES(X)
    MIRROR_WINDOW_ENTITIES(X)
#undef X
    return eq && a.sync == b.sync && a.conversationsByTransport == b.conversationsByTransport;
}
inline bool operator!=(const MirrorModel& a, const MirrorModel& b) noexcept {
    return !(a == b);
}

} // namespace mirror
