# Each feature is its own QML module under a directory mirroring its URI
# (DaemonApp.<Feature>). Only App is wired in the minimal foundation; the rest
# are added as they are implemented.

add_subdirectory(Theme)
add_subdirectory(Controls)
add_subdirectory(BlockEditor)
add_subdirectory(Turn)
add_subdirectory(Transcript)
add_subdirectory(ComposerSession)
add_subdirectory(Composer)
add_subdirectory(Sidebar)
add_subdirectory(ConversationsList)
add_subdirectory(Conversation)
add_subdirectory(Tabs)
add_subdirectory(Settings)
add_subdirectory(StatusModel)
add_subdirectory(StatusBar)
add_subdirectory(Command)
add_subdirectory(Export)
add_subdirectory(App)
