# The App module is the application executable and the DaemonApp.App QML module.
# (Target name 'daemon-app' intentionally differs from the URI 'DaemonApp.App'.)

qt_add_executable(daemon-app
    main.cpp
)

qt_add_qml_module(daemon-app
    URI DaemonApp.App
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(daemon-app
    PRIVATE
        Qt6::Quick
        daemon-app::warnings
)

# As feature modules land, compose them here:
#   target_link_libraries(daemon-app PRIVATE da_sidebar da_conversation ...)
