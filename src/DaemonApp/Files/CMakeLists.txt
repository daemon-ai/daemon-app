# DaemonApp.Files - the daemon-served file tree + fuzzy finder. Lazy models over
# the IFsService seam (da_fs); no local filesystem access lives here.
qt_add_qml_module(da_files
    STATIC
    URI DaemonApp.Files
    VERSION 1.0
    SOURCES
        fs_explorer_model.cpp
        fs_explorer_model.h
        file_finder_model.cpp
        file_finder_model.h
    QML_FILES
        qml/FileTree.qml
        qml/FileFinder.qml
        qml/SearchResults.qml
        qml/FilePicker.qml
        qml/FileExplorer.qml
)
add_library(daemon-app::files ALIAS da_files)

target_include_directories(da_files PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(da_files
    PRIVATE
        Qt6::Quick
        daemon-app::fs
        daemon-app::theme
        daemon-app::controls
        daemon-app::warnings
)
