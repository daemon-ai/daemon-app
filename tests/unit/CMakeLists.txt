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
        daemon-app::sidebar
)

add_test(NAME tst_sidebar_model COMMAND tst_sidebar_model)
set_tests_properties(tst_sidebar_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Conversation"
)
target_link_libraries(tst_todos
    PRIVATE
        Qt6::Test
        Qt6::Quick
        daemon-app::conversation
)
add_test(NAME tst_todos COMMAND tst_todos)
set_tests_properties(tst_todos PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

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
)
add_test(NAME tst_composer_session COMMAND tst_composer_session)
set_tests_properties(tst_composer_session PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tst_conversation_orchestrator: the shared submit pipeline - submit starts the
# turn + populates todos (and clears them after the turn settles), cancel resets,
# and slash "/new" is handled while other commands surface via commandRequested.
qt_add_executable(tst_conversation_orchestrator tst_conversation_orchestrator.cpp)
target_include_directories(tst_conversation_orchestrator
    PRIVATE "${CMAKE_SOURCE_DIR}/src/DaemonApp/Conversation"
)
target_link_libraries(tst_conversation_orchestrator
    PRIVATE
        Qt6::Test
        Qt6::Quick
        daemon-app::conversation
        daemon-app::turn
        daemon-app::persistence
        daemon-app::domain
)
add_test(NAME tst_conversation_orchestrator COMMAND tst_conversation_orchestrator)
set_tests_properties(tst_conversation_orchestrator PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
