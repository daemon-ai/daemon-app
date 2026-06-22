# Shared command-palette catalog: a GUI-free QAbstractListModel of palette entries
# (id/title/group/hint/shortcut) with a substring `search`. The GUI binds it as the
# `Commands` context property (CommandPalette.qml); the TUI compiles the source into
# da_tui_viewmodels and reads visibleCommands() to fill its PaletteDialog. Core-only
# (QAbstractListModel); no Qt Quick.
add_library(da_command STATIC
    command_registry.h
    command_registry.cpp
)
add_library(daemon-app::command ALIAS da_command)

set_target_properties(da_command PROPERTIES AUTOMOC ON)

target_include_directories(da_command PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_command
    PUBLIC
        Qt6::Core
    PRIVATE
        daemon-app::warnings
)
