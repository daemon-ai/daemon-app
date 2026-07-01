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
# offered locale list, code->name resolution, that installing the embedded
# pseudolocale actually translates marked C++ and QML strings, that switching
# back to the source language uninstalls it, and that Arabic reports RTL. The
# pseudolocale catalog is compiled (lrelease) and embedded into the test under
# ":/i18n" so applyLocale("pseudo") can load it; custom lupdate/lrelease target
# names avoid clashing with the app-wide update_translations/release_translations.
qt_add_executable(tst_localization tst_localization.cpp)
target_link_libraries(tst_localization
    PRIVATE
        Qt6::Test
        daemon-app::i18n
)
qt_add_translations(tst_localization
    TS_FILES "${CMAKE_SOURCE_DIR}/i18n/daemon-app_pseudo.ts"
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
)
add_test(NAME tst_first_run_model COMMAND tst_first_run_model)
set_tests_properties(tst_first_run_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
