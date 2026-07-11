# Vendored NodeApi CBOR codec: the zcbor C runtime plus the generated daemon-api client codec
# (codec/generated), produced and drift-checked by the superproject from the daemon-node contract.
# Built as plain C and kept free of the project's -Werror warning set since it is third-party /
# generated code; see codec/README.md for the regeneration recipe.
enable_language(C)

add_library(da_daemon_codec STATIC
    codec/vendor/zcbor_common.c
    codec/vendor/zcbor_decode.c
    codec/vendor/zcbor_encode.c
    codec/vendor/zcbor_print.c
    codec/generated/daemon_api_client_decode.c
    codec/generated/daemon_api_client_encode.c
)
add_library(daemon-app::daemon_codec ALIAS da_daemon_codec)
set_target_properties(da_daemon_codec PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
# Encode in canonical (definite-length) CBOR. Without this, zcbor emits indefinite-length maps
# (0xBF..0xFF), which the daemon's serde/ciborium decoder rejects for externally-tagged enum
# variants ("invalid type: map, expected enum") - so every data-carrying ApiRequest (CredentialSet,
# ModelCurrent, SessionsQuery, ...) would fail to decode. Canonical maps (0xA1/0xA2) match the
# daemon's own serde encoding byte-for-byte. PUBLIC so the codec headers + callers agree.
target_compile_definitions(da_daemon_codec PUBLIC ZCBOR_CANONICAL)
target_include_directories(da_daemon_codec PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/codec/vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/codec/generated"
)

# Platform-conditional TUs: Qt-for-wasm lacks the `localserver`, `ssl`, and `process` features,
# so the browser build swaps the Unix/TLS stream carriers and the QProcess-based launcher for
# fail-closed stubs (the WebSocket carrier in daemon_transport.cpp is the only wasm transport).
# Android shares the stubs and the WebSocket-only posture: the flake's android Qt builds without
# the ssl feature (no bundled OpenSSL, packaging/android/README.md), and an APK cannot spawn or
# probe a local daemon anyway - it is a thin remote client exactly like the browser. iOS is the
# same case: the flake's iOS Qt is static with no bundled OpenSSL and no `process` feature, and a
# sandboxed .app cannot spawn/probe a local daemon - so it too rides the WebSocket-only stubs.
if(DAEMON_APP_WASM OR ANDROID OR IOS)
    set(DA_DAEMON_PLATFORM_SOURCES
        daemon_transport_stream_wasm.cpp
        local_daemon_launcher_wasm.cpp
    )
else()
    set(DA_DAEMON_PLATFORM_SOURCES
        daemon_transport_stream.cpp
        local_daemon_launcher.cpp
    )
endif()

add_library(da_daemon_client STATIC
    daemon_transport.h
    daemon_transport.cpp
    node_api_client.h
    node_api_client.cpp
    node_api_auth.h
    node_api_auth.cpp
    principal_model.h
    page_loop.h
    node_api_codec.h
    node_api_codec/internal.h
    node_api_codec/mappers.cpp
    node_api_codec/encode_requests.cpp
    node_api_codec/wire_l0.cpp
    node_api_codec/decode_responses.cpp
    node_api_codec/fs_fleet_channels.cpp
    node_api_codec/chat_persons.cpp
    local_daemon_launcher.h
    windows_pipe_name.h
    daemon_connection_service.h
    daemon_connection_service.cpp
    ${DA_DAEMON_PLATFORM_SOURCES}
    daemon_accounts_service.h
    daemon_accounts_service.cpp
    daemon_auth_flow_service.h
    daemon_auth_flow_service.cpp
    daemon_model_catalog.h
    daemon_model_catalog.cpp
    daemon_provider_catalog.h
    daemon_provider_catalog.cpp
    daemon_profile_store.h
    daemon_profile_store.cpp
    daemon_session_settings.h
    daemon_session_settings.cpp
    daemon_checkpoint_timeline.h
    daemon_checkpoint_timeline.cpp
    daemon_approvals_inbox.h
    daemon_approvals_inbox.cpp
    # [wave2:app-engines] shared engine-identity facade (C3/C4/C5)
    engine_identity.h
    engine_identity.cpp
    # [wave2:app-approvals-safety] D2 tool inventory
    daemon_tool_inventory.h
    daemon_tool_inventory.cpp
    subscription_manager.h
    subscription_manager.cpp
    daemon_fs_service.h
    daemon_fs_service.cpp
    daemon_session_roster.h
    daemon_session_roster.cpp
    daemon_dashboard.h
    daemon_dashboard.cpp
    daemon_transport_registry.h
    daemon_transport_registry.cpp
    daemon_contacts_service.h
    daemon_contacts_service.cpp
    daemon_persons_service.h
    daemon_persons_service.cpp
    daemon_chat_service.h
    daemon_chat_service.cpp
    daemon_presence_service.h
    daemon_presence_service.cpp
    daemon_cache_store.h
    daemon_cache_store.cpp
    cached_session_store.h
    cached_session_store.cpp
    app_service_graph.h
    app_service_graph.cpp
    client_cache_schema.h
    repositories.h
    repositories.cpp
    feedback_repository.h
    feedback_repository.cpp
    seam_migration.h
    integration_slice.h
    cache_consolidation_policy.h
    # mirror A5: the decode-to-mirror bridge (CBOR reply -> mirror entities via the G1 mappers).
    # Always built (needs only the generated entities/mappers); the codec stays confined here.
    mirror_decode.h
    mirror_decode.cpp
    ingestor_bridge.h
)
add_library(daemon-app::daemon_client ALIAS da_daemon_client)

set_target_properties(da_daemon_client PROPERTIES AUTOMOC ON)

target_include_directories(da_daemon_client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(da_daemon_client
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Sql
        daemon-app::connection
        daemon-app::persistence
        daemon-app::settings_store
        daemon-app::config
        daemon-app::feedback
        daemon-app::fs
        daemon-app::memory
        daemon-app::models
        daemon-app::accounts
        daemon-app::auth
        daemon-app::profiles
        daemon-app::fleet
        daemon-app::automation
        daemon-app::session
        daemon-app::transports
        daemon-app::tools # [wave2:app-approvals-safety] D2 tool inventory seam
        daemon-app::daemonnet
        daemon-app::nav
        daemon-app::firstrun
        daemon-app::setup
        daemon-app::update
    PRIVATE
        # The WebSocket mux carrier (daemon_transport.cpp); private because no public header
        # exposes QtWebSockets types.
        Qt6::WebSockets
        daemon-app::daemon_codec
        # mirror A5: the generated entities + human-owned mappers (decode-to-mirror bridge). Always
        # available (built everywhere); the codec + mappers are confined to this lib (07§10).
        daemon-app::mirror
        # Browser-persistence + network-wakeup seams (all no-ops off wasm, private - no public
        # daemon_client header exposes platform types):
        #  - DaemonCacheStore calls platform::scheduleIdbfsSync() / installUnloadFlush() (W1/W2).
        #  - daemon_connection_service.cpp calls platform::installOnlineCallback() (W5).
        daemon-app::platform
        daemon-app::warnings
)

# mirror A5: live-wire the ingestor over NodeApiClient (spec 09 §5, wave M2). The DaemonFetchExecutor
# + MirrorService graph wiring + the outbox (local-<id>.db) are built only when the immer substrate
# is vendored (the ingestor/lens/outbox targets exist). A substrate-less stack (bare platform port)
# still builds daemon_client without the live mirror path.
if(DAEMON_APP_HAVE_MIRROR_SUBSTRATE)
    target_sources(da_daemon_client PRIVATE
        daemon_fetch_executor.h
        daemon_fetch_executor.cpp
        # mirror A7 (M4): the mirror-backed ISessionStore the session/fleet consumers rebind onto
        # (6->1 session-shape unification); BOTH modes since A8 (mock projects the seeded mirror).
        mirror_session_store.h
        mirror_session_store.cpp
        # A7T/G2: the ONE msg-fence transcript projection grammar (mirror TranscriptBlock ->
        # content markdown), shared by MirrorSessionStore::content() and the mock seeder's derived
        # legacy baseline so both sides are byte-equal by construction.
        transcript_projection.h
        transcript_projection.cpp
        # A7T (M4 sub-step 6): the app-local transcript stream re-homed onto the mirror window.
        # The engine-facing ITranscriptMirrorSink is mirror-free (always buildable); the concrete
        # adapter needs the ingestor, so it is substrate-gated here.
        transcript_mirror_sink.h
        mirror_transcript_sink.h
        mirror_transcript_sink.cpp
        # mirror A8 (M5): the mock scenario catalog (the ONLY mock seed source) + the host that
        # drives it (Seeder + timeline + scripted verb outcomes + the api/<N> mode select).
        mock_scenario.h
        mock_scenario.cpp
        mock_scenario_host.h
        mock_scenario_host.cpp
        # G2 (M5): the fleet TREE projected from the mirror FleetUnit entity (replaces the legacy
        # DaemonFleetTree). Needs the mirror store/ingestor, so it is substrate-gated here.
        mirror_fleet_tree.h
        mirror_fleet_tree.cpp
    )
    target_link_libraries(da_daemon_client PUBLIC
        daemon-app::mirror_ingestor
        daemon-app::mirror_lens
        daemon-app::local
    )
    target_compile_definitions(da_daemon_client PRIVATE DAEMON_APP_HAVE_MIRROR_SUBSTRATE=1)
endif()
