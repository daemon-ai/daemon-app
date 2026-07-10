find_package(Qt6 6.11 REQUIRED COMPONENTS Test)

# tst_fonts: validates the bundled font assets (glyph coverage, family names) so
# an icon-font regression is caught in CI rather than on screen. It reads the
# files straight from the source tree (the same ones the Theme module embeds).
qt_add_executable(tst_fonts tst_fonts.cpp)
target_link_libraries(tst_fonts PRIVATE Qt6::Gui Qt6::Test)
target_compile_definitions(tst_fonts
    PRIVATE DAEMON_APP_FONTS_DIR="${CMAKE_SOURCE_DIR}/assets/fonts"
)

add_test(NAME tst_fonts COMMAND tst_fonts)
# QRawFont needs a platform integration; run headless in CI.
set_tests_properties(tst_fonts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_localization: the shared runtime translation layer (src/core/i18n) - the
# offered locale list, code->name resolution, that installing an embedded real
# catalog (German) actually translates marked C++ and QML strings, that switching
# back to the source language uninstalls it, and that Arabic reports RTL. The
# German catalog is compiled (lrelease) and embedded into the test under ":/i18n"
# so applyLocale("de") can load it; custom lupdate/lrelease target names avoid
# clashing with the app-wide update_translations/release_translations (so the
# app-wide refresh never rewrites the German catalog from the test's own sources).
qt_add_executable(tst_localization tst_localization.cpp)
target_link_libraries(tst_localization
    PRIVATE
        Qt6::Test
        daemon-app::i18n
)
qt_add_translations(tst_localization
    TS_FILES "${CMAKE_SOURCE_DIR}/i18n/daemon-app_de.ts"
    RESOURCE_PREFIX "/i18n"
    NO_GENERATE_PLURALS_TS_FILE
    LUPDATE_TARGET tst_localization_lupdate
    LRELEASE_TARGET tst_localization_lrelease
)
add_test(NAME tst_localization COMMAND tst_localization)
set_tests_properties(tst_localization PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_settings_store: the shared client-local preference store (Phase 0) - typed
# get/set defaults, setupComplete + last-connection convenience accessors, change
# signalling, and write de-duplication.
qt_add_executable(tst_settings_store tst_settings_store.cpp)
target_link_libraries(tst_settings_store
    PRIVATE
        Qt6::Test
        daemon-app::settings_store
)
add_test(NAME tst_settings_store COMMAND tst_settings_store)
set_tests_properties(tst_settings_store PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_ws_default: the browser connection-target derivation (settings/ws_default) - `?ws=` query
# override precedence, ws-shape gating of the saved target, and the single-origin
# `ws(s)://<host[:port]>/ws` page-origin default. Pure logic, so it runs on desktop even though
# only the wasm build calls it.
qt_add_executable(tst_ws_default tst_ws_default.cpp)
target_link_libraries(tst_ws_default
    PRIVATE
        Qt6::Test
        daemon-app::settings_store
)
add_test(NAME tst_ws_default COMMAND tst_ws_default)
set_tests_properties(tst_ws_default PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_update_manager: the release-feed scaffold (src/core/update) - the package-time
# capability dial defaults to None, the manifest capability vocabulary round-trips
# (unknown input fails closed to None), and the descoped check/download/apply stubs
# emit not-implemented errors instead of doing anything.
qt_add_executable(tst_update_manager tst_update_manager.cpp)
target_link_libraries(tst_update_manager
    PRIVATE
        Qt6::Test
        daemon-app::update
)
add_test(NAME tst_update_manager COMMAND tst_update_manager)
set_tests_properties(tst_update_manager PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_minisign_verifier: the self-contained minisign (Ed25519, prehashed) feed
# verifier against real fixtures - valid signature, wrong key, tampered manifest,
# tampered trusted comment, truncated signature, and legacy-algorithm rejection.
qt_add_executable(tst_minisign_verifier tst_minisign_verifier.cpp)
target_link_libraries(tst_minisign_verifier
    PRIVATE
        Qt6::Test
        daemon-app::update
)
add_test(NAME tst_minisign_verifier COMMAND tst_minisign_verifier)
set_tests_properties(tst_minisign_verifier PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_update_manifest: the SemVer monotonicity comparator (downgrade-replay guard,
# prerelease + build-metadata handling) and the schema-1 manifest parse + artifact
# selection (kind/os/arch, arch=unknown skip, glibcFloor gate).
qt_add_executable(tst_update_manifest tst_update_manifest.cpp)
target_link_libraries(tst_update_manifest
    PRIVATE
        Qt6::Test
        daemon-app::update
)
add_test(NAME tst_update_manifest COMMAND tst_update_manifest)
set_tests_properties(tst_update_manifest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_self_apply_appimage: the AppImage self-apply staging/precondition gate
# (src/core/update/self_apply_appimage) - runningAsAppImage detection, the
# fail-closed degrade reasons (no $APPIMAGE / missing download / missing helper /
# un-writable target dir), the happy-path staging (helper copied out of the mount,
# new image staged same-fs + executable), and the leftover-staging-dir cleanup.
qt_add_executable(tst_self_apply_appimage tst_self_apply_appimage.cpp)
target_link_libraries(tst_self_apply_appimage
    PRIVATE
        Qt6::Test
        daemon-app::update
)
add_test(NAME tst_self_apply_appimage COMMAND tst_self_apply_appimage)
set_tests_properties(tst_self_apply_appimage PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_update_self_apply_windows: the Windows NSIS SelfApply arg construction +
# preconditions (src/core/update/self_apply_windows). Pure logic (testable on
# Linux): the exact frozen daemon-updater CLI contract and the fail-closed
# preflight (missing helper / staged installer / checksum / relaunch target).
qt_add_executable(tst_update_self_apply_windows tst_update_self_apply_windows.cpp)
target_link_libraries(tst_update_self_apply_windows
    PRIVATE
        Qt6::Test
        daemon-app::update
)
add_test(NAME tst_update_self_apply_windows COMMAND tst_update_self_apply_windows)
set_tests_properties(tst_update_self_apply_windows PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_self_apply_macos: the macOS DMG SelfApply guard decision (translocation,
# read-only volume, unwritable parent, cross-filesystem) + path predicates. Pure
# logic, so it runs on the Linux CI host even though the swap itself is darwin-only.
qt_add_executable(tst_self_apply_macos tst_self_apply_macos.cpp)
target_link_libraries(tst_self_apply_macos
    PRIVATE
        Qt6::Test
        daemon-app::update
)
add_test(NAME tst_self_apply_macos COMMAND tst_self_apply_macos)
set_tests_properties(tst_self_apply_macos PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_connection_service: the mock connection seam's liveness state machine
# (checking -> connecting -> ready / offline), the Test-connection probe, and
# disconnect, all driven on QTimers.
qt_add_executable(tst_connection_service tst_connection_service.cpp)
target_link_libraries(tst_connection_service
    PRIVATE
        Qt6::Test
        daemon-app::connection
)
add_test(NAME tst_connection_service COMMAND tst_connection_service)
set_tests_properties(tst_connection_service PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_approval_wire: the "allow permanently" plumbing (wire v28) on BOTH approval surfaces -
# encodeRespondApprovalRequest (inline) and encodeApprovalDecideRequest (durable inbox) ride the
# optional allow_permanent only for an allow that chose it, decoded back with the vendored codec's
# public request decoder to assert the on-wire fields.
qt_add_executable(tst_approval_wire tst_approval_wire.cpp)
target_link_libraries(tst_approval_wire
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
        daemon-app::daemon_codec
)
add_test(NAME tst_approval_wire COMMAND tst_approval_wire)
set_tests_properties(tst_approval_wire PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_approvals_inbox: the durable inbox "allow permanently" path (wire v28) - decodeApprovals
# surfaces ApprovalInfo.fingerprint as the offer signal (DecodedApprovalInfo::hasFingerprint),
# DaemonApprovalsInbox projects it onto each row's canAllowPermanent, and approve(id, true) forwards
# allow_permanent on ApprovalDecide only for a fingerprinted (offerable) row.
qt_add_executable(tst_daemon_approvals_inbox tst_daemon_approvals_inbox.cpp)
target_link_libraries(tst_daemon_approvals_inbox
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::fleet
        daemon-app::uimodels
)
add_test(NAME tst_daemon_approvals_inbox COMMAND tst_daemon_approvals_inbox)
set_tests_properties(tst_daemon_approvals_inbox PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# [wave2:app-approvals-safety] D2: tool inventory (ToolList -> Tools, wire v29) - decodeTools
# round-trip + the mock facade's display-row contract (enabled/requires/requirementIsCredential).
qt_add_executable(tst_tool_inventory tst_tool_inventory.cpp)
target_link_libraries(tst_tool_inventory
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::tools
        daemon-app::uimodels
)
add_test(NAME tst_tool_inventory COMMAND tst_tool_inventory)
set_tests_properties(tst_tool_inventory PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# [wave2:app-approvals-safety] D4: remembered-fingerprint management (FingerprintList/Revoke ->
# Fingerprints, wire v29) - request encode round-trips + decodeFingerprints response round-trip.
qt_add_executable(tst_fingerprint_manage tst_fingerprint_manage.cpp)
target_link_libraries(tst_fingerprint_manage
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
        daemon-app::daemon_codec
)
add_test(NAME tst_fingerprint_manage COMMAND tst_fingerprint_manage)
set_tests_properties(tst_fingerprint_manage PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_checkpoint_repository: durable checkpoints (E4/TOOL-9) - CheckpointList page loop +
# CheckpointRewind Ok/Error paths over the WireMuxServer fixture, plus the
# DaemonCheckpointTimeline projection (labels, tokens=-1, newest=current, no manual create).
qt_add_executable(tst_checkpoint_repository tst_checkpoint_repository.cpp)
target_link_libraries(tst_checkpoint_repository
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::session
        daemon-app::uimodels
)
add_test(NAME tst_checkpoint_repository COMMAND tst_checkpoint_repository)
set_tests_properties(tst_checkpoint_repository PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_routing_repository: the real routing manager backend (B6/ROU, wire v28) -
# RoutingListChats/BindChat/UnbindChat + TransportRooms over the WireMuxServer fixture, plus the
# DaemonDaemonNet IDaemonNet projection (pins, pin-table resolve, QML chip helpers).
qt_add_executable(tst_routing_repository tst_routing_repository.cpp)
target_link_libraries(tst_routing_repository
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::daemonnet
        daemon-app::uimodels
)
add_test(NAME tst_routing_repository COMMAND tst_routing_repository)
set_tests_properties(tst_routing_repository PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_gateway_repository: Phase F node OpenAI-compatible gateway - GatewayRepository GatewayGet/Set
# round-trips (status decode, Unsupported => supported=false, set error => operationFailed) over the
# WireMuxServer fixture, plus the DaemonConnectionService single health cache (retained
# DecodedHealth + healthChanged + per-service accessor).
qt_add_executable(tst_gateway_repository tst_gateway_repository.cpp)
target_link_libraries(tst_gateway_repository
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
)
add_test(NAME tst_gateway_repository COMMAND tst_gateway_repository)
set_tests_properties(tst_gateway_repository PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_integration: guards the daemon thin-client scaffolding - Unix-socket
# frame parsing, durable cache schema expectations, and first-slice ordering.
qt_add_executable(tst_daemon_integration tst_daemon_integration.cpp)
target_link_libraries(tst_daemon_integration
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
)
add_test(NAME tst_daemon_integration COMMAND tst_daemon_integration)
set_tests_properties(tst_daemon_integration PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_connection_resilience: the daemon-mode connection-resilience phase over a controllable
# QLocalServer fixture - the per-request timeout unsticks a hung daemon, the connection service
# self-corrects out of stale "ready" after a drop and auto-reconnects, a turn resumes from its cursor
# across a mid-stream drop, and the launcher restart budget caps re-spawns.
qt_add_executable(tst_connection_resilience tst_connection_resilience.cpp)
target_link_libraries(tst_connection_resilience
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::turn
)
add_test(NAME tst_connection_resilience COMMAND tst_connection_resilience)
set_tests_properties(tst_connection_resilience PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_wire_mux: the daemon-app multiplexed transport (wire L0) - the Hello handshake, one-shot
# Call/Reply correlation, push Open/Item/End streaming, and Cancel, driven against the shared
# WireMuxServer fixture over a real DaemonTransport + NodeApiClient.
qt_add_executable(tst_wire_mux tst_wire_mux.cpp)
target_link_libraries(tst_wire_mux
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
)
add_test(NAME tst_wire_mux COMMAND tst_wire_mux)
set_tests_properties(tst_wire_mux PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_principal_model: the WhoAmI / principal model that drives advisory capability gating -
# fail-closed defaults, admin vs viewer capability grants (a Viewer sees no admin page), and clear.
qt_add_executable(tst_principal_model tst_principal_model.cpp)
target_link_libraries(tst_principal_model
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
)
add_test(NAME tst_principal_model COMMAND tst_principal_model)
set_tests_properties(tst_principal_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_node_api_auth: the client SASL layer - SCRAM-SHA-256 against the RFC 7677 worked example
# (client-first/final proof + server-final v= verification), PLAIN/EXTERNAL single-step shapes, and
# the mechanism selection policy. Pure (no socket): verifiable without the server (Auth 3).
qt_add_executable(tst_node_api_auth tst_node_api_auth.cpp)
target_link_libraries(tst_node_api_auth
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
)
add_test(NAME tst_node_api_auth COMMAND tst_node_api_auth)
set_tests_properties(tst_node_api_auth PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_auth_flow (app-wizard-auth stream): the interactive-auth stack — codec round-trips for the
# AuthProviders/AuthBegin/AuthComplete/AuthCancel + AcpDiscover + ModelQuantize(s)/ModelInspect
# op families against the vendored zcbor codec, and the shared AuthFlowController state machine
# (begin -> awaiting_browser -> complete; failures; cancel; TTL) over the mock seam. useSink=false
# throughout: offscreen tests never bind ports.
qt_add_executable(tst_auth_flow tst_auth_flow.cpp)
target_link_libraries(tst_auth_flow
    PRIVATE
        Qt6::Test
        daemon-app::auth
        daemon-app::daemon_client
        daemon-app::daemon_codec
)
add_test(NAME tst_auth_flow COMMAND tst_auth_flow)
set_tests_properties(tst_auth_flow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# [wave2:app-engines] tst_agent_registry (C1/ENG-3): the foreign-agent registry codec —
# AgentRegister/AgentRemove/AgentCatalog/AgentDiscover round-trips against the vendored zcbor codec
# (source=Manual, protocol tag, stdio/endpoint recipe; installed absent), and AgentCatalog decode
# reading the v29 protocol field.
qt_add_executable(tst_agent_registry tst_agent_registry.cpp)
target_link_libraries(tst_agent_registry
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
        daemon-app::daemon_codec
)
add_test(NAME tst_agent_registry COMMAND tst_agent_registry)
set_tests_properties(tst_agent_registry PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# [integration:foreign-agents] tst_foreign_agents_codec (Phase B): the regenerated wire shapes the
# foreign-agent surfacing gates on — ProfileSpec.foreign_backend (encode + decode), AgentEntry
# .verification, SessionDetail.model_selector, gateway ops (GatewayGet/Set + GatewayStatus),
# ProfileInfo provenance (created_by/owner), CapsReport ceilings, and the ProfilesChanged event.
qt_add_executable(tst_foreign_agents_codec tst_foreign_agents_codec.cpp)
target_link_libraries(tst_foreign_agents_codec
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
        daemon-app::daemon_codec
)
add_test(NAME tst_foreign_agents_codec COMMAND tst_foreign_agents_codec)
set_tests_properties(tst_foreign_agents_codec PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# [wave2:app-engines] tst_engine_identity (C3/C4/C5): the shared EngineIdentity facade —
# session/profile -> engine {engine, agent, protocol, label}; labelFor presentation mapping.
qt_add_executable(tst_engine_identity tst_engine_identity.cpp)
target_link_libraries(tst_engine_identity
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
        daemon-app::uimodels
)
add_test(NAME tst_engine_identity COMMAND tst_engine_identity)
set_tests_properties(tst_engine_identity PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_tls_transport: the TLS/TCP transport - fail-closed cert verification (an untrusted self-signed
# cert aborts the handshake), and the explicitly-configured trust paths (SHA-256 pin / insecure
# skip) connecting + round-tripping an encrypted frame against a QSslServer.
qt_add_executable(tst_tls_transport tst_tls_transport.cpp)
target_link_libraries(tst_tls_transport
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
)
add_test(NAME tst_tls_transport COMMAND tst_tls_transport)
set_tests_properties(tst_tls_transport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_ws_transport: the WebSocket transport (the browser/wasm carrier, tested natively) - the
# daemon-mux subprotocol handshake, one-binary-message-per-frame Call/stream round-trips, SCRAM
# over WS, WS-vs-Unix behavioral parity over the shared MuxServerCore fixture, and the connection
# service's remote-ws mode (URL validation, connect-to-ready, per-target token persistence).
qt_add_executable(tst_ws_transport tst_ws_transport.cpp)
target_link_libraries(tst_ws_transport
    PRIVATE
        Qt6::Test
        Qt6::Network
        Qt6::WebSockets
        daemon-app::daemon_client
)
add_test(NAME tst_ws_transport COMMAND tst_ws_transport)
set_tests_properties(tst_ws_transport PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_sync_resync: the L2 client resync - the DaemonTurnEngine on a push Subscribe stream tracks the
# session-activation epoch and re-baselines from the durable journal on a generation change (epoch
# bump / Reset), and suspends+reopens on a transport drop, driven against the WireMuxServer fixture.
qt_add_executable(tst_sync_resync tst_sync_resync.cpp)
target_link_libraries(tst_sync_resync
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::turn
)
add_test(NAME tst_sync_resync COMMAND tst_sync_resync)
set_tests_properties(tst_sync_resync PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_subscription_manager: the L3 node-wide EventsSince feed consumer - the SubscriptionManager
# routes each NodeEvent (approval -> inbox refetch, roster -> debounced roster refetch,
# session-advanced -> focused-engine nudge, resync -> baseline) over the WireMuxServer fixture, plus
# the durable render-from-cache transcript (schema v2 blocks -> content() markdown, refocus replay).
qt_add_executable(tst_subscription_manager tst_subscription_manager.cpp)
target_link_libraries(tst_subscription_manager
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::turn
)
add_test(NAME tst_subscription_manager COMMAND tst_subscription_manager)
set_tests_properties(tst_subscription_manager PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_fleet_tree: the Phase 5b daemon-backed fleet seam - offline render from cache, a Tree
# query populating cache+model, and a PRO-10 control rejection (Unsupported) reverting the optimistic
# pause overlay, all over the WireMuxServer fixture.
qt_add_executable(tst_daemon_fleet_tree tst_daemon_fleet_tree.cpp)
target_link_libraries(tst_daemon_fleet_tree
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::fleet
        daemon-app::uimodels
)
add_test(NAME tst_daemon_fleet_tree COMMAND tst_daemon_fleet_tree)
set_tests_properties(tst_daemon_fleet_tree PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_roster_dashboard: the Phase 5 daemon-backed roster + dashboard seams - offline-first
# roster projected from the CachedSessionStore, the client-local suspend overlay + close->archive,
# and the dashboard counters derived from roster/fleet/approvals/connection.
qt_add_executable(tst_daemon_roster_dashboard tst_daemon_roster_dashboard.cpp)
target_link_libraries(tst_daemon_roster_dashboard
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::fleet
        daemon-app::uimodels
)
add_test(NAME tst_daemon_roster_dashboard COMMAND tst_daemon_roster_dashboard)
set_tests_properties(tst_daemon_roster_dashboard PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_fs_service: the Phase 4 daemon-backed filesystem seam - DaemonFsService over the fs_*
# codec facade (roots/list/read/write round-trip + the bstr content fix) and the lossless watch path
# (a reset page forces changed()), driven against the WireMuxServer fixture.
qt_add_executable(tst_daemon_fs_service tst_daemon_fs_service.cpp)
target_link_libraries(tst_daemon_fs_service
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::fs
)
add_test(NAME tst_daemon_fs_service COMMAND tst_daemon_fs_service)
set_tests_properties(tst_daemon_fs_service PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_file_finder_model: the Ctrl+P index walk - host browse roots are never crawled (the
# whole-home FsList storm guard) and a failed directory listing frees its in-flight slot instead
# of wedging the walk.
qt_add_executable(tst_file_finder_model tst_file_finder_model.cpp)
target_link_libraries(tst_file_finder_model
    PRIVATE
        Qt6::Test
        daemon-app::files
        daemon-app::fs
)
add_test(NAME tst_file_finder_model COMMAND tst_file_finder_model)
set_tests_properties(tst_file_finder_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_transports: the Phase 6a Channels read surface - the transport codec facade round-trips
# (Adapters/TransportInstances/Conversations via cbor_encode_api_response), and DaemonTransportRegistry
# + DaemonPresenceService render offline-first from cache and populate over the WireMuxServer fixture.
qt_add_executable(tst_daemon_transports tst_daemon_transports.cpp)
target_link_libraries(tst_daemon_transports
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::transports
)
add_test(NAME tst_daemon_transports COMMAND tst_daemon_transports)
set_tests_properties(tst_daemon_transports PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_integrations_seams (A1 app/core-seams; wire v38): the integrations foundation the later app
# packages consume — the v38 codec wrappers (enriched auth-param-field, AuthFlowKind::UserPassword,
# ConversationInfo.parent + Space, AdapterInfo.account_schema, ConvSend/ConvHistory, PersonList,
# TransportSettings/Configure, MessagesChanged/PersonsChanged), the ChatRepository / PersonsRepository
# + transport-settings repository surface, the daemon + mock IChatService/IPersonsService seams, and
# the SubscriptionManager fan-out — over the vendored zcbor codec + the WireMuxServer fixture.
qt_add_executable(tst_integrations_seams tst_integrations_seams.cpp)
target_link_libraries(tst_integrations_seams
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::auth
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::transports
)
add_test(NAME tst_integrations_seams COMMAND tst_integrations_seams)
set_tests_properties(tst_integrations_seams PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_profile_store: client-side id-derivation (slug) for ProfileCreate/Clone; the PRO-7/8
# distribution + revisions codec round-trips; and the DaemonProfileStore export->file->import +
# history/revert flows over the WireMuxServer fixture. End-to-end daemon acceptance: system-tests.
qt_add_executable(tst_daemon_profile_store tst_daemon_profile_store.cpp)
target_link_libraries(tst_daemon_profile_store
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::uimodels
)
add_test(NAME tst_daemon_profile_store COMMAND tst_daemon_profile_store)
set_tests_properties(tst_daemon_profile_store PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_app_service_graph: shared GUI/TUI service construction; mock mode must
# preserve the existing graph, daemon mode only swaps the connection seam for now.
qt_add_executable(tst_app_service_graph tst_app_service_graph.cpp)
target_link_libraries(tst_app_service_graph
    PRIVATE
        Qt6::Test
        daemon-app::daemon_client
)
add_test(NAME tst_app_service_graph COMMAND tst_app_service_graph)
set_tests_properties(tst_app_service_graph PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemonnet: the P2 IDaemonNet typed read seam - unit tree accessors, the
# All/Archived/Unit-subtree/Tag scope folds, the ByTransport/ByPeer lens grouping,
# and sessionDetail/content by SessionId, all over the one mock DaemonNet seed.
qt_add_executable(tst_daemonnet tst_daemonnet.cpp)
target_link_libraries(tst_daemonnet
    PRIVATE
        Qt6::Test
        daemon-app::daemonnet
        daemon-app::domain
)
add_test(NAME tst_daemonnet COMMAND tst_daemonnet)
set_tests_properties(tst_daemonnet PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_routing_model: the routing-manager model on MockDaemonNet - resolve() precedence
# (pin > rule > account-bound > default), the pin/handover/bindAccount mutations, and the
# routes/delivery/accounts projections.
qt_add_executable(tst_routing_model tst_routing_model.cpp)
target_link_libraries(tst_routing_model
    PRIVATE
        Qt6::Test
        daemon-app::daemonnet
        daemon-app::domain
)
add_test(NAME tst_routing_model COMMAND tst_routing_model)
set_tests_properties(tst_routing_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_graph_model: the reusable Daemon Kit graph model - generic node/edge ingestion, degree, the
# normalized force layout, and edges()/nodeById/neighborsOf.
qt_add_executable(tst_graph_model tst_graph_model.cpp)
target_include_directories(tst_graph_model PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Graph")
target_link_libraries(tst_graph_model
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::graph
)
add_test(NAME tst_graph_model COMMAND tst_graph_model)
set_tests_properties(tst_graph_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_fs_service: the filesystem seam's local mock must preserve the daemon FS
# contract around optimistic concurrency and path containment.
qt_add_executable(tst_fs_service tst_fs_service.cpp)
target_link_libraries(tst_fs_service
    PRIVATE
        Qt6::Test
        daemon-app::fs
)
add_test(NAME tst_fs_service COMMAND tst_fs_service)
set_tests_properties(tst_fs_service PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_first_run_model: the shared onboarding gate (Phase 1) - fresh install
# starts at connect, a good connection advances to inference, completeInference
# finishes + persists setupComplete, and a returning user boots straight to done.
qt_add_executable(tst_first_run_model tst_first_run_model.cpp)
target_link_libraries(tst_first_run_model
    PRIVATE
        Qt6::Test
        daemon-app::firstrun
        daemon-app::connection
        daemon-app::settings_store
        daemon-app::models
        daemon-app::profiles
        daemon-app::accounts
)
add_test(NAME tst_first_run_model COMMAND tst_first_run_model)
set_tests_properties(tst_first_run_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_agent_setup_model (Phase C): the shared engine+backend+inference setup pipeline - the derived
# predicates (needsInference/needsBackendChoice/needsKey/gatewayRequired/commitReady across Core,
# Foreign+AgentNative, Foreign+NodeProvider), the engineChoices projection with node-derived
# verification badges, and the commit payload projection (a NodeProvider commit persists the right
# foreign_backend).
qt_add_executable(tst_agent_setup_model tst_agent_setup_model.cpp)
target_link_libraries(tst_agent_setup_model
    PRIVATE
        Qt6::Test
        daemon-app::setup
        daemon-app::models
        daemon-app::profiles
)
add_test(NAME tst_agent_setup_model COMMAND tst_agent_setup_model)
set_tests_properties(tst_agent_setup_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_provider_catalog: the node-driven provider/model discovery seams - the codec facade
# round-trips ProviderCatalog / ProviderModels (id-string provider dimension + wire_selector +
# default_base_url + model display_name), the shared local-compose helper + MockProviderCatalog
# shape (Mock excluded), and DaemonProviderCatalog over the WireMuxServer fixture.
qt_add_executable(tst_provider_catalog tst_provider_catalog.cpp)
target_link_libraries(tst_provider_catalog
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::models
        daemon-app::uimodels
)
add_test(NAME tst_provider_catalog COMMAND tst_provider_catalog)
set_tests_properties(tst_provider_catalog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_model_catalog: the daemon-backed model-catalog facade - uint64 download-id encoding
# (no truncation through the generated codec), search paging accumulation (has_more/searchMore),
# the downloadStarted/downloadFinished signal bridge incl. installed-set growth detection,
# operationFailed -> lastError surfacing, and the real-data Providers projection.
qt_add_executable(tst_daemon_model_catalog tst_daemon_model_catalog.cpp)
target_link_libraries(tst_daemon_model_catalog
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
        daemon-app::daemon_codec
        daemon-app::models
        daemon-app::uimodels
)
add_test(NAME tst_daemon_model_catalog COMMAND tst_daemon_model_catalog)
set_tests_properties(tst_daemon_model_catalog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_local_daemon_launcher: the managed local-daemon launcher (CON-1b) - binary discovery
# precedence (explicit override resolves/rejects) and the liveness probe against a real QLocalServer
# vs an absent path. Spawn + poll are covered end-to-end by the system-tests suite.
qt_add_executable(tst_local_daemon_launcher tst_local_daemon_launcher.cpp)
target_link_libraries(tst_local_daemon_launcher
    PRIVATE
        Qt6::Test
        Qt6::Network
        daemon-app::daemon_client
)
add_test(NAME tst_local_daemon_launcher COMMAND tst_local_daemon_launcher)
set_tests_properties(tst_local_daemon_launcher PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_daemon_config: the Phase 2 daemon-config facade mock - seeded defaults,
# enumerated options, typed round-trip, and change signalling/dedup.
qt_add_executable(tst_daemon_config tst_daemon_config.cpp)
target_link_libraries(tst_daemon_config
    PRIVATE
        Qt6::Test
        daemon-app::config
)
add_test(NAME tst_daemon_config COMMAND tst_daemon_config)
set_tests_properties(tst_daemon_config PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_model_catalog: the Phase 3 Models hub mock - search/size filter, a download
# that ticks to completion into Installed, activate (current), and remove.
qt_add_executable(tst_model_catalog tst_model_catalog.cpp)
target_link_libraries(tst_model_catalog
    PRIVATE
        Qt6::Test
        daemon-app::models
        daemon-app::uimodels
)
add_test(NAME tst_model_catalog COMMAND tst_model_catalog)
set_tests_properties(tst_model_catalog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_accounts_service: the Phase 4 accounts mock - seeded accounts, masked
# API-key add, the simulated OAuth round-trip (pending -> connected), and removal.
qt_add_executable(tst_accounts_service tst_accounts_service.cpp)
target_link_libraries(tst_accounts_service
    PRIVATE
        Qt6::Test
        daemon-app::accounts
        daemon-app::uimodels
)
add_test(NAME tst_accounts_service COMMAND tst_accounts_service)
set_tests_properties(tst_accounts_service PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_profile_store: the Phase 5 profiles mock - seeded profiles + catalogs,
# create, field/skill update, exclusive default, and remove default-reassignment.
qt_add_executable(tst_profile_store tst_profile_store.cpp)
target_link_libraries(tst_profile_store
    PRIVATE
        Qt6::Test
        daemon-app::profiles
        daemon-app::uimodels
)
add_test(NAME tst_profile_store COMMAND tst_profile_store)
set_tests_properties(tst_profile_store PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_fleet_ops: the Phase 6 ops mocks - roster state transitions, fleet
# pause/resume, approvals resolve, and the dashboard's derived live counters.
qt_add_executable(tst_fleet_ops tst_fleet_ops.cpp)
target_link_libraries(tst_fleet_ops
    PRIVATE
        Qt6::Test
        daemon-app::fleet
        daemon-app::uimodels
)
add_test(NAME tst_fleet_ops COMMAND tst_fleet_ops)
set_tests_properties(tst_fleet_ops PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_automation: the Phase 7 routing + cron mocks - routing target/enable edits
# and add/remove; cron create/update/runNow/enable/remove.
qt_add_executable(tst_automation tst_automation.cpp)
target_link_libraries(tst_automation
    PRIVATE
        Qt6::Test
        daemon-app::automation
        daemon-app::uimodels
)
add_test(NAME tst_automation COMMAND tst_automation)
set_tests_properties(tst_automation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_session: the Phase 8 composer seams - per-session overrides (dedup'd
# change signal) and the checkpoint timeline (restore truncates + sets current).
qt_add_executable(tst_session tst_session.cpp)
target_link_libraries(tst_session
    PRIVATE
        Qt6::Test
        daemon-app::session
        daemon-app::uimodels
)
add_test(NAME tst_session COMMAND tst_session)
set_tests_properties(tst_session PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_theme_palette: locks the shared theme token table (da_theme_core) - the
# single source of truth backing both Theme.qml and the TUI's tui_palette. The
# canonical per-theme hexes mirror tests/qml/theme/tst_theme.qml.
qt_add_executable(tst_theme_palette tst_theme_palette.cpp)
target_link_libraries(tst_theme_palette
    PRIVATE
        Qt6::Test
        daemon-app::theme_core
)
add_test(NAME tst_theme_palette COMMAND tst_theme_palette)
set_tests_properties(tst_theme_palette PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_store: exercises the in-memory store's uniformly-recursive agent tree -
# roots, arbitrary depth, subtree-folded counts/scopes, and creation.
qt_add_executable(tst_store tst_store.cpp)
target_link_libraries(tst_store
    PRIVATE
        Qt6::Test
        daemon-app::domain
        daemon-app::persistence
)

add_test(NAME tst_store COMMAND tst_store)
set_tests_properties(tst_store PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_sidebar_model: exercises the flattened agent-tree sidebar model - recursive
# depths, tree roles, expand/collapse, and node-scope selection.
qt_add_executable(tst_sidebar_model tst_sidebar_model.cpp)
target_include_directories(tst_sidebar_model
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Sidebar"
)
target_link_libraries(tst_sidebar_model
    PRIVATE
        Qt6::Test
        Qt6::Qml
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::daemonnet
        daemon-app::sidebar
)

add_test(NAME tst_sidebar_model COMMAND tst_sidebar_model)
set_tests_properties(tst_sidebar_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_participants_model: the right sidebar's Participants section model (shared by
# the GUI and TUI) - the collapsible header, the seeded Agent/User rows with their
# green "available" presence dots, and the section fold.
qt_add_executable(tst_participants_model tst_participants_model.cpp)
target_include_directories(tst_participants_model
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Participants"
)
target_link_libraries(tst_participants_model
    PRIVATE
        Qt6::Test
        Qt6::Qml
        Qt6::Quick
        daemon-app::domain
        daemon-app::persistence
        daemon-app::daemonnet
        daemon-app::participants
)

add_test(NAME tst_participants_model COMMAND tst_participants_model)
set_tests_properties(tst_participants_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_turn_controller: exercises the shared turn-lifecycle FSM (the C++ port of
# TurnSimulator.qml) - a started turn runs its scripted events to completion and
# settles idle, the "fail" prompt drives the error branch, and cancel resets.
qt_add_executable(tst_turn_controller tst_turn_controller.cpp)
target_link_libraries(tst_turn_controller
    PRIVATE
        Qt6::Test
        daemon-app::turn
)
add_test(NAME tst_turn_controller COMMAND tst_turn_controller)
set_tests_properties(tst_turn_controller PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_status_model: guards byte-for-byte parity of the shared StatusBarModel's
# formatting (elapsed/abbrev/context bar+label+percent, gateway tone, agents
# detail) with the JS funcs StatusBar.qml previously used.
qt_add_executable(tst_status_model tst_status_model.cpp)
target_link_libraries(tst_status_model
    PRIVATE
        Qt6::Test
        daemon-app::status_model
)
add_test(NAME tst_status_model COMMAND tst_status_model)
set_tests_properties(tst_status_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_todos: the shared TodoListModel (composer status-stack todos) - role
# exposure, setTodos replacement + signalling, and clear.
qt_add_executable(tst_todos tst_todos.cpp)
target_include_directories(tst_todos
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Session"
)
target_link_libraries(tst_todos
    PRIVATE
        Qt6::Test
        Qt6::Quick
        daemon-app::session_view
)
add_test(NAME tst_todos COMMAND tst_todos)
set_tests_properties(tst_todos PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_subagent_model: the shared SubagentModel (live delegation rows) - role
# exposure, id-keyed upsert vs append, ignoring non-subagent events, running
# count, and clear. Both front ends render this model off the turn event stream.
qt_add_executable(tst_subagent_model tst_subagent_model.cpp)
target_include_directories(tst_subagent_model
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Session"
)
target_link_libraries(tst_subagent_model
    PRIVATE
        Qt6::Test
        Qt6::Quick
        daemon-app::session_view
)
add_test(NAME tst_subagent_model COMMAND tst_subagent_model)
set_tests_properties(tst_subagent_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_completion: the shared completion FSM (CompletionModel data/search + the
# trigger detection/navigation/accept transforms on ComposerSessionController).
qt_add_executable(tst_completion tst_completion.cpp)
target_link_libraries(tst_completion
    PRIVATE
        Qt6::Test
        daemon-app::composer_session
)
add_test(NAME tst_completion COMMAND tst_completion)
set_tests_properties(tst_completion PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_composer_session: the composer-session logic extracted from QML - the
# primary-action rule (send/queue/stop + enablement) and the model selector.
qt_add_executable(tst_composer_session tst_composer_session.cpp)
target_link_libraries(tst_composer_session
    PRIVATE
        Qt6::Test
        daemon-app::composer_session
        daemon-app::models
        daemon-app::uimodels
)
add_test(NAME tst_composer_session COMMAND tst_composer_session)
set_tests_properties(tst_composer_session PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_line_editor: the TUI composer's hand-rolled readline/emacs editing layer -
# the pure (text, cursor) command transforms + kill-ring, and the tiny ~/.inputrc
# remap parser (keyseq -> function-name). Compiles the source directly; it only
# depends on Qt6::Core (no Tui Widgets), so it runs in the normal unit suite.
qt_add_executable(tst_line_editor
    tst_line_editor.cpp
    ${CMAKE_SOURCE_DIR}/src/tui/line_editor.cpp
)
target_include_directories(tst_line_editor
    PRIVATE "${CMAKE_SOURCE_DIR}/src/tui"
)
target_link_libraries(tst_line_editor
    PRIVATE
        Qt6::Test
        Qt6::Core
)
add_test(NAME tst_line_editor COMMAND tst_line_editor)
set_tests_properties(tst_line_editor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_tab_model: the shared pane-tab model both frontends bind - find-or-create
# open semantics, active-tab tracking (currentIndex/CurrentRole), close-neighbour
# selection, the Settings singleton page, cycling, and reorder. Compiles the
# source directly (it only needs QtCore/QtQml, no QML engine) so the logic test
# stays in the fast unit suite.
qt_add_executable(tst_tab_model
    tst_tab_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
)
set_target_properties(tst_tab_model PROPERTIES AUTOMOC ON)
target_include_directories(tst_tab_model
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs"
)
target_link_libraries(tst_tab_model
    PRIVATE
        Qt6::Test
        Qt6::Qml
)
add_test(NAME tst_tab_model COMMAND tst_tab_model)
set_tests_properties(tst_tab_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_chat_conversation_controller (A4 app/chat-tabs; wire v38): the shared native-chat
# view-model BOTH surfaces bind — history load on open, the oldest-first markdown projection
# of ConvHistory rows, send-without-local-echo, authoritative MessagesChanged refresh (fg + bg),
# per-conversation signal filtering, and sendFailed surfacing. Compiles the controller source
# directly and links the transports seam (IChatService + MockChatService).
qt_add_executable(tst_chat_conversation_controller
    tst_chat_conversation_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Chat/chat_conversation_controller.cpp
)
set_target_properties(tst_chat_conversation_controller PROPERTIES AUTOMOC ON)
target_include_directories(tst_chat_conversation_controller
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Chat"
)
target_link_libraries(tst_chat_conversation_controller
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::transports
)
add_test(NAME tst_chat_conversation_controller COMMAND tst_chat_conversation_controller)
set_tests_properties(tst_chat_conversation_controller
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_session_orchestrator: the shared submit pipeline - submit starts the
# turn + populates todos (and clears them after the turn settles), cancel resets,
# and slash "/new" is handled while other commands surface via commandRequested.
qt_add_executable(tst_session_orchestrator tst_session_orchestrator.cpp)
target_include_directories(tst_session_orchestrator
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Session"
)
target_link_libraries(tst_session_orchestrator
    PRIVATE
        Qt6::Test
        Qt6::Quick
        daemon-app::session_view
        daemon-app::turn
        daemon-app::session
        daemon-app::profiles
        daemon-app::persistence
        daemon-app::domain
)
add_test(NAME tst_session_orchestrator COMMAND tst_session_orchestrator)
set_tests_properties(tst_session_orchestrator PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_transcript_search: the in-transcript search feasibility spike - the shared
# be::TranscriptSearchController collecting ordered matches over a be::DocumentStore
# (correct block/offset geometry, case sensitivity, typed-block visible text) and
# driving next/prev cursor navigation + the navigateTo anchor signal.
qt_add_executable(tst_transcript_search tst_transcript_search.cpp)
target_link_libraries(tst_transcript_search
    PRIVATE
        Qt6::Test
        be_core
)
add_test(NAME tst_transcript_search COMMAND tst_transcript_search)
set_tests_properties(tst_transcript_search PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_transcript_log: the P4 transcript<->SessionLogEntry bridge - decomposing a
# session's markdown into entries and rebuilding the document round-trips to the same
# markdown the parse path produces (the showcase agent blocks + the role layer).
qt_add_executable(tst_transcript_log tst_transcript_log.cpp)
target_link_libraries(tst_transcript_log
    PRIVATE
        Qt6::Test
        daemon-app::transcript_bridge
        daemon-app::domain
)
add_test(NAME tst_transcript_log COMMAND tst_transcript_log)
set_tests_properties(tst_transcript_log PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_editor_search: proves EditorController owns and exposes the shared find
# engine bound to its DocumentStore, re-collecting matches on both the reload
# (loadMarkdown) and documentChanged mutation paths. Links the block-editor QML
# module for the controller (the engine geometry is covered by tst_transcript_search).
qt_add_executable(tst_editor_search tst_editor_search.cpp)
target_link_libraries(tst_editor_search
    PRIVATE
        Qt6::Test
        Qt6::Quick
        # editor_controller.h transitively includes core/persistence.h (QSqlDatabase);
        # da_blockeditor links be_core privately, so the Sql include path is not
        # propagated to header consumers and must be added explicitly here.
        Qt6::Sql
        daemon-app::blockeditor
)
add_test(NAME tst_editor_search COMMAND tst_editor_search)
set_tests_properties(tst_editor_search PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_editor_feedback: EditorController's per-message thumbs-feedback API - submit
# records the rating (ratingFor), a re-rate updates it in place, an empty id is a
# no-op, and the resolved wire anchor is forwarded on messageFeedbackSubmitted (the
# anchor capture itself is covered by be_core_tests). Links the block-editor QML
# module for the controller (+ Qt6::Sql for the transitive persistence include).
qt_add_executable(tst_editor_feedback tst_editor_feedback.cpp)
target_link_libraries(tst_editor_feedback
    PRIVATE
        Qt6::Test
        Qt6::Quick
        Qt6::Sql
        daemon-app::blockeditor
)
add_test(NAME tst_editor_feedback COMMAND tst_editor_feedback)
set_tests_properties(tst_editor_feedback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_feedback: the user-feedback seam's in-memory stand-in (MockFeedback) -
# records message/app submissions, persists telemetry consent across restarts
# (dedup'd change signal), and only enables telemetry via the app-feedback
# dialog's explicit opt-in.
qt_add_executable(tst_feedback tst_feedback.cpp)
target_link_libraries(tst_feedback
    PRIVATE
        Qt6::Test
        daemon-app::feedback
)
add_test(NAME tst_feedback COMMAND tst_feedback)
set_tests_properties(tst_feedback PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_memory: the seeded memory seam (MockMemoryService) + the shared view-models
# that back BOTH front ends - list filter/sort/search, scope filtering, stat
# aggregation, the graph nodes/neighbours + kinds, the flattened timeline, and the
# Memory page singleton. Compiles the view-model sources directly (they need only
# QtCore/QtQml, no QML engine) so the logic test stays in the fast unit suite.
qt_add_executable(tst_memory
    tst_memory.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Memory/memory_list_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Memory/memory_stats_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Memory/memory_timeline_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Memory/memory_graph_model.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs/tab_model.cpp
)
set_target_properties(tst_memory PROPERTIES AUTOMOC ON)
target_include_directories(tst_memory
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/DaemonApp/Memory"
        "${CMAKE_SOURCE_DIR}/src/DaemonApp/Tabs"
)
target_link_libraries(tst_memory
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::memory
)
add_test(NAME tst_memory COMMAND tst_memory)
set_tests_properties(tst_memory PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_composer_upload: the browser (wasm) attachment upload-path helper
# (ComposerUploadController::uploadPathFor) - uploads/<utc-timestamp>-<sanitized-name>: UTC
# normalization, [A-Za-z0-9._-] sanitization, path basenaming, and the empty-name fallback.
# Compiles the source directly (only QtCore/QtQml + the platform/fs symbols it references, no QML
# engine) so the pure helper is covered on the desktop ctest run even though upload is wasm-only.
qt_add_executable(tst_composer_upload
    tst_composer_upload.cpp
    ${CMAKE_SOURCE_DIR}/src/DaemonApp/Composer/composer_upload_controller.cpp
)
set_target_properties(tst_composer_upload PROPERTIES AUTOMOC ON)
target_include_directories(tst_composer_upload
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Composer"
)
target_link_libraries(tst_composer_upload
    PRIVATE
        Qt6::Test
        Qt6::Qml
        daemon-app::fs
        daemon-app::platform
)
add_test(NAME tst_composer_upload COMMAND tst_composer_upload)
set_tests_properties(tst_composer_upload PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
