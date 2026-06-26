# DaemonApp.Participants - the right sidebar's "Participants" section: a flat model
# over the session store's participant roster (copied from the DaemonNet seed),
# mirroring the left sidebar's Tags section. Shared verbatim with the TUI.
qt_add_qml_module(da_participants
    STATIC
    URI DaemonApp.Participants
    VERSION 1.0
    SOURCES
        participants_model.cpp
        participants_model.h
    QML_FILES
        qml/Participants.qml
)
add_library(daemon-app::participants ALIAS da_participants)

target_include_directories(da_participants PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_participants
    PRIVATE
        Qt6::Quick
        daemon-app::persistence
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
)
