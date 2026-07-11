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
# ACTIVE degraded journal, mirror-<id>.db persistence, and the observe seam.
# ADR-008 FINALIZED (A3, M1): default = coarse signals; the lager candidate was
# deleted after gate 1 stayed decisively RED on the representative VM TU set
# (see src/core/mirror/ADR-008-addendum-A3.md). UNCONDITIONAL (AD, ratified):
# the mirror IS the data path; cmake/Dependencies.cmake requires the immer pin
# (every shipped configuration carries it), so there is no substrate-less build.
# ---------------------------------------------------------------------------
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
            immer::immer         # the value substrate; the observe seam is coarse signals (ADR-008)
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
        chat_window_model.cpp
        person_list_model.cpp
        contact_list_model.cpp
        table_model.h
        window_model.h
        stable_id.h
        conversation_list_model.h
        chat_window_model.h
        person_list_model.h
        contact_list_model.h
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

    # -----------------------------------------------------------------------
    # A4 ingestor (spec 09 §5, §4.6, §13 M1): the single event-policy authority.
    # Policy table (16 arms, compile-time exhaustive), fetch scheduler (dedup/
    # priority/cap), cursor+epoch+node-rev lifecycle, reconnect both modes,
    # visibility staleness, ConvHistory cursor fix, chat windows v1, the
    # write-behind consumer, and dual-write parity asserts. Transport-agnostic
    # (FetchExecutor seam) so it is unit-testable with synthetic E4 streams; the
    # daemon bridge (src/core/daemon) wraps NodeApiClient + the vendored codec.
    # -----------------------------------------------------------------------
    add_library(da_mirror_ingestor STATIC
        fetch_scheduler.cpp
        sync_state.cpp
        ingestor.cpp
        write_behind.cpp
        parity.cpp
        mirror_service.cpp
        seeder.cpp
        node_event.h
        policy_table.h
        fetch_job.h
        fetch_scheduler.h
        sync_state.h
        ingestor.h
        write_behind.h
        parity.h
        mirror_service.h
        seeder.h
    )
    add_library(daemon-app::mirror_ingestor ALIAS da_mirror_ingestor)

    # ingestor.h / write_behind.h carry Q_OBJECT.
    set_target_properties(da_mirror_ingestor PROPERTIES AUTOMOC ON)

    target_include_directories(da_mirror_ingestor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

    target_link_libraries(da_mirror_ingestor
        PUBLIC
            Qt6::Core
            daemon-app::mirror_substrate
    )
