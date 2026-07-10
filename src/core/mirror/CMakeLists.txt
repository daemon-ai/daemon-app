# Mirror entity layer (spec 09 §3). The generated artifacts under generated/ are produced by the
# superproject's second update-codec emitter from daemon-api.cddl + entity-map.toml and vendored
# here (drift-gated by `just codec-drift`); do NOT hand-edit them. This target exists so the
# generated Q_GADGET entity headers actually compile (moc-processed) and the one-time mapper
# skeleton (entities_map.cpp, human-owned bodies) links. No consumers yet — the store/ingestor/
# projections (packages A1/A3/A4) build on top of this.
add_library(da_mirror STATIC
    generated/entities_gen.h
    generated/entities_provenance_gen.h
    generated/entities_map_gen.h
    entities_map.cpp
)
add_library(daemon-app::mirror ALIAS da_mirror)

# Q_GADGET structs in the generated headers need moc.
set_target_properties(da_mirror PROPERTIES AUTOMOC ON)

target_include_directories(da_mirror PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."          # so `mirror/generated/entities_gen.h` resolves
    "${CMAKE_CURRENT_SOURCE_DIR}/generated"   # so the generated headers' sibling includes resolve
)

target_link_libraries(da_mirror
    PUBLIC
        Qt6::Core
        # the decoded codec structs the mappers consume (daemon_api_client_types.h) + its include dir
        daemon-app::daemon_codec
)

# ---------------------------------------------------------------------------
# Mirror substrate (spec 09 §4, §11; ADR-002/-001/-008) — the store root, the
# ACTIVE degraded journal, mirror-<id>.db persistence, and the swappable observe
# seam (both spike implementations). Built ONLY when the header-only immer +
# zug + lager reactive-core subset is vendored (cmake/Dependencies.cmake sets
# DAEMON_APP_HAVE_MIRROR_SUBSTRATE and creates the immer/zug/lager targets).
# Platform stacks that do not pass the sources simply skip it; the generated
# entity layer (da_mirror, above) still builds everywhere.
# ---------------------------------------------------------------------------
if(DAEMON_APP_HAVE_MIRROR_SUBSTRATE)
    add_library(da_mirror_substrate STATIC
        journal.cpp
        store.cpp
        persistence.cpp
        observe_coarse.cpp
        model.h
        entity_reflect.h
        journal.h
        store.h
        observe.h
        observe_coarse.h
        observe_lager.h
    )
    add_library(daemon-app::mirror_substrate ALIAS da_mirror_substrate)

    # store.h / observe_coarse.h carry Q_OBJECT.
    set_target_properties(da_mirror_substrate PROPERTIES AUTOMOC ON)

    # Embed the generated, drift-gated DDL as a Qt resource (read, never copied):
    # mirror::Persistence applies it verbatim to create mirror-<id>.db.
    qt_add_resources(da_mirror_substrate "da_mirror_schema"
        PREFIX "/mirror"
        BASE "${CMAKE_CURRENT_SOURCE_DIR}/generated"
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/generated/mirror_schema_gen.sql"
    )

    target_include_directories(da_mirror_substrate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

    target_link_libraries(da_mirror_substrate
        PUBLIC
            Qt6::Core
            Qt6::Sql
            daemon-app::mirror   # the generated Q_GADGET entities
            immer::immer
            lager::lager         # reactive-core subset only (observe seam); pulls zug + boost::intrusive
    )

    # -----------------------------------------------------------------------
    # Mirror lens layer (spec 09 §8; package A3) — the generic journal-delta
    # collection adapters (mirror::TableModel / mirror::WindowModel), the 64-bit
    # stable-id interner, and the ConversationListModel role-map demonstrator.
    # GUI-free C++ (QAbstractListModel is Qt6::Core): QML binds these, the TUI
    # consumes them via DisplayRoleAdapter (§8.4). Built on the substrate.
    # -----------------------------------------------------------------------
    add_library(da_mirror_lens STATIC
        table_model.cpp
        conversation_list_model.cpp
        table_model.h
        window_model.h
        stable_id.h
        conversation_list_model.h
    )
    add_library(daemon-app::mirror_lens ALIAS da_mirror_lens)

    # TableModelBase + ConversationListModel carry Q_OBJECT.
    set_target_properties(da_mirror_lens PROPERTIES AUTOMOC ON)

    target_include_directories(da_mirror_lens PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

    target_link_libraries(da_mirror_lens
        PUBLIC
            Qt6::Core
            daemon-app::mirror_substrate
    )
endif()
