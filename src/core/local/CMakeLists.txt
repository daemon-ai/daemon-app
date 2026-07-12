# The precious per-identity local-<id>.db module (spec 09 §4.5) + the durable outbox (§6) — A2 of
# the mirror architecture program. Versioned + migrated, never dropped: the durable home of pending
# write-intent (outbox) and client-local sidecar state (§3.4). GUI-free; consumed by the verb seams
# (enqueue), the ingestor (provenance confirm), the pending strip (PendingOpsModel), and the
# composer turn-prompt lane storage swap (§6.7).
#
# Owns ONLY local-<id>.db. The store/journal (mirror-<id>.db) is package A1's; nothing here touches
# it. This is low-level durable infra: it deliberately does NOT depend on any UI/composer module —
# the composer reaches DOWN to it for durability (that adapter lives in the composer module).
add_library(da_local STATIC
    outbox_types.h
    uuidv7.h
    uuidv7.cpp
    verb_class.h
    verb_class.cpp
    local_database.h
    local_database.cpp
    outbox.h
    outbox.cpp
    pending_ops_model.h
    pending_ops_model.cpp
)
add_library(daemon-app::local ALIAS da_local)

# PendingOpsModel / Outbox each declare Q_OBJECT.
set_target_properties(da_local PROPERTIES AUTOMOC ON)

target_include_directories(da_local PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_local
    PUBLIC
        Qt6::Core
    PRIVATE
        Qt6::Sql
        daemon-app::platform # wasm IDBFS write-back seam (no-op off wasm)
        daemon-app::warnings
)
