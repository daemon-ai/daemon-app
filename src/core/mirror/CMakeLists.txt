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
